# src/python/gui/managers/hop_map_manager.py
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
        "to_id": 3
      }
    """

    def __init__(self):
        self.start_node_data = None
        self.end_node_data = None
        self.hop_nodes_data = []
        self.edges_data = []

        # Track the highest node_id used so far; updated in initialize_map().
        self._last_node_id = 1  # Will be revised once nodes are created

    def initialize_map(self, total_nodes: int):
        """
        Creates the default set of nodes (start, end, and hop nodes),
        with random positions and latencies. Clears any pre-existing data.
        """
        if total_nodes < 2:
            total_nodes = 2

        self.start_node_data = None
        self.end_node_data = None
        self.hop_nodes_data.clear()
        self.edges_data.clear()

        # Basic geometry placeholders
        width = 1000.0
        height = 600.0

        margin = 50.0
        radius = 15
        mid_y = height / 2.0

        # Create Client (start) node
        self.start_node_data = {
            "node_id": 0,
            "x": margin,
            "y": mid_y,
            "radius": radius,
            "color": CLIENT_NODE_COLOR,
            "label": "Client",
            "delay_ms": random.randint(10, 50)
        }

        # Create Server (end) node
        end_x = width - margin
        self.end_node_data = {
            "node_id": 1,
            "x": end_x,
            "y": mid_y,
            "radius": radius,
            "color": SERVER_NODE_COLOR,
            "label": "Server",
            "delay_ms": random.randint(10, 50)
        }

        # Create hop nodes
        hop_count = total_nodes - 2
        placed_hops = []
        for i in range(hop_count):
            node_id = i + 2
            hop_data = self._create_random_hop(
                node_id=node_id, 
                radius=radius,
                margin=margin, 
                width=width, 
                height=height
            )
            placed_hops.append(hop_data)
        self.hop_nodes_data.extend(placed_hops)

        # Update _last_node_id to the maximum created so far
        self._last_node_id = max(1, total_nodes - 1)

    def _create_random_hop(self, node_id: int, radius: int, margin: float,
                           width: float, height: float) -> dict:
        """
        Finds a random (x, y) that does not overlap with existing hop nodes
        or the start/end nodes. Returns a node data dict.
        This version uses uniform distribution and slightly stricter spacing
        to reduce overlapping.
        """
        max_tries = 100
        required_spacing_sq = (radius * 2 + 30) ** 2

        existing_positions = []
        if self.start_node_data:
            existing_positions.append((self.start_node_data["x"], self.start_node_data["y"]))
        if self.end_node_data:
            existing_positions.append((self.end_node_data["x"], self.end_node_data["y"]))
        for h in self.hop_nodes_data:
            existing_positions.append((h["x"], h["y"]))

        placed = False
        x_final = width / 2.0
        y_final = height / 2.0

        # First attempt: random uniform tries
        for _ in range(max_tries):
            x = random.uniform(margin, width - margin)
            y = random.uniform(margin, height - margin)
            if all((ox - x)**2 + (oy - y)**2 >= required_spacing_sq for ox, oy in existing_positions):
                x_final = x
                y_final = y
                placed = True
                break

        if not placed:
            # Fallback if we never found a non-overlapping spot
            # Attempt corners with small offsets
            corners = [
                (margin, margin),
                (width - margin, margin),
                (margin, height - margin),
                (width - margin, height - margin),
            ]
            corner_indices = [0, 1, 2, 3]
            random.shuffle(corner_indices)

            corner_placed = False
            for idx in corner_indices:
                cx, cy = corners[idx]
                for _ in range(10):
                    ox = cx + random.uniform(-40.0, 40.0)
                    oy = cy + random.uniform(-40.0, 40.0)
                    if all((ox2 - ox)**2 + (oy2 - oy)**2 >= required_spacing_sq
                           for ox2, oy2 in existing_positions):
                        x_final = ox
                        y_final = oy
                        corner_placed = True
                        placed = True
                        break
                if corner_placed:
                    break

            if not corner_placed:
                # If still not found, force place at the first corner (may overlap)
                x_final, y_final = corners[corner_indices[0]]

        return {
            "node_id": node_id,
            "x": x_final,
            "y": y_final,
            "radius": radius,
            "color": HOP_NODE_COLOR,
            "label": f"hop #{node_id - 2}",
            "delay_ms": random.randint(10, 50)
        }

    def create_default_edges(self):
        """
        Builds a default path: start -> up to 3 nearest hops -> end.
        Stores edges in self.edges_data.
        """
        self.edges_data.clear()

        if not self.start_node_data or not self.end_node_data:
            return

        interior = min(3, len(self.hop_nodes_data))
        path_node_ids = [self.start_node_data["node_id"]]
        remaining = list(self.hop_nodes_data)

        def dist_sq(ax, ay, bx, by):
            return (ax - bx)**2 + (ay - by)**2

        current = self.start_node_data
        for _ in range(interior):
            if not remaining:
                break
            nearest = min(
                remaining,
                key=lambda h: dist_sq(h["x"], h["y"], current["x"], current["y"])
            )
            path_node_ids.append(nearest["node_id"])
            remaining.remove(nearest)
            current = nearest

        path_node_ids.append(self.end_node_data["node_id"])

        # Build edges
        for i in range(len(path_node_ids) - 1):
            self.edges_data.append({
                "from_id": path_node_ids[i],
                "to_id":   path_node_ids[i+1]
            })

    def export_graph_topology(self):
        """
        Returns a dictionary containing the nodes and edges in the current map.
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

        new_hops = []
        for _ in range(count):
            new_id = self._last_node_id + 1
            hop_data = self._create_random_hop(
                node_id=new_id,
                radius=radius,
                margin=margin,
                width=width,
                height=height
            )
            self.hop_nodes_data.append(hop_data)
            new_hops.append(hop_data)
            self._last_node_id = new_id

        return new_hops
