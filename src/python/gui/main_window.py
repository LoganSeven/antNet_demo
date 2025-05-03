from qtpy.QtCore import Qt
from qtpy.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QPushButton,
    QLabel,
    QSplitter,
)
from core.core_manager import CoreManager

class MainWindow(QMainWindow):
    """
    Main window with three resizable zones. Zone 1 on the left (full height, half width),
    Zone 2 top-right (half height, half width), and Zone 3 bottom-right (half height, half width).
    Splitters between zones are styled for visibility.
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("AntNet Demo")

        self.core_manager = CoreManager()
        self.core_manager.start(num_workers=1)

        self.iteration_count = 0
        self.label_best_paths = []

        # Create the three zones as empty widgets
        self.zone1 = QWidget()
        self.zone2 = QWidget()
        self.zone3 = QWidget()

        # Splitter for Zone 1 (left) and a sub-splitter for Zones 2 and 3 (right)
        self.main_splitter = QSplitter(Qt.Horizontal, self)
        self.sub_splitter = QSplitter(Qt.Vertical)

        # Add Zone 2 and Zone 3 to the sub-splitter
        self.sub_splitter.addWidget(self.zone2)
        self.sub_splitter.addWidget(self.zone3)

        # Add Zone 1 and the sub-splitter to the main splitter
        self.main_splitter.addWidget(self.zone1)
        self.main_splitter.addWidget(self.sub_splitter)

        # Set main splitter as the central widget
        self.setCentralWidget(self.main_splitter)

        # Style splitters for improved handle visibility
        splitter_style = """
        QSplitter::handle {
            background-color: #808080;
        }
        QSplitter::handle:horizontal {
            width: 6px;
        }
        QSplitter::handle:vertical {
            height: 6px;
        }
        """
        self.main_splitter.setStyleSheet(splitter_style)
        self.sub_splitter.setStyleSheet(splitter_style)

        # Provide initial sizes (width/height) for the splitters
        self.main_splitter.setSizes([400, 400])
        self.sub_splitter.setSizes([200, 200])

        # Prepare layout for Zone 1 to hold the existing controls
        zone1_layout = QVBoxLayout(self.zone1)

        # Connect adapter signals and create labels
        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            label = QLabel(f"Worker {idx} Best Path: --", self.zone1)
            self.label_best_paths.append(label)
            zone1_layout.addWidget(label)

            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)

        self.label_iterations = QLabel("Iterations: 0", self.zone1)
        zone1_layout.addWidget(self.label_iterations)

        self.button_stop = QPushButton("Stop All Workers", self.zone1)
        self.button_stop.clicked.connect(self.core_manager.stop)
        zone1_layout.addWidget(self.button_stop)

    def update_best_path(self, worker_idx, path_info):
        """
        Updates best path label for a specific worker.
        """
        nodes = path_info.get("nodes", [])
        total_latency = path_info.get("total_latency", "unknown")
        self.label_best_paths[worker_idx].setText(
            f"Worker {worker_idx} Best Path: {nodes} (Latency: {total_latency}ms)"
        )

    def on_iteration_done(self):
        """
        Updates iteration count.
        """
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        """
        Ensures proper cleanup when the window is closed.
        """
        self.core_manager.stop()
        super().closeEvent(event)
