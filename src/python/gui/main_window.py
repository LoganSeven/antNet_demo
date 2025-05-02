from qtpy.QtWidgets import QMainWindow, QLabel, QVBoxLayout, QPushButton, QWidget
from core.core_manager import CoreManager

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AntNet Demo")

        self.core_manager = CoreManager()
        self.core_manager.start(num_workers=1)  # Change here to start multiple colonies later!

        # Connect callback adapters for all workers
        self.iteration_count = 0
        self.label_best_paths = []
        layout = QVBoxLayout()

        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            label = QLabel(f"Worker {idx} Best Path: --")
            self.label_best_paths.append(label)
            layout.addWidget(label)

            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)

        self.label_iterations = QLabel("Iterations: 0")
        layout.addWidget(self.label_iterations)

        self.button_stop = QPushButton("Stop All Workers")
        self.button_stop.clicked.connect(self.core_manager.stop)
        layout.addWidget(self.button_stop)

        central = QWidget()
        central.setLayout(layout)
        self.setCentralWidget(central)

    def update_best_path(self, worker_idx, path_info):
        """
        Update best path label for a specific worker.
        """
        nodes = path_info.get("nodes", [])
        total_latency = path_info.get("total_latency", "unknown")
        self.label_best_paths[worker_idx].setText(
            f"Worker {worker_idx} Best Path: {nodes} (Latency: {total_latency}ms)"
        )

    def on_iteration_done(self):
        """
        Update iteration count.
        """
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        """
        Ensure proper cleanup when window is closed.
        """
        self.core_manager.stop()
        super().closeEvent(event)
