from qtpy.QtWidgets import QGraphicsLineItem
from qtpy.QtGui import QPen, QColor
from qtpy.QtCore import Qt

class EdgeItem(QGraphicsLineItem):
    """
    QGraphicsLineItem for representing an edge between two node positions.
    """

    def __init__(self, x1, y1, x2, y2, color=Qt.black, pen_width=2, parent=None):
        super().__init__(parent)
        self.setLine(x1, y1, x2, y2)
        pen = QPen(QColor(color))
        pen.setWidth(pen_width)
        self.setPen(pen)
