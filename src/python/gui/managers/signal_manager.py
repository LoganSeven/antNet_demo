# gui/managers/signal_manager.py

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
