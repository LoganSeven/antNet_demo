from qtpy.QtWidgets import QGraphicsEllipseItem
from qtpy.QtGui import QBrush, QPen, QColor
from qtpy.QtCore import QRectF, Qt


class NodeItem(QGraphicsEllipseItem):
    """
    Basic circular representation of a node in the graph.
    """

    def __init__(self, x, y, radius=15, parent=None):
        super().__init__(parent)
        self.setRect(QRectF(-radius, -radius, 2 * radius, 2 * radius))
        self.setBrush(QBrush(QColor(100, 150, 200)))
        self.setPen(QPen(Qt.black, 2))
        self.setPos(x, y)
