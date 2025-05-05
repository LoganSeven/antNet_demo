from qtpy.QtCore import Qt, QTimer, Signal, QEvent
from qtpy.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QSplitter,
    QLabel,
    QPushButton,
    QSizePolicy,
    QApplication
)
from qtpy.QtGui import QColor
from core.core_manager import CoreManager
from gui.graph_view.graph_canvas import GraphCanvas
from gui.control_widget import ControlWidget
from gui.aco_visu_widget import AcoVisuWidget
from gui.managers.signal_manager import SignalManager


class MainWindow(QMainWindow):
    splitter_horizontal_released = Signal()
    splitter_vertical_released = Signal()
    window_resize_finished = Signal()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("AntNet Demo")
        self._first_show = True  # Flag for initial layout setup

        # Internal flags to detect drag and resize
        self._is_dragging_splitter_h = False
        self._is_dragging_splitter_v = False
        self._is_resizing_window = False

        # Initialize core system components
        self.core_manager = CoreManager()
        self.core_manager.start(num_workers=1)

        # Application state initialization
        self.iteration_count = 0

        # Main UI containers setup
        self.zone1 = QWidget()
        self.zone2 = QWidget()
        self.zone3 = QWidget()

        # Configure container size policies
        size_policy_expanding = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.zone1.setSizePolicy(size_policy_expanding)
        self.zone2.setSizePolicy(size_policy_expanding)
        self.zone3.setSizePolicy(size_policy_expanding)

        # Create splitter hierarchy
        self.main_splitter = QSplitter(Qt.Horizontal, self)
        self.sub_splitter = QSplitter(Qt.Vertical)
        self.main_splitter.addWidget(self.zone1)
        self.main_splitter.addWidget(self.sub_splitter)
        self.sub_splitter.addWidget(self.zone2)
        self.sub_splitter.addWidget(self.zone3)
        self.setCentralWidget(self.main_splitter)

        # Configure splitter minimum sizes
        self.zone2.setMinimumWidth(150)
        self.zone3.setMinimumWidth(150)

        # Enable splitter collapsibility
        for i in range(2):
            self.main_splitter.setCollapsible(i, True)
            self.sub_splitter.setCollapsible(i, True)

        # Set splitter stretch factors
        self.main_splitter.setStretchFactor(0, 3)
        self.main_splitter.setStretchFactor(1, 1)
        self.sub_splitter.setStretchFactor(0, 1)
        self.sub_splitter.setStretchFactor(1, 2)

        # Configure splitter visual style
        handle_style = """
        QSplitter::handle { background-color: #0a87ba; }
        QSplitter::handle:horizontal { width: 6px; }
        QSplitter::handle:vertical { height: 6px; }
        """
        self.main_splitter.setStyleSheet(handle_style)
        self.sub_splitter.setStyleSheet(handle_style)

        # Install event filters on splitter handles to detect drag start and release
        for idx in range(self.main_splitter.count() - 1):
            handle = self.main_splitter.handle(idx + 1)
            handle.installEventFilter(self)
        for idx in range(self.sub_splitter.count() - 1):
            handle = self.sub_splitter.handle(idx + 1)
            handle.installEventFilter(self)

        # Zone 1: Graph visualization area
        zone1_layout = QVBoxLayout(self.zone1)
        zone1_layout.setContentsMargins(8, 8, 8, 8)
        self.graph_canvas = GraphCanvas(self.zone1)
        self.graph_canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone1_layout.addWidget(self.graph_canvas)

        # Zone 2: Algorithm visualization and controls
        zone2_layout = QVBoxLayout(self.zone2)
        zone2_layout.setContentsMargins(8, 8, 8, 8)
        zone2_layout.setSpacing(6)

        self.label_iterations = QLabel("Iterations: 0", self.zone2)
        self.label_iterations.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        zone2_layout.addWidget(self.label_iterations)

        self.aco_visu = AcoVisuWidget(self.zone2)
        self.aco_visu.draw_demo_graph()
        self.aco_visu.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone2_layout.addWidget(self.aco_visu)

        self.button_stop = QPushButton("Stop All Workers", self.zone2)
        self.button_stop.clicked.connect(self.core_manager.stop)
        zone2_layout.addWidget(self.button_stop)

        # Zone 3: Control panel
        zone3_layout = QVBoxLayout(self.zone3)
        zone3_layout.setContentsMargins(8, 8, 8, 8)
        zone3_layout.setSpacing(4)

        self.control = ControlWidget(self.zone3)
        self.control.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone3_layout.addWidget(self.control)
        zone3_layout.setStretch(0, 1)

        # Connect gui signal manager
        self.signal_manager = SignalManager(self, self.control, self.core_manager)

        # Connect core system callbacks
        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)

    def showEvent(self, event):
        """Handle initial window positioning and sizing on first display"""
        if self._first_show:
            screen = self.screen().availableGeometry()
            width = int(screen.width() * 0.8)
            height = int(screen.height() * 0.8)

            # Set window dimensions and center position
            self.resize(width, height)
            self.move(screen.center() - self.rect().center())

            # Delay splitter adjustment until after initial layout
            QTimer.singleShot(0, self._adjust_initial_splitter_positions)
            self._first_show = False
        super().showEvent(event)

    def _adjust_initial_splitter_positions(self):
        """Set initial splitter positions to 50% ratios"""
        main_width = self.main_splitter.width()
        self.main_splitter.setSizes([main_width // 2, main_width // 2])

        sub_height = self.sub_splitter.height()
        self.sub_splitter.setSizes([sub_height // 2, sub_height // 2])

    def update_best_path(self, worker_idx, path_info):
        """Handle best path updates from core system"""
        hops = path_info.get("nodes", [])
        latency = path_info.get("total_latency", "unknown")
        log_msg = f"Worker {worker_idx} Best Path: {hops} (Latency: {latency}ms)"
        self.aco_visu.addLog(log_msg, "#8B0000")  # Dark red color
        self.graph_canvas.scene.draw_best_path_by_hop_indices(hops)

    def on_iteration_done(self):
        """Update iteration counter display"""
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        """Ensure clean shutdown of background workers"""
        self.core_manager.stop()
        super().closeEvent(event)

    def resizeEvent(self, event):
        self._is_resizing_window = True
        super().resizeEvent(event)

    def eventFilter(self, obj, event):
        # Detect press on splitter handles
        if event.type() == QEvent.MouseButtonPress:
            if obj in [self.main_splitter.handle(i+1) for i in range(self.main_splitter.count()-1)]:
                self._is_dragging_splitter_h = True
            elif obj in [self.sub_splitter.handle(i+1) for i in range(self.sub_splitter.count()-1)]:
                self._is_dragging_splitter_v = True

        # Detect release to emit signals
        elif event.type() == QEvent.MouseButtonRelease:
            if self._is_dragging_splitter_h:
                self.splitter_horizontal_released.emit()
                self._is_dragging_splitter_h = False
            if self._is_dragging_splitter_v:
                self.splitter_vertical_released.emit()
                self._is_dragging_splitter_v = False
            if self._is_resizing_window:
                self.window_resize_finished.emit()
                self._is_resizing_window = False

        return super().eventFilter(obj, event)

    def mouseReleaseEvent(self, event):
        # Fallback for window resize signal if release occurs outside handle
        if self._is_resizing_window:
            self.window_resize_finished.emit()
            self._is_resizing_window = False
        super().mouseReleaseEvent(event)
