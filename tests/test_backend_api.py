# tests/test_backend_api.py
"""
PyTest suite validating the AntNet C backend via CFFI bindings.

Readability: every succeeded test prints a leading ✅ line.          # hardening/log
Security-oriented tests print 🛡️ SECURITY ✅ on success.
Prints bypass PyTest's capture so they always appear in build logs.
"""

import os
import sys
import pytest
import threading
import time
import os as _os


# -------------------------------------------------------------------- util
def _announce(msg: str) -> None:
    """Bypass pytest capture – always visible in CI logs."""
    _os.write(1, (msg + "\n").encode())


# ----------------------------------------------------------------- sys-path
this_dir = os.path.dirname(__file__)
project_root = os.path.abspath(os.path.join(this_dir, ".."))
sys.path.insert(0, os.path.join(project_root, "build/python"))
sys.path.insert(0, os.path.join(project_root, "src/python"))

from ffi.backend_api import AntNetWrapper
from backend_cffi import ffi, lib


# -------------------------------------------------------------- basic flow
def test_basic_backend_integration():
    """
    Basic integration test covering initialization, iteration,
    path retrieval, and shutdown.
    """
    wrapper = AntNetWrapper(10, 2, 5)
    wrapper.run_iteration()
    result = wrapper.get_best_path_struct()

    assert result is not None
    assert isinstance(result["nodes"], list)
    assert isinstance(result["total_latency"], int)
    assert len(result["nodes"]) == 6

    wrapper.shutdown()
    _announce("✅ basic_backend_integration")


# ------------------------------------------------------------- topology ok
def test_topology_update():
    """
    Update topology data and verify backend stability.
    """
    wrapper = AntNetWrapper(4, 1, 3)
    wrapper.run_iteration()

    new_nodes = [
        {"node_id": 0, "delay_ms": 50},
        {"node_id": 1, "delay_ms": 10},
        {"node_id": 2, "delay_ms": 20},
    ]
    new_edges = [
        {"from_id": 0, "to_id": 1},
        {"from_id": 1, "to_id": 2},
    ]
    wrapper.update_topology(new_nodes, new_edges)

    wrapper.run_iteration()
    path_info_after = wrapper.get_best_path_struct()

    assert path_info_after is not None
    assert "nodes" in path_info_after and "total_latency" in path_info_after

    wrapper.shutdown()
    _announce("✅ topology_update")


# ------------------------------------------------ multiple iterations / updates
def test_multiple_iterations_with_updates():
    """
    Exercise multiple topology changes across iterations.
    """
    wrapper = AntNetWrapper(5, 1, 5)

    for _ in range(3):
        wrapper.run_iteration()

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

    for _ in range(2):
        wrapper.run_iteration()

    second_nodes = [
        {"node_id": 10, "delay_ms": 100},
        {"node_id": 11, "delay_ms": 200},
    ]
    second_edges = [{"from_id": 10, "to_id": 11}]
    wrapper.update_topology(second_nodes, second_edges)

    for _ in range(2):
        wrapper.run_iteration()

    path_info = wrapper.get_best_path_struct()
    assert path_info is not None
    assert "nodes" in path_info and "total_latency" in path_info

    wrapper.shutdown()
    _announce("✅ multiple_iterations_with_updates")


# --------------------------------------------- security: invalid topology
def test_invalid_topology_args():
    """
    Security: ensure invalid node/edge IDs are rejected.
    """
    wrapper = AntNetWrapper(4, 1, 3)

    invalid_nodes = [
        {"node_id": 0, "delay_ms": 50},
        {"node_id": -1, "delay_ms": 10},
    ]
    invalid_edges = [{"from_id": 0, "to_id": -1}]

    with pytest.raises(ValueError):
        wrapper.update_topology(invalid_nodes, invalid_edges)

    wrapper.shutdown()
    _announce("🛡️ SECURITY ✅ invalid_topology_args")


# --------------------------------------------- security: latency overflow
def test_extreme_latency_overflow():
    """
    Security: detect integer overflow on huge latency sums.
    """
    wrapper = AntNetWrapper(5, 1, 5)

    big_nodes = [
        {"node_id": 0, "delay_ms": 2_000_000_000},
        {"node_id": 1, "delay_ms": 2_140_000_000},
        {"node_id": 2, "delay_ms": 1_000},
    ]
    big_edges = [{"from_id": 0, "to_id": 1}, {"from_id": 1, "to_id": 2}]
    wrapper.update_topology(big_nodes, big_edges)

    with pytest.raises(ValueError):
        wrapper.run_all_solvers()

    wrapper.shutdown()
    _announce("🛡️ SECURITY ✅ extreme_latency_overflow")


# ------------------------------------------------------ concurrency check
def test_concurrent_contexts():
    """
    Stress test concurrency with multiple contexts / threads.
    """
    def worker(w, it=5):
        for _ in range(it):
            w.run_iteration()
            time.sleep(0.01)

    wrappers = [AntNetWrapper(6, 1, 5) for _ in range(3)]
    threads = [threading.Thread(target=worker, args=(w,)) for w in wrappers]

    for t in threads:
        t.start()
    for t in threads:
        t.join()

    for w in wrappers:
        assert "nodes" in w.get_best_path_struct()
        w.shutdown()

    _announce("✅ concurrent_contexts")


# -------------------------------------- security: invalid context IDs
def test_multiple_invalid_context_ids():
    """
    Security: API returns error for nonsense context IDs.
    """
    assert lib.pub_run_iteration(-9999) < 0
    assert lib.pub_shutdown(-9999) < 0
    assert lib.pub_run_iteration(9999) < 0
    assert lib.pub_shutdown(9999) < 0
    _announce("🛡️ SECURITY ✅ multiple_invalid_context_ids")
