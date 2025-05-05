# tests/test_backend_api.py

import os
import sys

# Add build and src/python paths dynamically
this_dir = os.path.dirname(__file__)
project_root = os.path.abspath(os.path.join(this_dir, ".."))
sys.path.insert(0, os.path.join(project_root, "build/python"))
sys.path.insert(0, os.path.join(project_root, "src/python"))

from ffi.backend_api import AntNetWrapper
from backend_cffi import ffi, lib


def test_basic_backend_integration():
    """
    Basic integration test covering initialization, iteration,
    path retrieval, and shutdown.
    """
    wrapper = AntNetWrapper(10, 2, 5)
    wrapper.run_iteration()
    result = wrapper.get_best_path_struct()

    # Verify best_path return
    assert result is not None
    assert isinstance(result["nodes"], list)
    assert isinstance(result["total_latency"], int)
    assert len(result["nodes"]) == 6

    wrapper.shutdown()


def test_topology_update():
    """
    Test updating the topology with user-defined node/edge data
    and verify that no error is raised and the context remains valid.
    """
    # Create a context with 4 default nodes
    wrapper = AntNetWrapper(4, 1, 3)

    # Verify it runs at least one iteration without error
    wrapper.run_iteration()
    path_info_before = wrapper.get_best_path_struct()
    assert path_info_before is not None

    # Define new topology data
    # This example has 3 nodes, 2 edges.
    new_nodes = [
        {"node_id": 0, "delay_ms": 50},
        {"node_id": 1, "delay_ms": 10},
        {"node_id": 2, "delay_ms": 20},
    ]
    new_edges = [
        {"from_id": 0, "to_id": 1},
        {"from_id": 1, "to_id": 2},
    ]

    # Update the topology
    wrapper.update_topology(new_nodes, new_edges)

    # Run another iteration post-update, ensure no crash or error
    wrapper.run_iteration()
    path_info_after = wrapper.get_best_path_struct()
    assert path_info_after is not None

    # For now, best path is still mock-based in the backend,
    # so length or structure may not change significantly.
    # Check that the dictionary format remains valid:
    assert "nodes" in path_info_after
    assert "total_latency" in path_info_after

    wrapper.shutdown()


def test_multiple_iterations_with_updates():
    """
    Test performing multiple iterations and multiple updates, ensuring
    the backend remains stable across changes.
    """
    wrapper = AntNetWrapper(5, 1, 5)

    # First run iteration several times
    for _ in range(3):
        wrapper.run_iteration()

    # Define a first update
    first_nodes = [
        {"node_id": 0, "delay_ms": 5},
        {"node_id": 1, "delay_ms": 15},
        {"node_id": 2, "delay_ms": 25},
        {"node_id": 3, "delay_ms": 35},
    ]
    first_edges = [
        {"from_id": 0, "to_id": 1},
        {"from_id": 1, "to_id": 2},
        {"from_id": 2, "to_id": 3},
    ]
    wrapper.update_topology(first_nodes, first_edges)

    # Run iterations with the first topology
    for _ in range(2):
        wrapper.run_iteration()
    path_info_1 = wrapper.get_best_path_struct()
    assert path_info_1 is not None
    assert "nodes" in path_info_1
    assert "total_latency" in path_info_1

    # Define a second update
    second_nodes = [
        {"node_id": 10, "delay_ms": 100},
        {"node_id": 11, "delay_ms": 200},
    ]
    second_edges = [
        {"from_id": 10, "to_id": 11},
    ]
    wrapper.update_topology(second_nodes, second_edges)

    # Run iterations with the second topology
    for _ in range(2):
        wrapper.run_iteration()
    path_info_2 = wrapper.get_best_path_struct()
    assert path_info_2 is not None
    assert "nodes" in path_info_2
    assert "total_latency" in path_info_2

    # Shutdown at the end
    wrapper.shutdown()
