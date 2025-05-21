# Relative Path: tests/test_render_path_grid.py
"""
Unit test for backend pub_render_path_grid via the Python helper 'compute_polyline'.
Ensures a small topology yields a correct polyline path.
"""

import os
import sys
import pytest
import math

# Adjust sys.path if needed to locate the build or src/python
this_dir = os.path.dirname(__file__)
project_root = os.path.abspath(os.path.join(this_dir, ".."))
sys.path.insert(0, os.path.join(project_root, "build", "python"))
sys.path.insert(0, os.path.join(project_root, "src", "python"))

from ffi.backend_api import AntNetWrapper
from gui.managers.path_computation import compute_polyline

def _close_enough(a, b, tol=1e-3):
    """Helper to check if two floats are within `tol` of each other."""
    return abs(a - b) <= tol

@pytest.mark.parametrize("test_offset", [(0.0, 0.0), (5.0, -5.0)])
def test_render_path_grid_minimal(test_offset):
    """
    Validates that pub_render_path_grid (via compute_polyline) returns
    a non-empty polyline that begins and ends near the correct node coordinates.
    Also verifies optional offset is applied.
    """

    # 1) Create a small AntNet context (no config file). Just 4 nodes total:
    #    Node 0 (start), Node 1 (end), Node 2 & 3 (some mid hops).
    #    We'll connect them so 0->2->3->1 is a valid path.
    min_hops = 1
    max_hops = 2
    antnet = AntNetWrapper(node_count=4, min_hops=min_hops, max_hops=max_hops)

    # 2) Define topology with explicit (x,y) for each node.
    #    Node 0 is at (100,200), Node 1 at (400,200), etc.
    #    Edges form a chain: 0->2->3->1
    nodes = [
        {"node_id": 0, "x": 100.0, "y": 200.0, "delay_ms": 10, "radius": 15},
        {"node_id": 1, "x": 400.0, "y": 200.0, "delay_ms": 10, "radius": 15},
        {"node_id": 2, "x": 200.0, "y": 100.0, "delay_ms": 20, "radius": 15},
        {"node_id": 3, "x": 300.0, "y": 300.0, "delay_ms": 25, "radius": 15},
    ]
    edges = [
        {"from_id": 0, "to_id": 2},
        {"from_id": 2, "to_id": 3},
        {"from_id": 3, "to_id": 1},
        # optional direct 0->1 if you want a fallback
    ]

    # 3) Update topology in the backend
    antnet.update_topology(nodes, edges)

    # 4) Suppose we want a path from node 0 -> 2 -> 3 -> 1
    #    We'll ask the backend to build a discrete polyline for that ID list.
    node_ids = [0, 2, 3, 1]
    offset_x, offset_y = test_offset

    coords = compute_polyline(antnet, node_ids, offset_x=offset_x, offset_y=offset_y)

    # 5) Validate the returned polyline
    #    - Must be non-empty
    #    - The first point should be near node 0 + offset
    #    - The last point near node 1 + offset
    #    - The path presumably has corners for 2,3 in between
    assert coords, "No coordinates returned from compute_polyline"

    first_pt = coords[0]
    last_pt = coords[-1]

    # Node 0 is at (100,200). The offset is (offset_x, offset_y).
    expected_x0 = 100.0 + offset_x
    expected_y0 = 200.0 + offset_y

    # Node 1 is at (400,200). The offset is (offset_x, offset_y).
    expected_x1 = 400.0 + offset_x
    expected_y1 = 200.0 + offset_y

    assert _close_enough(first_pt[0], expected_x0), f"First point X mismatch: {first_pt[0]} vs {expected_x0}"
    assert _close_enough(first_pt[1], expected_y0), f"First point Y mismatch: {first_pt[1]} vs {expected_y0}"

    assert _close_enough(last_pt[0], expected_x1), f"Last point X mismatch: {last_pt[0]} vs {expected_x1}"
    assert _close_enough(last_pt[1], expected_y1), f"Last point Y mismatch: {last_pt[1]} vs {expected_y1}"

    # Optionally: check that there's at least one corner in the path
    # (2 or more segments => at least 3 points).
    # This is not guaranteed if the backend found a shorter route, but let's just
    # demonstrate an approach:
    assert len(coords) >= 3, "Expected at least one corner in the polyline"

    # 6) Clean up
    antnet.shutdown()

    print(f"✅ test_render_path_grid_minimal offset={test_offset} → coords length={len(coords)}")
