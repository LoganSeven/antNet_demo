# Relative Path: src/python/gui/graph_view/edge_item.py
"""
EdgeItem visually represents a directed link between two NodeItems in the scene.
Updates position if the associated nodes move, maintaining correct line geometry.
Supports a cosmetic pen for color and width adjustments.
"""

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
        pen.setWidthF(float(pen_width))
        pen.setCapStyle(Qt.RoundCap)
        pen.setJoinStyle(Qt.RoundJoin)
        pen.setCosmetic(True)  # thickness remains constant when zooming
        self.setPen(pen)

        self.setZValue(-1)  # ensure it's behind nodes

    def updateLine(self):
        """
        updateLine: read from_node and to_node positions and update geometry accordingly.
        """
        if self.from_node and self.to_node:
            self.setLine(self.from_node.x, self.from_node.y,
                         self.to_node.x, self.to_node.y)
