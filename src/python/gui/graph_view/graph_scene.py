# src/python/gui/graph_view/graph_scene.py

import random
from qtpy.QtWidgets import QGraphicsScene
from qtpy.QtCore import QRectF, Qt
from .node_item import NodeItem
from .edge_item import EdgeItem

class GraphScene(QGraphicsScene):
    """
    QGraphicsScene that manages start node, end node, and a list of hop nodes separately.
    Spacing is enforced so that hop nodes do not overlap.
    """

    def __init__(self, width=1000, height=600, total_nodes=16, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)

        self.total_nodes = total_nodes
        self.start_node = None
        self.end_node = None
        self.hop_nodes = []
        self.best_path_edges = []

        # Maintain a list of all EdgeItem objects
        self.edges = []

        self._create_nodes()

    def _create_nodes(self):
        """
        Creates one start node, one end node, and the remaining (total_nodes - 2) hops.
        """
        margin = 50
        radius = 15
        mid_y = self.sceneRect().height() / 2

        # Create start node
        start_x = margin
        start_y = mid_y
        self.start_node = NodeItem(
            x=start_x,
            y=start_y,
            radius=radius,
            color="#9FE2BF",
            label="Client endpoint",
            node_id=0,     # For demonstration, ID = 0
            delay_ms=10    # Example default delay
        )
        self.addItem(self.start_node)

        # Create end node
        end_x = self.sceneRect().width() - margin
        end_y = mid_y
        self.end_node = NodeItem(
            x=end_x,
            y=end_y,
            radius=radius,
            color="#FFBF00",
            label="Server endpoint",
            node_id=1,
            delay_ms=10
        )
        self.addItem(self.end_node)

        # Create hop nodes
        hop_count = self.total_nodes - 2
        for i in range(hop_count):
            node_id = i + 2
            node = self._place_random_hop(
                label=f"hop #{i}",
                color=(100, 150, 200),
                radius=radius,
                margin=margin,
                node_id=node_id,
                delay_ms=10
            )
            self.hop_nodes.append(node)
            self.addItem(node)

    def _place_random_hop(self, label, color, radius, margin, node_id, delay_ms):
        """
        Randomly place a single hop node, ensuring it does not overlap with existing hops.
        """
        max_tries = 100
        spacing = radius * 2 + 20

        for _ in range(max_tries):
            x = random.gauss(self.sceneRect().width() / 2, self.sceneRect().width() / 6)
            y = random.gauss(self.sceneRect().height() / 2, self.sceneRect().height() / 6)

            x = max(margin, min(x, self.sceneRect().width() - margin))
            y = max(margin, min(y, self.sceneRect().height() - margin))

            if self._no_overlap_with_hops(x, y, spacing):
                return NodeItem(x, y, radius, color=color, label=label, node_id=node_id, delay_ms=delay_ms)

        # Fallback if no placement found
        fallback_x = self.sceneRect().width() / 2
        fallback_y = self.sceneRect().height() / 2
        return NodeItem(fallback_x, fallback_y, radius, color=color, label=label, node_id=node_id, delay_ms=delay_ms)

    def _no_overlap_with_hops(self, x, y, spacing):
        for hop_node in self.hop_nodes:
            dx = hop_node.x - x
            dy = hop_node.y - y
            if dx*dx + dy*dy < spacing*spacing:
                return False
        return True

    def draw_best_path_by_hop_indices(self, hop_indices):
        """
        Draws edges from start_node through hops to end_node.
        Clears any previously drawn path edges.
        """
        # Remove old edges
        for edge in self.best_path_edges:
            self.removeItem(edge)
        self.best_path_edges.clear()

        nodes = [self.hop_nodes[i] for i in hop_indices if 0 <= i < len(self.hop_nodes)]

        if not nodes:
            edge = EdgeItem(
                self.start_node.x, self.start_node.y,
                self.end_node.x, self.end_node.y,
                color=Qt.red, pen_width=3
            )
            self.addItem(edge)
            self.best_path_edges.append(edge)
            return

        def dist(a, b): return (a.x - b.x)**2 + (a.y - b.y)**2

        start_hop = min(nodes, key=lambda n: dist(n, self.start_node))
        end_hop   = min(nodes, key=lambda n: dist(n, self.end_node))
        middle = [n for n in nodes if n not in (start_hop, end_hop)]

        sequence = [self.start_node, start_hop] + middle + [end_hop, self.end_node]

        for a, b in zip(sequence, sequence[1:]):
            edge = EdgeItem(a.x, a.y, b.x, b.y, color=Qt.red, pen_width=3)
            self.addItem(edge)
            self.best_path_edges.append(edge)

    def add_edge(self, from_node, to_node, color=Qt.black, pen_width=2):
        """
        Creates an EdgeItem from from_node to to_node, stores it in self.edges.
        """
        edge = EdgeItem(
            from_node.x, from_node.y, 
            to_node.x, to_node.y,
            color=color,
            pen_width=pen_width,
            from_node=from_node,
            to_node=to_node
        )
        self.edges.append(edge)
        self.addItem(edge)

    def export_graph_topology(self):
        """
        Returns a dict containing all node info and edges.
        {
          "nodes": [
            { "node_id": int, "delay_ms": int },
            ...
          ],
          "edges": [
            { "from_id": int, "to_id": int },
            ...
          ]
        }
        """
        node_list = [self.start_node] + self.hop_nodes + [self.end_node]
        nodes_data = []
        for n in node_list:
            node_info = {
                "node_id": n.node_id,
                "delay_ms": n.delay_ms
            }
            nodes_data.append(node_info)

        edges_data = []
        for e in self.edges:
            if e.from_node and e.to_node:
                edges_data.append({
                    "from_id": e.from_node.node_id,
                    "to_id": e.to_node.node_id
                })

        return {
            "nodes": nodes_data,
            "edges": edges_data
        }
