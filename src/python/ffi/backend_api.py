# src/python/ffi/backend_api.py
# High-level Python wrapper for the C AntNet backend.
# security/hardening: every negative C return code ⇒ ValueError
# unexpected positive non-zero ⇒ RuntimeError

from __future__ import annotations

import os
import sys
import importlib


# --------------------------------------------------------------------- helper
def _ensure_backend_cffi_loaded():
    """
    Attempt to import backend_cffi. If not found, append common build/product
    paths to sys.path and retry exactly once.                         # hardening
    """
    try:
        return importlib.import_module("backend_cffi")
    except ModuleNotFoundError:
        # Typical relative locations after 'build.sh' or manual builds
        here = os.path.dirname(__file__)
        project_root = os.path.abspath(os.path.join(here, "..", "..", ".."))
        candidates = [
            os.path.join(project_root, "build", "python"),
            os.path.join(project_root, "src", "python"),
            os.path.join(project_root, "src", "python", "ffi"),
        ]
        for path in candidates:
            if path not in sys.path and os.path.isdir(path):
                sys.path.insert(0, path)
        # retry exactly once
        return importlib.import_module("backend_cffi")


# import backend_cffi safely
_backend = _ensure_backend_cffi_loaded()
ffi, lib = _backend.ffi, _backend.lib  # type: ignore

# ------------------------------------------------------------------- wrapper
class AntNetWrapper:
    """
    AntNetWrapper: thin, pythonic façade over the native AntNet API.
    """

    # ---------------------------------------------------------------- ctor
    def __init__(self, node_count: int | None = None,
                 min_hops: int | None = None,
                 max_hops: int | None = None,
                 from_config: str | None = None):
        self.context_id: int | None = None

        if from_config:
            if not isinstance(from_config, str):
                raise ValueError("from_config must be a path.")
            rc = lib.antnet_init_from_config(from_config.encode("utf-8"))
            if rc < 0:
                raise ValueError(f"antnet_init_from_config failed with code {rc}")
            self.context_id = rc
        else:
            if None in (node_count, min_hops, max_hops):
                raise ValueError("node_count, min_hops, max_hops are mandatory.")
            rc = lib.antnet_initialize(node_count, min_hops, max_hops)
            if rc < 0:
                raise ValueError(f"antnet_initialize failed with code {rc}")
            self.context_id = rc

    # ------------------------------------------------------------ alt ctor
    @classmethod
    def from_config(cls, path: str) -> "AntNetWrapper":
        return cls(from_config=path)

    # ------------------------------------------------------ iteration
    def run_iteration(self) -> None:
        rc = lib.antnet_run_iteration(self.context_id)
        if rc < 0:
            raise ValueError(f"run_iteration failed with code {rc}")
        if rc > 0:
            raise RuntimeError(f"run_iteration returned unexpected code {rc}")

    # --------------------------------------------------- best-path
    def get_best_path_struct(self):
        max_nodes = 1024
        nodes_buf = ffi.new("int[]", max_nodes)
        len_ptr = ffi.new("int*")
        lat_ptr = ffi.new("int*")

        rc = lib.antnet_get_best_path(self.context_id, nodes_buf, max_nodes, len_ptr, lat_ptr)
        if rc < 0:
            raise ValueError(f"get_best_path failed with code {rc}")
        if rc > 0:
            raise RuntimeError(f"get_best_path returned unexpected code {rc}")

        return {
            "nodes": [nodes_buf[i] for i in range(len_ptr[0])],
            "total_latency": lat_ptr[0],
        }

    # ------------------------------------------------- topology update
    def update_topology(self, nodes, edges):
        """
        Replace node/edge arrays in the backend.
        NodeData currently exposes only node_id and delay_ms.
        """
        n, e = len(nodes), len(edges)
        node_arr = ffi.new("NodeData[]", n)
        edge_arr = ffi.new("EdgeData[]", e)

        for i, nd in enumerate(nodes):
            node_arr[i].node_id = nd["node_id"]
            node_arr[i].delay_ms = nd["delay_ms"]

        for j, ed in enumerate(edges):
            edge_arr[j].from_id = ed["from_id"]
            edge_arr[j].to_id = ed["to_id"]

        rc = lib.antnet_update_topology(self.context_id, node_arr, n, edge_arr, e)
        if rc < 0:
            raise ValueError(f"update_topology failed with code {rc}")
        if rc > 0:
            raise RuntimeError(f"update_topology returned unexpected code {rc}")

    # ---------------------------------------------- run all solvers
    def run_all_solvers(self):
        max_nodes = 1024
        a_nodes = ffi.new("int[]", max_nodes)
        r_nodes = ffi.new("int[]", max_nodes)
        b_nodes = ffi.new("int[]", max_nodes)

        a_len = ffi.new("int*"); r_len = ffi.new("int*"); b_len = ffi.new("int*")
        a_lat = ffi.new("int*"); r_lat = ffi.new("int*"); b_lat = ffi.new("int*")

        rc = lib.antnet_run_all_solvers(
            self.context_id,
            a_nodes, max_nodes, a_len, a_lat,
            r_nodes, max_nodes, r_len, r_lat,
            b_nodes, max_nodes, b_len, b_lat
        )
        if rc < 0:
            raise ValueError(f"run_all_solvers failed with code {rc}")
        if rc > 0:
            raise RuntimeError(f"run_all_solvers returned unexpected code {rc}")

        return {
            "aco":    {"nodes": [a_nodes[i] for i in range(a_len[0])], "total_latency": a_lat[0]},
            "random": {"nodes": [r_nodes[i] for i in range(r_len[0])], "total_latency": r_lat[0]},
            "brute":  {"nodes": [b_nodes[i] for i in range(b_len[0])], "total_latency": b_lat[0]},
        }

    # -------------------------------------------------------- shutdown
    def shutdown(self):
        if self.context_id is not None:
            lib.antnet_shutdown(self.context_id)
            self.context_id = None

    # ---------------------------------------------------- destructor
    def __del__(self):
        self.shutdown()
