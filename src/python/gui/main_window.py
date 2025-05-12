# src/python/gui/main_window.py
"""
MainWindow sets up the GUI layout, starts the CoreManager (which spins up the Workers),
and connects signals for solver results to be drawn in the GraphScene.
"""

from qtpy.QtCore import Qt, QTimer, Signal, QEvent
from qtpy.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QSplitter,
    QLabel,
    QPushButton,
    QSizePolicy
)
from qtpy.QtGui import QColor
from core.core_manager import CoreManager
from gui.graph_view.graph_canvas import GraphCanvas
from gui.control_widget import ControlWidget
from gui.aco_visu_widget import AcoVisuWidget
from gui.managers.signal_manager import SignalManager
from gui.consts.gui_consts import ALGO_COLORS

class MainWindow(QMainWindow):
    splitter_horizontal_released = Signal()
    splitter_vertical_released = Signal()
    window_resize_finished = Signal()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("AntNet Demo")
        self._first_show = True

        self._is_dragging_splitter_h = False
        self._is_dragging_splitter_v = False
        self._is_resizing_window = False

        # Create the CoreManager, but do NOT start workers yet
        self.core_manager = CoreManager()
        self.iteration_count = 0

        # Store last logged latencies for each solver to avoid duplicates
        self.last_logged_latencies = {
            "aco": None,
            "random": None,
            "brute": None
        }

        # Main zones
        self.zone1 = QWidget()
        self.zone2 = QWidget()
        self.zone3 = QWidget()

        size_policy_expanding = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.zone1.setSizePolicy(size_policy_expanding)
        self.zone2.setSizePolicy(size_policy_expanding)
        self.zone3.setSizePolicy(size_policy_expanding)

        self.main_splitter = QSplitter(Qt.Horizontal, self)
        self.sub_splitter = QSplitter(Qt.Vertical)
        self.main_splitter.addWidget(self.zone1)
        self.main_splitter.addWidget(self.sub_splitter)
        self.sub_splitter.addWidget(self.zone2)
        self.sub_splitter.addWidget(self.zone3)
        self.setCentralWidget(self.main_splitter)

        self.zone2.setMinimumWidth(150)
        self.zone3.setMinimumWidth(150)

        for i in range(2):
            self.main_splitter.setCollapsible(i, True)
            self.sub_splitter.setCollapsible(i, True)

        self.main_splitter.setStretchFactor(0, 3)
        self.main_splitter.setStretchFactor(1, 1)
        self.sub_splitter.setStretchFactor(0, 1)
        self.sub_splitter.setStretchFactor(1, 2)

        handle_style = """
        QSplitter::handle { background-color: #0a87ba; }
        QSplitter::handle:horizontal { width: 6px; }
        QSplitter::handle:vertical { height: 6px; }
        """
        self.main_splitter.setStyleSheet(handle_style)
        self.sub_splitter.setStyleSheet(handle_style)

        for idx in range(self.main_splitter.count() - 1):
            handle = self.main_splitter.handle(idx + 1)
            handle.installEventFilter(self)
        for idx in range(self.sub_splitter.count() - 1):
            handle = self.sub_splitter.handle(idx + 1)
            handle.installEventFilter(self)

        # Zone 1: Graph area
        zone1_layout = QVBoxLayout(self.zone1)
        zone1_layout.setContentsMargins(8, 8, 8, 8)
        self.graph_canvas = GraphCanvas(self.zone1)
        self.graph_canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        zone1_layout.addWidget(self.graph_canvas)

        # Zone 2: ACO Visualization
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

        # Add a button to send the current topology to the backend
        self.button_update_topology = QPushButton("Update Topology", self.zone3)
        self.button_update_topology.clicked.connect(self.on_button_update_topology)
        zone3_layout.addWidget(self.button_update_topology)

        self.signal_manager = SignalManager(self, self.control, self.core_manager)

        # Now start the workers (which create contexts in the C backend) from .ini config
        self.core_manager.start(num_workers=1, from_config="config/settings.ini")

        # Retrieve the backend config to see how many nodes are configured
        config_data = self.core_manager.workers[0][0].backend.get_config()
        nb_nodes = config_data["set_nb_nodes"]

        # Initialize the scene with the configured number of nodes
        self.graph_canvas.scene.init_scene_with_nodes(nb_nodes)

        # Create a default chain of edges after the nodes are created
        self.graph_canvas.scene.create_default_edges()

        # (Optional) Render the manager edges for debugging
        self.graph_canvas.scene.render_manager_edges()

        # Immediately push the resulting topology so each worker sees it before the first iteration
        topology_data = self.graph_canvas.scene.export_graph_topology()
        self.core_manager.update_topology(topology_data)

        # Connect core system callbacks
        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)
            adapter.signal_pheromone_matrix.connect(self.on_pheromone_matrix)

    def showEvent(self, event):
        if self._first_show:
            screen = self.screen().availableGeometry()
            width = int(screen.width() * 0.8)
            height = int(screen.height() * 0.8)
            self.resize(width, height)
            self.move(screen.center() - self.rect().center())
            QTimer.singleShot(0, self._adjust_initial_splitter_positions)
            self._first_show = False
        super().showEvent(event)

    def _adjust_initial_splitter_positions(self):
        main_width = self.main_splitter.width()
        self.main_splitter.setSizes([main_width // 2, main_width // 2])

        sub_height = self.sub_splitter.height()
        self.sub_splitter.setSizes([sub_height // 2, sub_height // 2])

    def update_best_path(self, worker_idx, path_info):
        """
        Receives a dict with 'aco', 'random', and 'brute' sub-dicts,
        each containing 'nodes' and 'total_latency'. Logs the info
        and draws multiple paths simultaneously, skipping repeated latencies or empty paths.
        """
        for algo_key, algo_label in [("aco", "ACO"), ("random", "RND"), ("brute", "BF")]:
            if algo_key in path_info:
                data = path_info[algo_key]
                nodes = data.get("nodes", [])
                if nodes:
                    print(f"\033[36m[DEBUG]\033[0m {algo_label} path: {nodes}")
                    latency = data.get("total_latency", 0)
                    previous_latency = self.last_logged_latencies[algo_key]
                    if previous_latency is None or previous_latency != latency:
                        self.aco_visu.addLog(f"{algo_label}: {latency}", ALGO_COLORS[algo_key])
                        self.last_logged_latencies[algo_key] = latency

        self.graph_canvas.scene.draw_multiple_paths(path_info)

    def on_iteration_done(self):
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        self.core_manager.stop()
        super().closeEvent(event)

    def resizeEvent(self, event):
        self._is_resizing_window = True
        super().resizeEvent(event)

    def eventFilter(self, obj, event):
        if event.type() == QEvent.MouseButtonPress:
            if obj in [self.main_splitter.handle(i + 1) for i in range(self.main_splitter.count() - 1)]:
                self._is_dragging_splitter_h = True
            elif obj in [self.sub_splitter.handle(i + 1) for i in range(self.sub_splitter.count() - 1)]:
                self._is_dragging_splitter_v = True

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
        if self._is_resizing_window:
            self.window_resize_finished.emit()
            self._is_resizing_window = False
        super().mouseReleaseEvent(event)

    def on_button_update_topology(self):
        """
        Triggered by the 'Update Topology' button.
        Exports the current graph structure from the scene and sends it to the backend.
        """
        topology_data = self.graph_canvas.scene.export_graph_topology()
        self.core_manager.update_topology(topology_data)

    def on_pheromone_matrix(self, matrix: list[float]):
        """
        Called whenever Worker emits new pheromone data.
        Pass it to the scene so it can redraw the heatmap behind the nodes.
        """
        print(f"[DEBUG] on_pheromone_matrix called with size={len(matrix)}")
        self.graph_canvas.scene.update_heatmap(matrix)
