from gui.consts.algo_consts import PATH_DIRECTION

class GraphRoutingUtils:
    """
    Utility class to compute and evaluate geometric relationships
    between nodes on a HopMap-based grid. The primary entry point
    is compute_path(...), which generates path segments based on
    a list of node dictionaries.
    """

    def __init__(self, hop_map_manager):
        if hop_map_manager is None:
            raise ValueError("hop_map_manager cannot be None")
        self.hop_map = hop_map_manager
        self.row_spacing = self.hop_map.get_grid_row_spacing
        self.col_spacing = self.hop_map.get_grid_col_spacing
        self.last_direction = None

    def _get_node_id(self, node):
        node_id = node.get("node_id")
        if node_id is None:
            raise KeyError("Each node must contain a 'node_id' key.")
        return node_id

    def _get_row_col(self, node_id, col_count):
        if node_id < 2:
            return -1, -1
        return (node_id - 2) // col_count, (node_id - 2) % col_count

    def _validate_path(self, path):
        if not isinstance(path, list):
            raise TypeError("Path must be a list of node dictionaries.")
        for node in path:
            if not isinstance(node, dict):
                raise TypeError("Each node must be a dictionary.")
            if "x" not in node or "y" not in node or "node_id" not in node:
                raise KeyError("Each node must contain 'x', 'y', and 'node_id'.")

    def is_next_node_same_row(self, node_a, node_b):
        col_count = self.hop_map.get_grid_col_count()
        id_a = node_a["node_id"]
        id_b = node_b["node_id"]
        row_a, col_a = self._get_row_col(id_a, col_count)
        row_b, col_b = self._get_row_col(id_b, col_count)
        return row_a == row_b and row_a != -1

    def is_next_node_same_col(self, node_a, node_b):
        col_count = self.hop_map.get_grid_col_count()
        id_a = node_a["node_id"]
        id_b = node_b["node_id"]
        row_a, col_a = self._get_row_col(id_a, col_count)
        row_b, col_b = self._get_row_col(id_b, col_count)
        return col_a == col_b and col_a != -1

    def is_node_node_last_col(self, node):
        node_id = node["node_id"]
        col_count = self.hop_map.get_grid_col_count()
        _, col_index = self._get_row_col(node_id, col_count)
        return col_index == col_count - 1

    def get_node_col(self, node):
        node_id = node["node_id"]
        col_count = self.hop_map.get_grid_col_count()
        _, col_index = self._get_row_col(node_id, col_count)
        return col_index

    def get_node_coordinates(self, node):
        return node["x"], node["y"]

    def get_next_node_coordinates(self, path, current_index):
        if current_index < 0 or current_index >= len(path) - 1:
            return None
        nxt = path[current_index + 1]
        return nxt["x"], nxt["y"]

    def refresh_spacing(self):
        self.row_spacing = self.hop_map.get_grid_row_spacing
        self.col_spacing = self.hop_map.get_grid_col_spacing

    # ----- PHASE METHODS -----
    def phase_1(self, node_0, next_y, dx, dy):
        start_x = node_0["x"] + dx
        start_y = node_0["y"] + dy
        mid_x = start_x
        mid_y = next_y + dy

        self.last_direction = (
            PATH_DIRECTION[0] if mid_y < start_y
            else PATH_DIRECTION[2] if mid_y > start_y
            else None
        )
        return [(start_x, start_y, mid_x, mid_y)], mid_x, mid_y

    def phase_2(self, node_0, node_1, mid_x, mid_y, next_x, dx, half_row):
        col_count = self.hop_map.get_grid_col_count()
        col_index = (node_1["node_id"] - 2) % col_count
        if col_index == 0:
            return [(mid_x, mid_y, next_x + dx, mid_y)]
        extension = node_0["radius"] + half_row
        ext_x = mid_x
        ext_y = mid_y - extension if self.last_direction == "up" else mid_y + extension
        return [
            (mid_x, mid_y, ext_x, ext_y),
            (ext_x, ext_y, next_x + dx, ext_y)
        ]

    def phase_3(self, node_0, node_1, next_x, ext_y, half_row):
        col_count = self.hop_map.get_grid_col_count()
        col_index = (node_1["node_id"] - 2) % col_count
        if col_index == 0:
            return []
        extension = node_0["radius"] + half_row
        final_y2 = (
            ext_y + extension if self.last_direction == "up"
            else ext_y - extension if self.last_direction == "down"
            else ext_y
        )
        return [(next_x, ext_y, next_x, final_y2)]

    def phase_4(self, path, dx, dy, col_count, row_spacing, col_spacing):
        segments = []
        for i in range(1, len(path) - 2):
            n1, n2 = path[i], path[i + 1]
            x1, y1 = n1["x"] + dx, n1["y"] + dy
            x2, y2 = n2["x"] + dx, n2["y"] + dy

            if self.is_next_node_same_row(n1, n2):
                if n2["node_id"] == n1["node_id"] + 1:
                    segments.append((x1, y1, x2, y1))
                else:
                    mid_y1 = y1 - row_spacing / 2.0
                    segments.append((x1, y1, x1, mid_y1))
                    segments.append((x1, mid_y1, x2, mid_y1))
                    segments.append((x2, mid_y1, x2, y2))
            elif self.is_next_node_same_col(n1, n2):
                if n2["node_id"] == n1["node_id"] + col_count:
                    segments.append((x1, y1, x1, y2))
                else:
                    mid_x1 = x1 - col_spacing / 2.0
                    segments.append((x1, y1, mid_x1, y1))
                    segments.append((mid_x1, y1, mid_x1, y2))
                    segments.append((mid_x1, y2, x2, y2))
            else:
                # Complex pattern
                mid_y2 = y1 + n1["radius"] + row_spacing / 2.0
                segments.append((x1, y1, x1, mid_y2))
                segments.append((x1, mid_y2, x2 + col_spacing, mid_y2))
                segments.append((x2 + col_spacing, mid_y2, x2 + col_spacing, y2 + n2["radius"] + row_spacing / 2.0))
                segments.append((x2 + col_spacing, y2 + n2["radius"] + row_spacing / 2.0, x2, y2 + n2["radius"] + row_spacing / 2.0))
                segments.append((x2, y2 + n2["radius"] + row_spacing / 2.0, x2, y2))

        # Final connection
        if len(path) < 2:
            return segments
        n_last = path[-1]
        if segments:
            x2, y2 = segments[-1][2], segments[-1][3]
        else:
            n_prev = path[-2]
            x2, y2 = n_prev["x"] + dx, n_prev["y"] + dy

        final_x = n_last["x"] + dx
        final_y = n_last["y"] + dy
        if self.is_node_node_last_col(path[-2]):
            segments.append((x2, y2, final_x, y2))
            segments.append((final_x, y2, final_x, final_y))
        else:
            mid_y = y2 - row_spacing / 2.0
            segments.append((x2, y2, x2, mid_y))
            segments.append((x2, mid_y, final_x, mid_y))
            segments.append((final_x, mid_y, final_x, final_y))

        return segments

    def compute_path(self, path, offset=(0, 0)):
        self._validate_path(path)
        if len(path) < 2:
            return []

        dx, dy = offset
        # Cache spacing & col_count
        row_space = self.row_spacing()
        col_space = self.col_spacing()
        col_count = self.hop_map.get_grid_col_count()

        # next coords
        nxt = self.get_next_node_coordinates(path, 0)
        if not nxt:
            return []
        next_x, next_y = nxt

        # Phase 1
        seg1, mid_x, mid_y = self.phase_1(path[0], next_y, dx, dy)
        # Phase 2
        seg2 = self.phase_2(path[0], path[1], mid_x, mid_y, next_x, dx,
                            row_space / 2.0)
        ext_y = seg2[-1][3] if seg2 else mid_y
        # Phase 3
        seg3 = self.phase_3(path[0], path[1], next_x + dx, ext_y,
                            row_space / 2.0)
        # Phase 4
        seg4 = self.phase_4(path, dx, dy, col_count, row_space, col_space)

        return seg1 + seg2 + seg3 + seg4
