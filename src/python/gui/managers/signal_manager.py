# Relative Path: src/python/gui/managers/signal_manager.py
"""
Centralizes signal and slot connections among MainWindow, ControlWidget, and CoreManager.
Ensures queued connections for safe cross-thread communication.
Simplifies UI logic by collecting all event wiring in one place.
"""


from __future__ import annotations
from typing import TYPE_CHECKING
from qtpy.QtCore import QObject, Qt, QMetaObject

if TYPE_CHECKING:
    from gui.main_window import MainWindow
    from gui.widgets.control_widget import ControlWidget
    from core.core_manager import CoreManager


class SignalManager(QObject):
    """
    Central manager that connects signals from MainWindow, ControlWidget,
    and other GUI components to their respective slots or methods.

    Ensures all connections are declared in one place, facilitating
    thread-safe queued connections where necessary.
    """

    def __init__(self, main_window: MainWindow, control: ControlWidget, core_manager: CoreManager):
        super().__init__()
        self.main_window = main_window
        self.control = control
        self.core_manager = core_manager

        self.connect_signals()

    def connect_signals(self):
        # Qt built-in method: attempts to auto-connect based on objectName + slotName
        QMetaObject.connectSlotsByName(self)

        # Queued connections for certain signals from MainWindow
        self.main_window.splitter_horizontal_released.connect(
            self.on_horizontal_splitter_released, type=Qt.QueuedConnection
        )
        self.main_window.splitter_vertical_released.connect(
            self.on_vertical_splitter_released, type=Qt.QueuedConnection
        )
        self.main_window.window_resize_finished.connect(
            self.on_window_resize_finished, type=Qt.QueuedConnection
        )

        # Connect ControlWidget's existing signal for adding 10 nodes
        self.control.signal_add_10_nodes.connect(
            self.on_add_10_nodes, type=Qt.QueuedConnection
        )

        # Move MainWindow button signals here
        self.main_window.button_stop.clicked.connect(self.core_manager.stop)
        self.main_window.button_update_topology.clicked.connect(self.main_window.on_button_update_topology)

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

    def _addNodes(self, nb_nodes):
        """
        Adds nb_nodes new hops to the map using the underlying HopMapManager.
        Updates the topology in the backend after the new hops are appended.
        """
        manager = self.main_window.graph_canvas.scene.manager
        new_hops = manager.add_hops(nb_nodes)
        self.main_window.graph_canvas.scene.add_rendered_nodes(new_hops)
        topology = manager.export_graph_topology()
        self.core_manager.update_topology(topology)

    def on_add_10_nodes(self):
        """
        Handler for the 'Add 10 nodes' signal from ControlWidget.
        """
        self._addNodes(10)

    # Potential future usage (not yet connected in the UI):
    def on_add_100_nodes(self):
        self._addNodes(100)
