# src/python/gui/graph_view/edge_item.py

from qtpy.QtWidgets import QGraphicsLineItem
from qtpy.QtGui import QPen, QColor
from qtpy.QtCore import Qt

class EdgeItem(QGraphicsLineItem):
    """
    EdgeItem: visually represents a directed connection from one node to another.
    """

    def __init__(self, x1, y1, x2, y2, color=Qt.black, pen_width=2, from_node=None, to_node=None):
        super().__init__(x1, y1, x2, y2)
        self.from_node = from_node
        self.to_node   = to_node

        pen = QPen(QColor(color))
        pen.setWidth(pen_width)
        self.setPen(pen)
