# Relative Path: src/python/gui/managers/hop_map_manager.py
"""
Manages creation, deletion, and management of all hop nodes.
Centralizes latency handling and topological data (edges).
Decouples core logic from Qt drawing classes.
Can later support map persistence, topological presets, or procedural generation.

Thread safety is delegated to higher-level locks when calling these methods.
No direct Qt dependencies here.
"""

import random
import math
from gui.consts.gui_consts import CLIENT_NODE_COLOR, SERVER_NODE_COLOR, HOP_NODE_COLOR

class HopMapManager:
    """
    Handles conceptual node data and edges for the entire map.
    Maintains:
      - start_node_data (dict)
      - end_node_data (dict)
      - hop_nodes_data (list of dict)
      - edges_data (list of dict)
    Node dict format example:
      {
        "node_id": 0,
        "x": <float>,
        "y": <float>,
        "radius": 15,
        "color": "#abc123",
        "label": "Client",
        "delay_ms": 30
      }
    Edge dict format example:
      {
        "from_id": 0,
        "to_id": 3,
        "color": "#000000"
      }
    """

    def __init__(self):
        self.start_node_data = None
        self.end_node_data = None
        self.hop_nodes_data = []
        self.edges_data = []

        self._last_node_id = 1

        # Default range for random latencies
        self._default_min_delay = 10
        self._default_max_delay = 50

    def set_delay_range(self, min_delay: int, max_delay: int):
        """
        Adjusts the random latency range for newly created or re-initialized nodes.
        """
        if min_delay < 0:
            min_delay = 0
        if max_delay < min_delay:
            max_delay = min_delay

        self._default_min_delay = min_delay
        self._default_max_delay = max_delay

    def _get_random_delay(self) -> int:
        """
        Returns a random delay within the configured range.
        """
        if self._default_max_delay < self._default_min_delay:
            return self._default_min_delay
        return random.randint(self._default_min_delay, self._default_max_delay)

    def initialize_map(self, total_nodes: int):
        """
        Creates the default set of nodes (start, end, and hop nodes), with a grid arrangement.
        Latency is random within the configured range. If total_nodes is unchanged, skip re-init.
        """
        current_total = 0
        if self.start_node_data:
            current_total += 1
        if self.end_node_data:
            current_total += 1
        current_total += len(self.hop_nodes_data)

        if current_total == total_nodes:
            return

        if total_nodes < 2:
            total_nodes = 2

        self.start_node_data = None
        self.end_node_data = None
        self.hop_nodes_data.clear()
        self.edges_data.clear()

        width = 1000.0
        height = 600.0

        margin = 50.0
        radius = 15
        mid_y = height / 2.0

        # Start node
        self.start_node_data = {
            "node_id": 0,
            "x": margin,
            "y": mid_y,
            "radius": radius,
            "color": CLIENT_NODE_COLOR,
            "label": "Client",
            "delay_ms": self._get_random_delay()
        }

        # End node
        end_x = width - margin
        self.end_node_data = {
            "node_id": 1,
            "x": end_x,
            "y": mid_y,
            "radius": radius,
            "color": SERVER_NODE_COLOR,
            "label": "Server",
            "delay_ms": self._get_random_delay()
        }

        hop_count = total_nodes - 2
        if hop_count > 0:
            grid_left = self.start_node_data["x"] + 100.0
            grid_right = self.end_node_data["x"] - 100.0
            if grid_right < grid_left:
                grid_left = self.start_node_data["x"]
                grid_right = self.start_node_data["x"]

            row_count = int(math.ceil(math.sqrt(hop_count)))
            col_count = int(math.ceil(hop_count / row_count))

            w = max(1.0, grid_right - grid_left)
            h = max(1.0, height - 2.0 * margin)

            cell_width = w / col_count
            cell_height = h / row_count

            for i in range(hop_count):
                node_id = i + 2
                row = i // col_count
                col = i % col_count
                cx = grid_left + (col + 0.5) * cell_width
                cy = (margin) + (row + 0.5) * cell_height

                hop_data = {
                    "node_id": node_id,
                    "x": cx,
                    "y": cy,
                    "radius": radius,
                    "color": HOP_NODE_COLOR,
                    "label": f"#{node_id - 2}",
                    "delay_ms": self._get_random_delay()
                }
                self.hop_nodes_data.append(hop_data)

        self._last_node_id = max(1, total_nodes - 1)

    def recalc_positions(self, scene_width: float, scene_height: float):
        """
        Re-lays out the existing nodes to fill the horizontal space up to scene_width,
        and centers them vertically within scene_height. Does not reset latencies or IDs.
        """
        if scene_width < 10 or scene_height < 10:
            return

        margin = 50.0
        radius = 15

        if self.start_node_data:
            self.start_node_data["x"] = margin
            self.start_node_data["y"] = scene_height * 0.5
        if self.end_node_data:
            self.end_node_data["x"] = scene_width - margin
            self.end_node_data["y"] = scene_height * 0.5

        if not self.hop_nodes_data:
            return

        grid_left = (self.start_node_data["x"] + 100.0) if self.start_node_data else 100.0
        grid_right = (self.end_node_data["x"] - 100.0) if self.end_node_data else (scene_width - 100.0)
        if grid_right < grid_left:
            grid_left = margin
            grid_right = margin

        hop_count = len(self.hop_nodes_data)
        rowsf = math.sqrt(hop_count)
        row_count = int(math.ceil(rowsf))
        col_count = int(math.ceil(hop_count / row_count))

        usable_height = scene_height - 2.0 * margin
        if usable_height < 1.0:
            return
        cell_height = usable_height / row_count
        if cell_height < (2.0 * radius):
            cell_height = 2.0 * radius

        total_grid_height = row_count * cell_height
        top_offset = 0.5 * (scene_height - total_grid_height)
        if top_offset < margin:
            top_offset = margin

        w = max(1.0, (grid_right - grid_left))
        cell_width = w / col_count
        if cell_width < (2.0 * radius):
            cell_width = 2.0 * radius

        for i, hop_data in enumerate(self.hop_nodes_data):
            row = i // col_count
            col = i % col_count
            cx = grid_left + (col + 0.5) * cell_width
            cy = top_offset + (row + 0.5) * cell_height
            hop_data["x"] = cx
            hop_data["y"] = cy

    def create_default_edges(self):
        """
        Builds a default path: start -> up to 3 nearest hops -> end.
        Stores edges in self.edges_data.
        The default path color is forced transparent so it does not clutter the view.
        """
        self.edges_data.clear()
        if not self.start_node_data or not self.end_node_data:
            return

        if not self.hop_nodes_data:
            edge_transparent = {
                "from_id": self.start_node_data["node_id"],
                "to_id":   self.end_node_data["node_id"],
                "color": "#00000000"
            }
            self.edges_data.append(edge_transparent)
            return

        interior = min(3, len(self.hop_nodes_data))
        path_node_ids = [self.start_node_data["node_id"]]
        used_indices = set()

        def dist_sq(ax, ay, bx, by):
            return (ax - bx)**2 + (ay - by)**2

        current = self.start_node_data
        for _ in range(interior):
            best_idx = -1
            best_d = 1e12
            for idx, hop in enumerate(self.hop_nodes_data):
                if idx in used_indices:
                    continue
                dsq = dist_sq(current["x"], current["y"], hop["x"], hop["y"])
                if dsq < best_d:
                    best_d = dsq
                    best_idx = idx
            if best_idx >= 0:
                path_node_ids.append(self.hop_nodes_data[best_idx]["node_id"])
                used_indices.add(best_idx)
                current = self.hop_nodes_data[best_idx]

        path_node_ids.append(self.end_node_data["node_id"])
        for i in range(len(path_node_ids) - 1):
            edge_transparent = {
                "from_id": path_node_ids[i],
                "to_id":   path_node_ids[i+1],
                "color": "#00000000"
            }
            self.edges_data.append(edge_transparent)

    def export_graph_topology(self):
        """
        Returns a dictionary containing the nodes and edges in the current map.
        This is the standard full topology export (including start/end).
        """
        nodes = []
        if self.start_node_data:
            nodes.append({
                "node_id":   self.start_node_data["node_id"],
                "delay_ms":  self.start_node_data["delay_ms"]
            })
        for h in self.hop_nodes_data:
            nodes.append({
                "node_id":  h["node_id"],
                "delay_ms": h["delay_ms"]
            })
        if self.end_node_data:
            nodes.append({
                "node_id":  self.end_node_data["node_id"],
                "delay_ms": self.end_node_data["delay_ms"]
            })

        return {
            "nodes": nodes,
            "edges": list(self.edges_data)
        }

    def export_graph_topology_for_heatmap(self):
        """
        Returns only the hop nodes and edges that do not connect
        to the start or end node (node_id=0 or node_id=1).
        This excludes the Client and Server from the heatmap.
        """
        filtered_edges = []
        for e in self.edges_data:
            if e["from_id"] not in (0,1) and e["to_id"] not in (0,1):
                filtered_edges.append(e)

        nodes = []
        for h in self.hop_nodes_data:
            nodes.append({
                "node_id":  h["node_id"],
                "delay_ms": h["delay_ms"]
            })

        return {
            "nodes": nodes,
            "edges": filtered_edges
        }

    def add_hops(self, count: int):
        """
        Adds `count` new hop nodes with unique IDs, appends to self.hop_nodes_data,
        does NOT reset start/end nodes or existing edges.
        Returns the list of newly-created hop dicts.
        """
        width = 1000.0
        height = 600.0
        margin = 50.0
        radius = 15

        grid_left = (self.start_node_data["x"] if self.start_node_data else 50.0) + 100.0
        grid_right = (self.end_node_data["x"] if self.end_node_data else 950.0) - 100.0

        new_hops = []
        for _ in range(count):
            new_id = self._last_node_id + 1
            cx = grid_left
            cy = margin
            rand_delay = self._get_random_delay()
            hop_data = {
                "node_id": new_id,
                "x": cx,
                "y": cy,
                "radius": radius,
                "color": HOP_NODE_COLOR,
                "label": f"#{new_id - 2}",
                "delay_ms": rand_delay
            }
            self.hop_nodes_data.append(hop_data)
            new_hops.append(hop_data)
            self._last_node_id = new_id

        return new_hops
