# src/python/gui/graph_view/graph_scene.py

import random
from qtpy.QtWidgets import QGraphicsScene
from qtpy.QtCore import Qt
from .node_item import NodeItem
from .edge_item import EdgeItem

from gui.consts.gui_consts import ALGO_COLORS
from gui.managers.hop_map_manager import HopMapManager

class GraphScene(QGraphicsScene):
    """
    GraphScene is responsible for visualizing the network graph (nodes and edges).
    It delegates topology structure to HopMapManager and focuses on Qt-based rendering.
    """

    def __init__(self, width=1000, height=600, total_nodes=16, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)

        # Source-of-truth for nodes and edges
        self.manager = HopMapManager()
        self.manager.initialize_map(total_nodes)

        # Internal map of rendered NodeItems
        self._node_items_by_id = {}

        # List of rendered edges for current solver paths
        self.best_path_edges = []

        # Render the initial topology
        self._render_nodes()

    def create_default_edges(self):
        """
        Creates default edge path in manager, used for initial solver runs.
        """
        self.manager.create_default_edges()

    def export_graph_topology(self):
        """
        Exports current node and edge data to a backend-serializable format.
        """
        return self.manager.export_graph_topology()

    def _render_nodes(self):
        """
        Clears and recreates all node items based on HopMapManager state.
        """
        # Clear references to deleted items to avoid invalid memory access
        self.best_path_edges.clear()
        self._node_items_by_id.clear()
        self.clear()

        # Render start node
        if self.manager.start_node_data:
            data = self.manager.start_node_data
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

        # Render hop nodes
        for data in self.manager.hop_nodes_data:
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

        # Render end node
        if self.manager.end_node_data:
            data = self.manager.end_node_data
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

    def add_rendered_nodes(self, new_hops: list[dict]):
        """
        Creates NodeItem for each new hop, adds to scene, and updates _node_items_by_id
        without resetting existing nodes or edges.
        """
        for data in new_hops:
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

    def draw_multiple_paths(self, path_dict):
        """
        Draws solver paths using color-coded, slightly offset lines.
        If a path is empty or invalid, it is skipped.
        """
        # Attempt to remove each item safely (in case it was deleted already)
        for e in self.best_path_edges:
            try:
                self.removeItem(e)
            except RuntimeError:
                pass  # EdgeItem already deleted (e.g., after scene.clear())

        self.best_path_edges.clear()

        color_map = {
            "aco":    ALGO_COLORS["aco"],
            "random": ALGO_COLORS["random"],
            "brute":  ALGO_COLORS["brute"]
        }
        offset_map = {
            "aco":    (0, 0),
            "random": (6, -6),
            "brute":  (-6, 6)
        }

        for algo, info in path_dict.items():
            if "nodes" not in info:
                continue
            node_ids = info["nodes"]
            if not node_ids:
                continue

            ox, oy = offset_map.get(algo, (0, 0))
            pen_color = color_map.get(algo, "#000000")

            # Rebuild node path from known IDs
            seq = []
            for nid in node_ids:
                node = self._node_items_by_id.get(nid)
                if node:
                    seq.append(node)

            # Draw path as lines between consecutive nodes
            for a, b in zip(seq, seq[1:]):
                x1, y1 = a.x + ox, a.y + oy
                x2, y2 = b.x + ox, b.y + oy
                edge = EdgeItem(x1, y1, x2, y2, color=pen_color, pen_width=3)
                self.addItem(edge)
                self.best_path_edges.append(edge)
