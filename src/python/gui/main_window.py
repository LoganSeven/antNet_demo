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
from gui.widgets.control_widget import ControlWidget
from gui.widgets.aco_visu_widget import AcoVisuWidget
from gui.managers.signal_manager import SignalManager
from gui.consts.gui_consts import ALGO_COLORS
from ffi.backend_api import render_heatmap_rgba, init_async_renderer, shutdown_async_renderer

# Import your TabSheetWidget (fit_tabsheet) here:
from gui.widgets.tab_sheet_widget import TabSheetWidget

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

        self.last_logged_latencies = {
            "aco": None,
            "random": None,
            "brute": None
        }

        # Initialize the async renderer with a small default size
        init_async_renderer(width=64, height=64)

        # Attempt a small test for the renderer
        self.opengl_ok = False
        try:
            pts_xy = [-0.8, -0.8, -0.5, -0.5, -0.2, -0.2, 0.0, 0.0, 0.2, 0.2,
                      0.4, 0.4, 0.6, 0.6, 0.8, 0.8, -0.6, 0.6, 0.6, -0.6]
            strength = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
            w, h = 50, 50
            out = render_heatmap_rgba(pts_xy, strength, w, h)
            if out and len(out) == w * h * 4:
                self.opengl_ok = True
        except Exception as e:
            print(f"[DEBUG] OpenGL heatmap test failed: {e}")

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
        zone2_layout.addWidget(self.button_stop)

        # Zone 3: Control panel area
        zone3_layout = QVBoxLayout(self.zone3)
        zone3_layout.setContentsMargins(8, 8, 8, 8)
        zone3_layout.setSpacing(4)

        # ------------------------------------------------------------
        # Keep ControlWidget creation, but do NOT add it to layout,
        # hide it, and mark TODO remove after refactoring
        # ------------------------------------------------------------
        self.control = ControlWidget(self.zone3)
        self.control.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.control.setVisible(False)  # hidden
        # TODO remove after refactoring
        # zone3_layout.addWidget(self.control)  # commented out intentionally

        # ------------------------------------------------------------
        # Create a new fit_tabsheet with 3 tabs: Topology, ACO parameters, Ranking parameters
        # ------------------------------------------------------------
        self.tab_sheet = TabSheetWidget(self.zone3)
        self.tab_sheet.add_tabsheet("Topology")
        self.tab_sheet.add_tabsheet("ACO parameters")
        self.tab_sheet.add_tabsheet("Ranking parameters")

        zone3_layout.addWidget(self.tab_sheet)
        zone3_layout.setStretch(0, 1)

        # Example button placed after the tab sheet
        self.button_update_topology = QPushButton("Update Topology", self.zone3)
        zone3_layout.addWidget(self.button_update_topology)

        self.signal_manager = SignalManager(self, self.control, self.core_manager)

        self.graph_canvas.scene.set_gpu_ok(self.opengl_ok)

        # Now start the workers from .ini config
        self.core_manager.start(num_workers=1, from_config="config/settings.ini")

        # Retrieve config to see how many nodes and delay range
        config_data = self.core_manager.workers[0][0].backend.get_config()
        nb_nodes = config_data["set_nb_nodes"]
        min_delay = config_data["default_min_delay"]
        max_delay = config_data["default_max_delay"]

        # Update the scene's HopMapManager with the new delay range
        self.graph_canvas.scene.hop_map_manager.set_delay_range(min_delay, max_delay)

        # Initialize the scene with the configured number of nodes
        self.graph_canvas.scene.init_scene_with_nodes(nb_nodes)
        self.graph_canvas.scene.create_default_edges()
        self.graph_canvas.scene.render_manager_edges()

        # Auto-inject topology once everything is ready
        topology_data = self.graph_canvas.scene.export_graph_topology()
        print("[DEBUG] Auto-injecting initial topology to CoreManager...")
        self.core_manager.update_topology(topology_data)

        # Connect signals from the workers
        adapters = self.core_manager.get_callback_adapters()
        for idx, adapter in enumerate(adapters):
            adapter.signal_best_path_updated.connect(
                lambda path_info, idx=idx: self.update_best_path(idx, path_info)
            )
            adapter.signal_iteration_done.connect(self.on_iteration_done)
            adapter.signal_pheromone_matrix.connect(self.on_pheromone_matrix)
            adapter.signal_ranking_updated.connect(self.on_ranking_updated)

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
        for algo_key, algo_label in [("aco", "ACO"), ("random", "RND"), ("brute", "BF")]:
            if algo_key in path_info:
                data = path_info[algo_key]
                nodes = data.get("nodes", [])
                if nodes:
                    latency = data.get("total_latency", 0)
                    previous_latency = self.last_logged_latencies[algo_key]
                    if previous_latency is None or previous_latency != latency:
                        print(f"[DEBUG] {algo_label} best_path: {nodes}")
                        self.last_logged_latencies[algo_key] = latency
                        self.graph_canvas.scene.draw_multiple_paths(path_info)

    def on_iteration_done(self):
        self.iteration_count += 1
        self.label_iterations.setText(f"Iterations: {self.iteration_count}")

    def closeEvent(self, event):
        self.core_manager.stop()
        shutdown_async_renderer()
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
        topology_data = self.graph_canvas.scene.export_graph_topology()
        print("[DEBUG] on_button_update_topology called; sending to CoreManager...")
        self.core_manager.update_topology(topology_data)

    def on_pheromone_matrix(self, matrix: list[float]):
        self.graph_canvas.scene.update_heatmap(matrix)

    def on_ranking_updated(self, ranking: list):
        self.aco_visu.showRanking(ranking)
