# Relative Path: src/python/gui/graph_view/node_item.py
"""
Defines NodeItem, a moveable QGraphicsEllipseItem with an optional label.
Tracks x,y in itemChange to keep internal references and connected edges in sync.
Mainly responsible for visually representing a single node with configurable delay_ms.
"""

from qtpy.QtWidgets import QGraphicsEllipseItem, QGraphicsTextItem, QGraphicsItem
from qtpy.QtGui import QBrush, QPen, QColor
from qtpy.QtCore import QRectF, Qt

class NodeItem(QGraphicsEllipseItem):
    """
    Basic circular representation of a node in the graph, with a label displayed below.
    This itemChange override also tracks self.x and self.y for consistent usage
    (e.g., EdgeItem endpoints), and re-positions an internal label_item if present.
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
            brush_color = QColor(color)   # handles a string like "#9FE2BF"

        self.setRect(QRectF(-radius, -radius, 2 * radius, 2 * radius))
        self.setBrush(QBrush(brush_color))
        self.setPen(QPen(Qt.black, 2))

        # Move the node to the desired position
        self.setPos(self.x, self.y)

        # Optional single-line label as a child (used by some node types)
        self.label_item = None
        if label:
            self.label_item = QGraphicsTextItem(label, self)
            self.label_item.setDefaultTextColor(Qt.black)
            bounding_rect = self.label_item.boundingRect()
            label_x = -bounding_rect.width() / 2
            label_y = radius + 5
            self.label_item.setPos(label_x, label_y)

        # By default, accept position changes so itemChange(...) is triggered
        self.setFlag(QGraphicsItem.ItemIsMovable, False)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges, True)

    def itemChange(self, change, value):
        """
        Overriding itemChange(...) to keep label aligned and x,y updated if the node moves.
        """
        if change == QGraphicsItem.ItemPositionHasChanged:
            new_pos = value
            self.x = new_pos.x()
            self.y = new_pos.y()

            # If we have an internal single-line label_item, keep it below the node center
            if self.label_item:
                bounding_rect = self.label_item.boundingRect()
                label_x = -bounding_rect.width() / 2
                label_y = self.radius + 5
                self.label_item.setPos(label_x, label_y)

        return super().itemChange(change, value)
