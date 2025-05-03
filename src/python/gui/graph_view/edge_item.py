from qtpy.QtWidgets import QGraphicsLineItem
from qtpy.QtGui import QPen, QColor


class EdgeItem(QGraphicsLineItem):
    """
    Placeholder QGraphicsLineItem for representing edges.
    Currently unused until backend integration requires dynamic edges.
    """

    def __init__(self, x1, y1, x2, y2, parent=None):
        super().__init__(parent)
        self.setLine(x1, y1, x2, y2)
        self.setPen(QPen(QColor(0, 0, 0), 2))
