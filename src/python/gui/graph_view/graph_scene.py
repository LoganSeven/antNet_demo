import random
from qtpy.QtWidgets import QGraphicsScene
from qtpy.QtCore import QRectF
from .node_item import NodeItem
from .edge_item import EdgeItem
from qtpy.QtCore import Qt

class GraphScene(QGraphicsScene):
    """
    QGraphicsScene that manages start node, end node, and a list of hop nodes separately.
    Spacing is enforced so that hop nodes do not overlap. 
    """

    def __init__(self, width=1000, height=600, total_nodes=16, parent=None):
        """
        total_nodes includes start node, end node, and all hops.
        For example, if total_nodes=16, that means 1 start, 1 end, and 14 hops.
        """
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)

        # Basic definitions
        self.total_nodes = total_nodes
        self.start_node = None
        self.end_node = None
        self.hop_nodes = []  # Only the middle hops

        # For drawing and clearing the best path
        self.best_path_edges = []

        self._create_nodes()

    def _create_nodes(self):
        """
        Creates one start node, one end node, and the remaining (total_nodes - 2) hops.
        """
        margin = 50
        radius = 15
        mid_y = self.sceneRect().height() / 2

        # Create start node (vertically centered left)
        start_x = margin
        start_y = mid_y
        self.start_node = NodeItem(
            x=start_x,
            y=start_y,
            radius=radius,
            color="#9FE2BF",  # first node color
            label="Client endpoint"
        )
        self.addItem(self.start_node)

        # Create end node (vertically centered right)
        end_x = self.sceneRect().width() - margin
        end_y = mid_y
        self.end_node = NodeItem(
            x=end_x,
            y=end_y,
            radius=radius,
            color="#FFBF00",  # last node color
            label="Server endpoint"
        )
        self.addItem(self.end_node)

        # Create hop nodes
        hop_count = self.total_nodes - 2
        for i in range(hop_count):
            node = self._place_random_hop(
                label=f"hop #{i}",  # zero-based labeling: hop #0, hop #1, etc.
                color=(100, 150, 200),
                radius=radius,
                margin=margin
            )
            self.hop_nodes.append(node)
            self.addItem(node)

    def _place_random_hop(self, label, color, radius, margin):
        """
        Randomly place a single hop node, ensuring it does not overlap with existing hops.
        Up to max_tries attempts. If all fail, place near the center.
        """
        max_tries = 100
        spacing = radius * 2 + 20

        for _ in range(max_tries):
            x = random.gauss(self.sceneRect().width() / 2, self.sceneRect().width() / 6)
            y = random.gauss(self.sceneRect().height() / 2, self.sceneRect().height() / 6)

            x = max(margin, min(x, self.sceneRect().width() - margin))
            y = max(margin, min(y, self.sceneRect().height() - margin))

            if self._no_overlap_with_hops(x, y, spacing):
                return NodeItem(x, y, radius, color=color, label=label)

        # Fallback if no placement found
        fallback_x = self.sceneRect().width() / 2
        fallback_y = self.sceneRect().height() / 2
        return NodeItem(fallback_x, fallback_y, radius, color=color, label=label)

    def _no_overlap_with_hops(self, x, y, spacing):
        """
        Checks spacing only against hop_nodes (start/end are typically spaced far away).
        """
        for hop_node in self.hop_nodes:
            dx = hop_node.x - x
            dy = hop_node.y - y
            dist_sq = dx * dx + dy * dy
            if dist_sq < (spacing * spacing):
                return False
        return True

    def draw_best_path_by_hop_indices(self, hop_indices):
        """
        Draws edges between consecutive hop indices from self.hop_nodes.
        Clears any previously drawn path edges.
        """
        # Remove old edges
        for edge in self.best_path_edges:
            self.removeItem(edge)
        self.best_path_edges.clear()

        if len(hop_indices) < 2:
            return

        # Connect consecutive pairs of hop indices
        for i in range(len(hop_indices) - 1):
            idx1 = hop_indices[i]
            idx2 = hop_indices[i + 1]

            # Validate that the hop index is in range
            if idx1 < 0 or idx1 >= len(self.hop_nodes):
                continue
            if idx2 < 0 or idx2 >= len(self.hop_nodes):
                continue

            x1 = self.hop_nodes[idx1].x
            y1 = self.hop_nodes[idx1].y
            x2 = self.hop_nodes[idx2].x
            y2 = self.hop_nodes[idx2].y

            edge = EdgeItem(x1, y1, x2, y2, color=Qt.red, pen_width=3)
            self.addItem(edge)
            self.best_path_edges.append(edge)
