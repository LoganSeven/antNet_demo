import random
from qtpy.QtWidgets import QGraphicsScene
from qtpy.QtCore import QRectF
from .node_item import NodeItem


class GraphScene(QGraphicsScene):
    """
    QGraphicsScene that manages node creation and display. 
    By default, it places the start node on the left border,
    the end node on the right border, and randomizes other positions.
    """

    def __init__(self, width=1000, height=600, node_count=16, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)
        self.node_count = node_count
        self.node_items = []
        self.start_node = None
        self.end_node = None

        self._create_nodes()

    def _create_nodes(self):
        """
        Creates start node, end node, and other nodes with random distribution.
        """
        margin = 50
        mid_y = self.sceneRect().height() / 2

        # Create start node (vertically centered left)
        start_x = margin
        start_y = mid_y
        self.start_node = NodeItem(start_x, start_y)
        self.addItem(self.start_node)
        self.node_items.append(self.start_node)

        # Create end node (vertically centered right)
        end_x = self.sceneRect().width() - margin
        end_y = mid_y
        self.end_node = NodeItem(end_x, end_y)
        self.addItem(self.end_node)
        self.node_items.append(self.end_node)

        # Create the remaining nodes in random positions
        for _ in range(self.node_count - 2):
            x = random.gauss(self.sceneRect().width() / 2, self.sceneRect().width() / 6)
            y = random.gauss(self.sceneRect().height() / 2, self.sceneRect().height() / 6)

            # Clip positions to avoid placing nodes outside margins
            x = max(margin * 2, min(x, self.sceneRect().width() - margin * 2))
            y = max(margin, min(y, self.sceneRect().height() - margin))

            node = NodeItem(x, y)
            self.addItem(node)
            self.node_items.append(node)
