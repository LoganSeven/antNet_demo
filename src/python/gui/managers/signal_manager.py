# src/python/gui/managers/signal_manager.py

from __future__ import annotations
from typing import TYPE_CHECKING
from qtpy.QtCore import QObject, Qt, QMetaObject

if TYPE_CHECKING:
    from gui.main_window import MainWindow
    from gui.control_widget import ControlWidget
    from core.core_manager import CoreManager


class SignalManager(QObject):
    def __init__(self, main_window: MainWindow, control: ControlWidget, core_manager: CoreManager):
        super().__init__()
        self.main_window = main_window
        self.control = control
        self.core_manager = core_manager

        self.connect_signals()

    def connect_signals(self):
        QMetaObject.connectSlotsByName(self)

        # Explicit QueuedConnections
        self.main_window.splitter_horizontal_released.connect(
            self.on_horizontal_splitter_released, type=Qt.QueuedConnection
        )
        self.main_window.splitter_vertical_released.connect(
            self.on_vertical_splitter_released, type=Qt.QueuedConnection
        )
        self.main_window.window_resize_finished.connect(
            self.on_window_resize_finished, type=Qt.QueuedConnection
        )
        self.control.signal_add_10_nodes.connect(
            self.on_add_10_nodes, type=Qt.QueuedConnection
        )

    def on_horizontal_splitter_released(self):
        print("Horizontal splitter released")
        self.main_window.control.label_manager.adjust_font_to_width()
        self.main_window.control.button_manager.adjust_font_to_width()

    def on_vertical_splitter_released(self):
        print("Vertical splitter released")
        self.main_window.control.label_manager.adjust_font_to_width()
        self.main_window.control.button_manager.adjust_font_to_width()

    def on_window_resize_finished(self):
        print("Window resize finished")
        self.main_window.control.label_manager.adjust_font_to_width()
        self.main_window.control.button_manager.adjust_font_to_width()

    def _addNodes(self,nb_nodes):
        # The old approach (re-initialize map and edges) is replaced:
        # manager.initialize_map(new_total)
        # manager.create_default_edges()
        # self.main_window.graph_canvas.scene._render_nodes()  # re-render scene
        manager = self.main_window.graph_canvas.scene.manager
        # Now simply add nb_nodes new hops, do not reset or remove existing nodes/edges
        new_hops = manager.add_hops(nb_nodes)
        # Add these new nodes to the scene
        self.main_window.graph_canvas.scene.add_rendered_nodes(new_hops)
        # Finally export and update topology
        topology = manager.export_graph_topology()
        self.core_manager.update_topology(topology)


    def on_add_10_nodes(self):
        self._addNodes(10)

    def on_add_100_nodes(self):
        self._addNodes(100)
