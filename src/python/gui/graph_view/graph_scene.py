# Relative Path: src/python/gui/graph_view/graph_scene.py
"""
GraphScene is responsible for visualizing the network graph (nodes and edges).
It delegates topology structure to HopMapManager and focuses on Qt-based rendering.
"""

import random
from qtpy.QtWidgets import QGraphicsScene, QGraphicsPixmapItem, QGraphicsTextItem, QGraphicsRectItem
from qtpy.QtGui import QBrush, QFont, QColor, QPen
from qtpy.QtCore import Qt, QRectF
from .node_item import NodeItem
from .edge_item import EdgeItem

from gui.consts.gui_consts import ALGO_COLORS
from gui.managers.hop_map_manager import HopMapManager
from gui.graph_view.heatmap_generator import *


class GraphScene(QGraphicsScene):
    def __init__(self, width=1000, height=600, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)
        self.manager = HopMapManager()
        self._node_items_by_id = {}
        self.best_path_edges = []
        self.static_edges = []
        self._heatmap_item = None
        self._gpu_is_ok = False

    def set_gpu_ok(self, is_ok):
        self._gpu_is_ok = is_ok

    def init_scene_with_nodes(self, total_nodes: int):
        self.manager.initialize_map(total_nodes)
        self._render_nodes()

    def create_default_edges(self):
        self.manager.create_default_edges()

    def render_manager_edges(self):
        for e_item in self.static_edges:
            try:
                self.removeItem(e_item)
            except RuntimeError:
                pass
        self.static_edges.clear()

        for edge_data in self.manager.edges_data:
            from_node = self._node_items_by_id.get(edge_data["from_id"])
            to_node = self._node_items_by_id.get(edge_data["to_id"])
            if not from_node or not to_node:
                continue
            x1, y1 = from_node.x, from_node.y
            x2, y2 = to_node.x, to_node.y
            edge_item = EdgeItem(x1, y1, x2, y2, color="#000000FF", pen_width=2)
            self.addItem(edge_item)
            self.static_edges.append(edge_item)

    def export_graph_topology(self):
        return self.manager.export_graph_topology()

    def _add_centered_text_block(self, x, y, line1: str, line2: str, radius: float):
        """
        Draws a two-line centered block (label + delay) inside the circle of given radius.
        The vertical center of both lines matches the circle center, with compact spacing.
        """
        font1 = QFont("Consolas", 7)
        font1.setBold(False)
        font2 = QFont("Consolas", 7)
        font2.setBold(False)

        text_item_1 = QGraphicsTextItem(line1)
        text_item_1.setFont(font1)
        text_item_1.setDefaultTextColor(Qt.white)
        text_item_1.setZValue(10)
        rect1 = text_item_1.boundingRect()

        text_item_2 = QGraphicsTextItem(line2)
        text_item_2.setFont(font2)
        text_item_2.setDefaultTextColor(Qt.white)
        text_item_2.setZValue(10)
        rect2 = text_item_2.boundingRect()

        # Reduced spacing between lines
        spacing_adjustment = -9.0
        total_height = rect1.height() + rect2.height() + spacing_adjustment
        start_y = y - (total_height / 2)

        text_item_1.setPos(x - rect1.width() / 2, start_y)
        text_item_2.setPos(x - rect2.width() / 2, start_y + rect1.height() + spacing_adjustment)

        text_item_1.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)
        text_item_2.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)

        self.addItem(text_item_1)
        self.addItem(text_item_2)


    def _add_background_label(self, x, y, text, bg_color="#f5f5cc", text_color=Qt.black):
        pass
        rect = QRectF(x - 25, y, 50, 18)
        bg = QGraphicsRectItem(rect)
        bg.setBrush(QBrush(QColor(bg_color)))
        bg.setPen(QPen(Qt.NoPen))
        bg.setZValue(5)
        self.addItem(bg)

        label = QGraphicsTextItem(text)
        font = QFont("Consolas", 8)
        font.setBold(False)
        label.setFont(font)
        label.setDefaultTextColor(text_color)
        label.setZValue(6)

        label_bounds = label.boundingRect()
        label.setPos(x - label_bounds.width() / 2, y + 1)
        label.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)
        self.addItem(label)

    def _render_nodes(self):
        self.best_path_edges.clear()
        self.static_edges.clear()
        self._node_items_by_id.clear()
        self.clear()

        if self.manager.start_node_data:
            data = self.manager.start_node_data
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item
            self._add_background_label(data["x"], data["y"] + data["radius"] + 4, "Client")

        for data in self.manager.hop_nodes_data:
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

            self._add_centered_text_block(
                x=data["x"],
                y=data["y"],
                line1=data["label"],
                line2=f"{data['delay_ms']} ms",
                radius=data["radius"]
            )

        if self.manager.end_node_data:
            data = self.manager.end_node_data
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item
            self._add_background_label(data["x"], data["y"] + data["radius"] + 4, "Server")

    def add_rendered_nodes(self, new_hops: list[dict]):
        for data in new_hops:
            item = NodeItem(
                x=data["x"], y=data["y"], radius=data["radius"],
                color=data["color"], label=data["label"],
                node_id=data["node_id"], delay_ms=data["delay_ms"]
            )
            self.addItem(item)
            self._node_items_by_id[data["node_id"]] = item

            self._add_centered_text_block(
                x=data["x"],
                y=data["y"],
                line1=data["label"],
                line2=f"{data['delay_ms']} ms",
                radius=data["radius"]
            )

    def draw_multiple_paths(self, path_dict):
        for e in self.best_path_edges:
            try:
                self.removeItem(e)
            except RuntimeError:
                pass
        self.best_path_edges.clear()

        color_map = {
            "aco": ALGO_COLORS["aco"],
            "random": ALGO_COLORS["random"],
            "brute": ALGO_COLORS["brute"]
        }
        offset_map = {
            "aco": (0, 0),
            "random": (6, -6),
            "brute": (-6, 6)
        }

        for algo, info in path_dict.items():
            node_ids = info.get("nodes", [])
            if not node_ids:
                continue
            ox, oy = offset_map.get(algo, (0, 0))
            pen_color = color_map.get(algo, "#000000")
            seq = [self._node_items_by_id.get(nid) for nid in node_ids if self._node_items_by_id.get(nid)]
            for a, b in zip(seq, seq[1:]):
                x1, y1 = a.x + ox, a.y + oy
                x2, y2 = b.x + ox, b.y + oy
                edge = EdgeItem(x1, y1, x2, y2, color=pen_color, pen_width=3)
                self.addItem(edge)
                self.best_path_edges.append(edge)

    def update_heatmap(self, pheromone_matrix):
        if not pheromone_matrix:
            return

        node_positions = self._ordered_node_positions()
        pixmap = None

        if self._gpu_is_ok:
            try:
                from gui.graph_view.heatmap_generator import generate_heatmap_gl
                scene_w = int(self.sceneRect().width())
                scene_h = int(self.sceneRect().height())
                pixmap = generate_heatmap_gl(
                    pheromone_matrix, node_positions=node_positions,
                    width=scene_w, height=scene_h
                )
            except Exception as e:
                print(f"[GraphScene] GPU heatmap rendering failed: {e}")
                pixmap = None

        if pixmap is None or pixmap.isNull():
            from gui.graph_view.heatmap_generator import generate_heatmap
            pixmap = generate_heatmap(pheromone_matrix, node_positions=node_positions, size_factor=1.25)

        if pixmap is None or pixmap.isNull():
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
