# Relative Path: src/python/gui/managers/path_computation.py
"""
This module provides a helper to compute a discrete path polyline (A* with bounding box)
via the backend's pub_render_path_grid function. The node positions and bounding logic
are handled entirely by the backend, returning a final list of (x, y) points.
"""

from typing import List, Tuple
from ffi.backend_api import ffi, lib, ERR_SUCCESS

def compute_polyline(antnet_wrapper, node_ids: List[int], offset_x: float = 0.0, offset_y: float = 0.0) -> List[Tuple[float, float]]:
    """
    compute_polyline: uses the backend's pub_render_path_grid through the existing stub
    function to produce the final path. Returns list of (x, y) floats.
    antnet_wrapper is an instance of AntNetWrapper.

    The node_ids is a list of integers. The backend does the scaling and discrete A* logic.
    """
    if not node_ids or antnet_wrapper.context_id is None:
        return []

    arr_len = len(node_ids)

    # We must build the raw C arrays for the direct stub call
    c_node_ids = ffi.new("int[]", arr_len)
    for i, nval in enumerate(node_ids):
        c_node_ids[i] = nval

    max_coords = 2 * 4096  # capacity for up to 2048 points
    c_outcoords = ffi.new("float[]", max_coords)
    c_count = ffi.new("int*")

    rc = lib.pub_render_path_grid(
        antnet_wrapper.context_id,
        c_node_ids,
        arr_len,
        offset_x,
        offset_y,
        c_outcoords,
        max_coords,
        c_count
    )
    if rc != ERR_SUCCESS:
        return []

    result = []
    out_len = c_count[0]
    for idx in range(0, out_len, 2):
        fx = c_outcoords[idx]
        fy = c_outcoords[idx + 1]
        result.append((fx, fy))

    return result
