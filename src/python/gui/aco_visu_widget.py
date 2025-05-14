# src/python/gui/aco_visu_widget.py

import sys
from qtpy.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QTextEdit, QScrollBar, QApplication
from qtpy.QtCore import Qt, QTimer, QEvent
from qtpy.QtGui import QColor, QTextCharFormat

import pyqtgraph as pg
from pyqtgraph import PlotWidget
import numpy as np

from gui.consts.gui_consts import ALGO_COLORS


class AcoVisuWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        print("Initializing AcoVisuWidget")

        main_layout = QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        # Always use 2D PlotWidget
        self.graph_widget = PlotWidget()
        self.graph_widget.setBackground('w')
        self.graph_widget.showGrid(x=True, y=True, alpha=0.3)
        self.graph_widget.getAxis('left').setPen(pg.mkPen(color='k'))
        self.graph_widget.getAxis('bottom').setPen(pg.mkPen(color='k'))
        self.graph_widget.setMinimumWidth(200)

        self.text_area = QTextEdit()
        self.text_area.setReadOnly(True)
        self.text_area.setMinimumWidth(100)

        self.scrollbar = self.text_area.verticalScrollBar()
        self.scrollbar.valueChanged.connect(self._on_scrollbar_value_changed)
        self.scrollbar.installEventFilter(self)

        main_layout.addWidget(self.graph_widget, 2)
        main_layout.addWidget(self.text_area, 1)
        self.setLayout(main_layout)

        self.curve_brute_force = None
        self.curve_random = None
        self.curve_performance_aco = None
        self.curve_performance_brute = None
        self.curve_performance_random = None

        self.log_buffer = []

        self._auto_scroll_enabled = True
        self._mouse_pressed = False

        self._drag_timer = QTimer()
        self._drag_timer.setSingleShot(True)
        self._drag_timer.timeout.connect(self._reactivate_autoscroll_after_drag)

        self._wheel_timer = QTimer()
        self._wheel_timer.setSingleShot(True)
        self._wheel_timer.timeout.connect(self._reactivate_autoscroll_after_wheel)

    def eventFilter(self, obj, event):
        if obj == self.scrollbar:
            if event.type() == QEvent.MouseButtonPress and event.button() == Qt.LeftButton:
                self._mouse_pressed = True
                self._auto_scroll_enabled = False
            elif event.type() == QEvent.MouseButtonRelease and event.button() == Qt.LeftButton:
                self._mouse_pressed = False
                self._drag_timer.start(250)
        return super().eventFilter(obj, event)

    def _on_scrollbar_value_changed(self):
        if not self._mouse_pressed:
            self._auto_scroll_enabled = False
            self._wheel_timer.start(1000)

    def _reactivate_autoscroll_after_drag(self):
        if not self._mouse_pressed:
            self._auto_scroll_enabled = True

    def _reactivate_autoscroll_after_wheel(self):
        if not self._mouse_pressed:
            self._auto_scroll_enabled = True

    def draw_demo_graph(self):
        print("Drawing demo graph...")

        self.graph_widget.clear()

        node_counts = np.arange(5, 55, 5)
        print(f"Node counts: {node_counts}")

        aco_times = np.log(node_counts) * 10 + np.random.rand(len(node_counts)) * 5
        brute_times = (node_counts ** 2) * 0.5 + np.random.rand(len(node_counts)) * 10
        rand_times = np.random.rand(len(node_counts)) * 50 + 50

        print(f"ACO times: {aco_times}")
        print(f"Brute Force times: {brute_times}")
        print(f"Random times: {rand_times}")

        self.curve_performance_aco = self.graph_widget.plot(
            node_counts,
            aco_times,
            pen=pg.mkPen(ALGO_COLORS["aco"], width=2),
            name="ACO"
        )

        self.curve_performance_brute = self.graph_widget.plot(
            node_counts,
            brute_times,
            pen=pg.mkPen(ALGO_COLORS["brute"], width=2, style=Qt.DashLine),
            name="Brute Force"
        )

        self.curve_performance_random = self.graph_widget.plot(
            node_counts,
            rand_times,
            pen=pg.mkPen(ALGO_COLORS["random"], width=2, style=Qt.DotLine),
            name="Random Pick"
        )

        self.graph_widget.setLabel('bottom', 'Number of Nodes')
        self.graph_widget.setLabel('left', 'Execution Time (ms)')
        self.graph_widget.addLegend()
        self.text_area.setPlainText("Demo performance graph: Execution time vs Number of nodes.")
        print("Demo graph rendered.")

    def addLog(self, message: str, color):
        try:
            qcolor = QColor(color)
            if not qcolor.isValid():
                raise ValueError("Invalid color string")
        except Exception as e:
            print(f"Invalid color string: {color}, error: {e}")
            return

        cursor = self.text_area.textCursor()
        cursor.movePosition(cursor.End)

        fmt = QTextCharFormat()
        fmt.setForeground(qcolor)
        cursor.setCharFormat(fmt)
        cursor.insertText(message + "\n")

        self.text_area.setTextColor(QColor("black"))
        self.log_buffer.append((message, qcolor.name()))

        if self._auto_scroll_enabled or self.text_area.verticalScrollBar().value() == self.text_area.verticalScrollBar().maximum():
            self.text_area.moveCursor(cursor.End)

    def clearLog(self):
        self.text_area.clear()
        self.log_buffer.clear()

    def reloadLog(self):
        self.text_area.clear()
        cursor = self.text_area.textCursor()
