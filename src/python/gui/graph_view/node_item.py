# src/python/gui/graph_view/node_item.py

from qtpy.QtWidgets import QGraphicsEllipseItem, QGraphicsTextItem
from qtpy.QtGui import QBrush, QPen, QColor
from qtpy.QtCore import QRectF, Qt

class NodeItem(QGraphicsEllipseItem):
    """
    Basic circular representation of a node in the graph, with a label displayed below.
    """

    def __init__(self, x, y, radius=15, color=(100, 150, 200), label="", node_id=0, delay_ms=10):
        super().__init__()

        # Node position and appearance
        self.x = x
        self.y = y
        self.radius = radius

        # Unique ID and latency
        self.node_id = node_id
        self.delay_ms = delay_ms

        # Convert color to a valid QColor
        if isinstance(color, tuple):
            brush_color = QColor(*color)  # expands (r, g, b)
        else:
            brush_color = QColor(color)   # handles string like "#9FE2BF"

        self.setRect(QRectF(-radius, -radius, 2 * radius, 2 * radius))
        self.setBrush(QBrush(brush_color))
        self.setPen(QPen(Qt.black, 2))

        # Move the node to the desired position
        self.setPos(x, y)

        # Create and position the text label
        self.label_item = QGraphicsTextItem(label, self)
        self.label_item.setDefaultTextColor(Qt.black)

        bounding_rect = self.label_item.boundingRect()
        label_x = -bounding_rect.width() / 2
        label_y = radius + 5
        self.label_item.setPos(label_x, label_y)
