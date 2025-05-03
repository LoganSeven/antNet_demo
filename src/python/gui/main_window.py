from qtpy.QtCore import Qt
from qtpy.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QSplitter,
    QLabel,
    QPushButton,
    QSizePolicy,
)
from core.core_manager import CoreManager
from gui.graph_view.graph_canvas import GraphCanvas
from gui.control_widget import ControlWidget


class MainWindow(QMainWindow):
    """
    Main window with three resizable zones.
    Zone 1 on the left (canvas).
    Zones 2 and 3 stacked on the right (status and controls).
    Splitters and size policies configured for full responsiveness.
    Dynamically scales font size in Zone 2 for reactive resizing.
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("AntNet Demo")

        # Backend manager initialization
        self.core_manager = CoreManager()
        self.core_manager.start(num_workers=1)

        self.iteration_count = 0
        self.label_best_paths = []

        # Zone containers
        self.zone1 = QWidget()
        self.zone2 = QWidget()
        self.zone3 = QWidget()

        # Size policy adjustments
        self.zone1.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.zone2.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.zone3.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Splitters setup
        self.main_splitter = QSplitter(Qt.Horizontal, self)
        self.sub_splitter = QSplitter(Qt.Vertical)

        # Assemble splitter hierarchy
        self.main_splitter.addWidget(self.zone1)
        self.main_splitter.addWidget(self.sub_splitter)
        self.sub_splitter.addWidget(self.zone2)
        self.sub_splitter.addWidget(self.zone3)
        self.setCentralWidget(self.main_splitter)

        # Allow partial collapsing but not vanish
        self.zone2.setMinimumWidth(150)
        self.zone3.setMinimumWidth(150)

        self.main_splitter.setCollapsible(0, True)
        self.main_splitter.setCollapsible(1, True)
        self.sub_splitter.setCollapsible(0, True)
        self.sub_splitter.setCollapsible(1, True)

        # Splitter stretch factors
        self.main_splitter.setStretchFactor(0, 3)  # Zone 1 gets more width
        self.main_splitter.setStretchFactor(1, 1)
        self.sub_splitter.setStretchFactor(0, 1)
        self.sub_splitter.setStretchFactor(1, 2)

        # Initial splitter sizes
        self.main_splitter.setSizes([900, 300])  # Adjust if needed
        self.sub_splitter.setSizes([200, 600])

        # Splitter handle styling
        handle_style = """
        QSplitter::handle { background-color: #808080; }
        QSplitter::handle:horizontal { width: 6px; }
        QSplitter::handle:vertical   { height: 6px; }
        """
        self.main_splitter.setStyleSheet(handle_style)
        self.sub_splitter.setStyleSheet(handle_style)

        # Zone 1: graph canvas setup
        zone1_layout = QVBoxLayout(self.zone1)
        zone1_layout.setContentsMargins(8, 8, 8, 8)
        self.graph_canvas = GraphCanvas(self.zone1)
        self.graph_canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone1_layout.addWidget(self.graph_canvas)

        # Zone 2: status labels and stop button
        zone2_layout = QVBoxLayout(self.zone2)
        zone2_layout.setContentsMargins(8, 8, 8, 8)
        zone2_layout.setSpacing(4)

        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            label = QLabel(f"Worker {idx} Best Path: --", self.zone2)
            label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
            self.label_best_paths.append(label)
            zone2_layout.addWidget(label)

            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)

        self.label_iterations = QLabel("Iterations: 0", self.zone2)
        self.label_iterations.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        zone2_layout.addWidget(self.label_iterations)

        self.button_stop = QPushButton("Stop All Workers", self.zone2)
        self.button_stop.clicked.connect(self.core_manager.stop)
        zone2_layout.addWidget(self.button_stop)

        # Ensure zone2 does not expand beyond content
        zone2_layout.addStretch(0)

        # Zone 3: control panel
        zone3_layout = QVBoxLayout(self.zone3)
        zone3_layout.setContentsMargins(8, 8, 8, 8)
        zone3_layout.setSpacing(4)

        control = ControlWidget(self.zone3)
        control.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone3_layout.addWidget(control)
        zone3_layout.setStretch(0, 1)

        # Base references for dynamic font resizing in zone2
        self._zone2_base_size = None
        self._zone2_base_font_size = None

    def update_best_path(self, worker_idx, path_info):
        """
        Updates best path label for the specified worker and draws it.
        Expects path_info["nodes"] as zero-based hop indices.
        """
        hops = path_info.get("nodes", [])
        latency = path_info.get("total_latency", "unknown")

        self.label_best_paths[worker_idx].setText(
            f"Worker {worker_idx} Best Path: {hops} (Latency: {latency}ms)"
        )
        self.graph_canvas.scene.draw_best_path_by_hop_indices(hops)

    def on_iteration_done(self):
        """
        Increments iteration counter and updates display.
        """
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        """
        Ensures backend shutdown before application closes.
        """
        self.core_manager.stop()
        super().closeEvent(event)

    def resizeEvent(self, event):
        """
        Dynamically scales font size for Zone 2 based on its resized dimensions.
        """
        super().resizeEvent(event)

        # Record base size for zone2 only once
        if self._zone2_base_size is None:
            self._zone2_base_size = self.zone2.size()
            current_font = self.zone2.font()
            # If pointSize == -1, it's in pixels, so use pixelSize
            if current_font.pointSize() > 0:
                self._zone2_base_font_size = current_font.pointSize()
            else:
                self._zone2_base_font_size = current_font.pixelSize() or 12

        if (self._zone2_base_size is not None and
                self._zone2_base_size.isValid() and
                self._zone2_base_font_size):

            w_ratio = self.zone2.width() / self._zone2_base_size.width()
            h_ratio = self.zone2.height() / self._zone2_base_size.height()
            scale = min(w_ratio, h_ratio)

            new_font_size = max(1, int(self._zone2_base_font_size * scale))

            current_font = self.zone2.font()
            # Could use setPixelSize for uniform cross-platform
            current_font.setPointSize(new_font_size)

            self.zone2.setFont(current_font)

            # Optionally apply to children explicitly in zone2
            for child in self.zone2.findChildren(QWidget):
                child.setFont(current_font)
