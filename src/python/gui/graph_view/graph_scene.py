# Relative Path: src/python/gui/graph_view/graph_scene.py
"""
GraphScene is responsible for visualizing the network graph (nodes and edges).
It delegates topology structure to HopMapManager and focuses on Qt-based rendering.
"""

import random
from qtpy.QtWidgets import (
    QGraphicsScene,
    QGraphicsPixmapItem,
    QGraphicsTextItem,
    QGraphicsRectItem,
)
from qtpy.QtGui import QBrush, QFont, QColor, QPen
from qtpy.QtCore import Qt, QRectF

from .node_item import NodeItem
from .edge_item import EdgeItem
from gui.consts.gui_consts import ALGO_COLORS
from gui.managers.hop_map_manager import HopMapManager
from gui.graph_view.heatmap_generator import *


class GraphScene(QGraphicsScene):
    """
    GraphScene holds a HopMapManager for node/edge creation, but focuses on Qt-based
    rendering of node items, edges, and heatmap overlays. The HopMapManager is
    exposed as self.hop_map_manager so that external code can directly configure
    delay ranges or topology updates.
    """

    def __init__(self, width: int = 1000, height: int = 600, parent=None):
        super().__init__(parent)
        self.setSceneRect(0, 0, width, height)

        # Match glClearColor(0.02, 0.02, 0.1, 1.0)
        self.setBackgroundBrush(QColor.fromRgbF(0.02, 0.02, 0.1, 1.0))

        self.hop_map_manager = HopMapManager()

        # Caches
        self._node_items_by_id: dict[int, NodeItem] = {}
        self.best_path_edges:  list[EdgeItem]       = []
        self.static_edges:     list[EdgeItem]       = []

        self._bg_label_items_by_id:  dict[int, tuple] = {}  # start / end
        self._hop_label_items_by_id: dict[int, tuple] = {}  # hops

        self._heatmap_item: QGraphicsPixmapItem | None = None
        self._gpu_is_ok: bool = False

    def set_gpu_ok(self, is_ok: bool):
        """
        Informs GraphScene whether the OpenGL-based heatmap renderer is valid.
        If True, attempts to generate GPU-based heatmaps before falling back to CPU.
        """
        self._gpu_is_ok = is_ok

    def init_scene_with_nodes(self, total_nodes: int):
        """
        (Re)initialises HopMapManager with `total_nodes`, then renders nodes.
        """
        self.hop_map_manager.initialize_map(total_nodes)
        self._render_nodes()

    def recalc_node_positions(self, scene_width: float, scene_height: float):
        """
        Lays out nodes for the new scene size and updates QGraphicsItems.
        Heat-map overlay is invalidated so the next backend update repaints it
        at the correct resolution.
        """
        self.hop_map_manager.recalc_positions(scene_width, scene_height)
        self._apply_positions_to_items()
        self._invalidate_heatmap()

    def create_default_edges(self):
        """
        Calls hop_map_manager.create_default_edges() and defers final drawing
        to render_manager_edges().
        """
        self.hop_map_manager.create_default_edges()

    def render_manager_edges(self):
        """
        Clears & re-creates static (transparent) edges from the hop_map_manager data.
        """
        for e_item in self.static_edges:
            try:
                self.removeItem(e_item)
            except RuntimeError:
                pass
        self.static_edges.clear()

        # For each edge, we create a QGraphics item
        for edge_data in self.hop_map_manager.edges_data:
            from_node = self._node_items_by_id.get(edge_data["from_id"])
            to_node   = self._node_items_by_id.get(edge_data["to_id"])
            if not from_node or not to_node:
                continue
            edge_item = EdgeItem(
                from_node.x,
                from_node.y,
                to_node.x,
                to_node.y,
                color="#000000FF",
                pen_width=2,
                from_node=from_node,
                to_node=to_node,
            )
            self.addItem(edge_item)
            self.static_edges.append(edge_item)

    def export_graph_topology(self):
        """
        Exports the current node+edge data from the hop_map_manager.
        """
        return self.hop_map_manager.export_graph_topology()

    def add_rendered_nodes(self, new_hops: list[dict]):
        """
        Called by SignalManager after HopMapManager.add_hops().
        Creates QGraphicsItems for each new hop, then triggers a full layout
        recalculation so **all** nodes (old + new) snap to the updated grid.
        """
        if not new_hops:
            return

        for data in new_hops:
            node_item = NodeItem(
                x=data["x"],
                y=data["y"],
                radius=data["radius"],
                color=data["color"],
                label="",
                node_id=data["node_id"],
                delay_ms=data["delay_ms"],
            )
            self.addItem(node_item)
            self._node_items_by_id[data["node_id"]] = node_item

            text1, text2 = self._create_hop_label_block(
                data.get("label", ""),
                f"{data['delay_ms']} ms",
            )
            self._hop_label_items_by_id[data["node_id"]] = (text1, text2)
            self.addItem(text1)
            self.addItem(text2)

        w = self.sceneRect().width()
        h = self.sceneRect().height()
        self.recalc_node_positions(w, h)

    # ─────────────────────────────────────────────────────────────────
    # Private rendering helpers
    # ─────────────────────────────────────────────────────────────────
    def _render_nodes(self):
        """
        Clears scene and renders all nodes (start, hops, end) from hop_map_manager.
        """
        self.best_path_edges.clear()
        self.static_edges.clear()
        self._node_items_by_id.clear()
        self._bg_label_items_by_id.clear()
        self._hop_label_items_by_id.clear()
        self.clear()

        # Start node
        if self.hop_map_manager.start_node_data:
            data = self.hop_map_manager.start_node_data
            node_item = NodeItem(
                x=data["x"],
                y=data["y"],
                radius=data["radius"],
                color=data["color"],
                label=data["label"],
                node_id=data["node_id"],
                delay_ms=data["delay_ms"],
            )
            self.addItem(node_item)
            self._node_items_by_id[data["node_id"]] = node_item

            bg, lbl = self._create_background_label(
                data["x"],
                data["y"] + data["radius"] + 4,
                "Client",
            )
            self._bg_label_items_by_id[data["node_id"]] = (bg, lbl)

        # Hop nodes
        for data in self.hop_map_manager.hop_nodes_data:
            node_item = NodeItem(
                x=data["x"],
                y=data["y"],
                radius=data["radius"],
                color=data["color"],
                label="",
                node_id=data["node_id"],
                delay_ms=data["delay_ms"],
            )
            self.addItem(node_item)
            self._node_items_by_id[data["node_id"]] = node_item

            t1, t2 = self._create_hop_label_block(
                data.get("label", ""),
                f"{data['delay_ms']} ms",
            )
            self._hop_label_items_by_id[data["node_id"]] = (t1, t2)
            self.addItem(t1)
            self.addItem(t2)

        # End node
        if self.hop_map_manager.end_node_data:
            data = self.hop_map_manager.end_node_data
            node_item = NodeItem(
                x=data["x"],
                y=data["y"],
                radius=data["radius"],
                color=data["color"],
                label=data["label"],
                node_id=data["node_id"],
                delay_ms=data["delay_ms"],
            )
            self.addItem(node_item)
            self._node_items_by_id[data["node_id"]] = node_item

            bg, lbl = self._create_background_label(
                data["x"],
                data["y"] + data["radius"] + 4,
                "Server",
            )
            self._bg_label_items_by_id[data["node_id"]] = (bg, lbl)

    def _apply_positions_to_items(self):
        """
        Synchronises all QGraphicsItems to hop_map_manager-defined coordinates and
        updates attached text / edge geometry.
        """
        # Start node
        if self.hop_map_manager.start_node_data:
            d = self.hop_map_manager.start_node_data
            itm = self._node_items_by_id.get(d["node_id"])
            if itm:
                itm.setPos(d["x"], d["y"])

        # Hops
        for h in self.hop_map_manager.hop_nodes_data:
            itm = self._node_items_by_id.get(h["node_id"])
            if itm:
                itm.setPos(h["x"], h["y"])

        # End node
        if self.hop_map_manager.end_node_data:
            d = self.hop_map_manager.end_node_data
            itm = self._node_items_by_id.get(d["node_id"])
            if itm:
                itm.setPos(d["x"], d["y"])

        self._reposition_special_labels()

        for e_item in self.static_edges + self.best_path_edges:
            e_item.updateLine()

    def _reposition_special_labels(self):
        # Background labels (start / end)
        for node_id, (bg_rect, lbl_item) in self._bg_label_items_by_id.items():
            node_itm = self._node_items_by_id.get(node_id)
            if not node_itm:
                continue
            cx, cy, r = node_itm.x, node_itm.y, node_itm.radius
            bg_rect.setRect(cx - 25, cy + r, 50, 18)
            lbl_bounds = lbl_item.boundingRect()
            lbl_item.setPos(cx - lbl_bounds.width() / 2, cy + r + 1)

        # Two-line hop labels
        for node_id, (t1, t2) in self._hop_label_items_by_id.items():
            node_itm = self._node_items_by_id.get(node_id)
            if not node_itm:
                continue
            cx, cy = node_itm.x, node_itm.y
            r1 = t1.boundingRect()
            r2 = t2.boundingRect()
            spacing = -9.0
            total_h = r1.height() + r2.height() + spacing
            top_y = cy - total_h / 2
            t1.setPos(cx - r1.width() / 2, top_y)
            t2.setPos(cx - r2.width() / 2, top_y + r1.height() + spacing)

    # ─────────────────────────────────────────────────────────────────
    # Heat-map
    # ─────────────────────────────────────────────────────────────────
    def update_heatmap(self, pheromone_matrix: list[float]):
        """
        Called by solver signals to refresh or generate a new heatmap overlay,
        layering below all nodes & edges (ZValue=-9999).
        """
        if not pheromone_matrix:
            return

        node_positions = self._ordered_node_positions()
        pixmap = None

        if self._gpu_is_ok:
            try:
                scene_w = int(self.sceneRect().width())
                scene_h = int(self.sceneRect().height())
                pixmap = generate_heatmap_gl(
                    pheromone_matrix,
                    node_positions=node_positions,
                    width=scene_w,
                    height=scene_h,
                )
            except Exception as e:
                print(f"[GraphScene] GPU heatmap rendering failed: {e}")
                pixmap = None

        if pixmap is None or pixmap.isNull():
            pixmap = generate_heatmap(
                pheromone_matrix,
                node_positions=node_positions,
                size_factor=1.25,
            )

        if pixmap is None or pixmap.isNull():
            return

        self._invalidate_heatmap()
        self._heatmap_item = QGraphicsPixmapItem(pixmap)
        self._heatmap_item.setZValue(-9999)
        self.addItem(self._heatmap_item)

    def _invalidate_heatmap(self):
        if self._heatmap_item:
            try:
                self.removeItem(self._heatmap_item)
            except RuntimeError:
                pass
            self._heatmap_item = None

    def _ordered_node_positions(self):
        """
        Returns list whose index == node_id → (x, y) for heat-map input.
        """
        all_nodes = []
        if self.hop_map_manager.start_node_data:
            all_nodes.append(self.hop_map_manager.start_node_data)
        all_nodes.extend(self.hop_map_manager.hop_nodes_data)
        if self.hop_map_manager.end_node_data:
            all_nodes.append(self.hop_map_manager.end_node_data)

        if not all_nodes:
            return []

        max_id = max(nd["node_id"] for nd in all_nodes)
        positions = [(0.0, 0.0)] * (max_id + 1)
        for nd in all_nodes:
            positions[nd["node_id"]] = (nd["x"], nd["y"])
        return positions

    # ─────────────────────────────────────────────────────────────────
    # Label / background helpers
    # ─────────────────────────────────────────────────────────────────
    def _create_hop_label_block(self, line1: str, line2: str):
        font = QFont("Consolas", 7)
        font.setBold(False)

        t1 = QGraphicsTextItem(line1)
        t1.setFont(font)
        t1.setDefaultTextColor(Qt.white)
        t1.setZValue(10)
        t1.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)

        t2 = QGraphicsTextItem(line2)
        t2.setFont(font)
        t2.setDefaultTextColor(Qt.white)
        t2.setZValue(10)
        t2.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)

        return t1, t2

    def _create_background_label(
        self,
        x: float,
        y: float,
        text: str,
        bg_color: str = "#f5f5cc",
        text_color=Qt.black,
    ):
        rect = QRectF(x - 25, y, 50, 18)
        bg = QGraphicsRectItem(rect)
        bg.setBrush(QBrush(QColor(bg_color)))
        bg.setPen(QPen(Qt.NoPen))
        bg.setZValue(5)
        self.addItem(bg)

        lbl = QGraphicsTextItem(text)
        font = QFont("Consolas", 8)
        lbl.setFont(font)
        lbl.setDefaultTextColor(text_color)
        lbl.setZValue(6)
        lbl_bounds = lbl.boundingRect()
        lbl.setPos(x - lbl_bounds.width() / 2, y + 1)
        lbl.setCacheMode(QGraphicsTextItem.DeviceCoordinateCache)
        self.addItem(lbl)

        return bg, lbl

    def draw_multiple_paths(self, path_dict):
        """
        Removes old best_path_edges, then draws the new paths.
        Lines are placed **above** node circles for guaranteed visibility.
        """
        for e in self.best_path_edges:
            try:
                self.removeItem(e)
            except RuntimeError:
                pass
        self.best_path_edges.clear()

        color_map = {
            "aco":    ALGO_COLORS["aco"],
            "random": ALGO_COLORS["random"],
            "brute":  ALGO_COLORS["brute"],
        }
        offset_map = {
            "aco":    (0,   0),
            "random": (6,  -6),
            "brute":  (-6,  6),
        }

        for algo, info in path_dict.items():
            node_ids = info.get("nodes", [])
            if not node_ids:
                continue

            ox, oy   = offset_map.get(algo, (0, 0))
            p_color  = color_map.get(algo, "#ffffff")

            seq = [
                self._node_items_by_id.get(nid)
                for nid in node_ids
                if self._node_items_by_id.get(nid)
            ]
            for a, b in zip(seq, seq[1:]):
                edge = EdgeItem(
                    a.x + ox,
                    a.y + oy,
                    b.x + ox,
                    b.y + oy,
                    color=p_color,
                    pen_width=3,
                    from_node=None,
                    to_node=None,
                )
                edge.setZValue(2)
                self.addItem(edge)
                self.best_path_edges.append(edge)
