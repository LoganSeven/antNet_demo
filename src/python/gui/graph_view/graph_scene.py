# src/python/gui/graph_view/graph_scene.py
"""
GraphScene is responsible for visualizing the network graph (nodes and edges).
It delegates topology structure to HopMapManager and focuses on Qt-based rendering.
"""

import random
from qtpy.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from qtpy.QtCore import Qt
from .node_item import NodeItem
from .edge_item import EdgeItem

from gui.consts.gui_consts import ALGO_COLORS
from gui.managers.hop_map_manager import HopMapManager

# For the heatmap background:
from .heatmap_generator import generate_heatmap

class GraphScene(QGraphicsScene):
    """
    GraphScene manages Qt-based drawing of nodes and edges.
    The actual data (how many nodes, which edges, etc.) comes from HopMapManager.
    The 'init_scene_with_nodes(...)' call triggers the creation and rendering
    of the node topology with a desired count.
    """

    def __init__(self, width=1000, height=600, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)

        # Source-of-truth for nodes/edges is here:
        self.manager = HopMapManager()

        # Keep track of rendered NodeItems
        self._node_items_by_id = {}

        # List of rendered edges for solver paths (to remove/redraw them each iteration)
        self.best_path_edges = []

        # Optional list of static edges (e.g. create_default_edges)
        self.static_edges = []

        # Optional heatmap item for background
        self._heatmap_item = None

    def init_scene_with_nodes(self, total_nodes: int):
        """
        Initializes the HopMapManager with 'total_nodes' and renders them.
        Call this once you know how many nodes you want (e.g., from .ini config).
        """
        # Create new map data
        self.manager.initialize_map(total_nodes)
        # Render them in the scene
        self._render_nodes()

    def create_default_edges(self):
        """
        Creates a default path in manager; call after nodes have been initialized.
        Then call render_manager_edges() if you want these edges visible on screen.
        """
        self.manager.create_default_edges()

    def render_manager_edges(self):
        """
        Renders edges from self.manager.edges_data as static lines in the scene.
        This is optional, useful for debugging or displaying a basic chain.
        """
        # Clear existing static edges from the scene
        for e_item in self.static_edges:
            try:
                self.removeItem(e_item)
            except RuntimeError:
                pass
        self.static_edges.clear()

        for edge_data in self.manager.edges_data:
            from_id = edge_data["from_id"]
            to_id   = edge_data["to_id"]
            from_node = self._node_items_by_id.get(from_id)
            to_node   = self._node_items_by_id.get(to_id)

            if not from_node or not to_node:
                continue  # Possibly invalid reference

            x1, y1 = from_node.x, from_node.y
            x2, y2 = to_node.x, to_node.y
            edge_item = EdgeItem(x1, y1, x2, y2, color="#AAAAAA", pen_width=2)
            self.addItem(edge_item)
            self.static_edges.append(edge_item)

    def export_graph_topology(self):
        """
        Exports current node and edge data to a backend-serializable format.
        """
        return self.manager.export_graph_topology()

    def _render_nodes(self):
        """
        Clears all old items, then creates NodeItems for each manager node.
        This does NOT create edges – call create_default_edges() or use
        render_manager_edges() for visual edges.
        """
        # Remove existing items from the scene
        self.best_path_edges.clear()
        self.static_edges.clear()
        self._node_items_by_id.clear()
        self.clear()

        # 1) Render start node
        if self.manager.start_node_data:
            data = self.manager.start_node_data
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

        # 2) Render hop nodes
        for data in self.manager.hop_nodes_data:
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

        # 3) Render end node
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
        Draws solver paths using color-coded lines.
        If a path is empty or invalid, it is skipped.
        Each path is offset slightly so they don't overlap each other.
        """
        # Safely remove old path edges
        for e in self.best_path_edges:
            try:
                self.removeItem(e)
            except RuntimeError:
                pass

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

            # Draw lines between consecutive nodes
            for a, b in zip(seq, seq[1:]):
                x1, y1 = a.x + ox, a.y + oy
                x2, y2 = b.x + ox, b.y + oy
                edge = EdgeItem(x1, y1, x2, y2, color=pen_color, pen_width=3)
                self.addItem(edge)
                self.best_path_edges.append(edge)

    def update_heatmap(self, pheromone_matrix):
        """
        Converts the given pheromone matrix into a QPixmap and displays it as
        a background item behind all nodes and edges. This uses generate_heatmap
        from heatmap_generator.py, rendering one square under each node.
        """
        if not pheromone_matrix:
            if self._heatmap_item:
                self.removeItem(self._heatmap_item)
                self._heatmap_item = None
            return

        node_positions = self._ordered_node_positions()

        pixmap = generate_heatmap(
            pheromone_matrix,
            node_positions=node_positions,
            size_factor=1.25
        )
        if pixmap.isNull():
            return

        if self._heatmap_item:
            try:
                self.removeItem(self._heatmap_item)
            except RuntimeError:
                pass
            self._heatmap_item = None

        self._heatmap_item = QGraphicsPixmapItem(pixmap)
        self._heatmap_item.setZValue(-9999)
        self.addItem(self._heatmap_item)

    def _ordered_node_positions(self):
        """
        Returns a list 'pos' where pos[i] = (x, y) of node with id i.
        Length = (max_node_id + 1). Missing nodes yield (0.0, 0.0).
        """
        all_nodes = []
        if self.manager.start_node_data:
            all_nodes.append(self.manager.start_node_data)
        all_nodes.extend(self.manager.hop_nodes_data)
        if self.manager.end_node_data:
            all_nodes.append(self.manager.end_node_data)

        if not all_nodes:
            return []

        max_id = max(nd["node_id"] for nd in all_nodes)
        positions = [(0.0, 0.0)] * (max_id + 1)
        for nd in all_nodes:
            positions[nd["node_id"]] = (nd["x"], nd["y"])
        return positions
