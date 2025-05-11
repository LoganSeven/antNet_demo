# src/python/ffi/backend_api.py

# High-level Python wrapper for the C AntNet backend.
# security/hardening: negative C return code ⇒ ValueError
# unexpected positive non-zero ⇒ RuntimeError
from __future__ import annotations

import os
import sys
import importlib

from consts._generated.error_codes_generated import ERR_SUCCESS

def _ensure_backend_cffi_loaded():
    """
    Attempt to import backend_cffi. If not found, append common build/product
    paths to sys.path and retry exactly once.                         # hardening
    """
    try:
        return importlib.import_module("backend_cffi")
    except ModuleNotFoundError:
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
        return importlib.import_module("backend_cffi")


_backend = _ensure_backend_cffi_loaded()
ffi, lib = _backend.ffi, _backend.lib  # type: ignore

class AntNetWrapper:
    """
    AntNetWrapper: thin, pythonic façade over the native AntNet API.
    This version allows three ways to initialize:
      (1) from_config="some.ini"
      (2) app_config=some_dict
      (3) node_count=..., min_hops=..., max_hops=...
    If more than one is provided, from_config has highest priority, then app_config,
    then node_count+min_hops+max_hops.
    """

    def __init__(
        self,
        node_count: int | None = None,
        min_hops: int | None = None,
        max_hops: int | None = None,
        from_config: str | None = None,
        app_config: dict | None = None,
    ):
        self.context_id: int | None = None

        # 1) If from_config is given
        if from_config is not None:
            if not isinstance(from_config, str):
                raise ValueError("from_config must be a string path to .ini")
            rc = lib.antnet_init_from_config(from_config.encode("utf-8"))
            if rc < 0:
                raise ValueError(f"antnet_init_from_config failed with code {rc}")
            self.context_id = rc
            return

        # 2) If app_config is given
        if app_config is not None:
            if not isinstance(app_config, dict):
                raise ValueError("app_config must be a dict (likely from TypedDict)")
            node_count = app_config["set_nb_nodes"]
            min_hops = app_config["min_hops"]
            max_hops = app_config["max_hops"]

            rc = lib.antnet_initialize(node_count, min_hops, max_hops)
            if rc < 0:
                raise ValueError(f"antnet_initialize failed with code {rc}")
            self.context_id = rc
            return

        # 3) node_count + min_hops + max_hops
        if node_count is not None and min_hops is not None and max_hops is not None:
            rc = lib.antnet_initialize(node_count, min_hops, max_hops)
            if rc < 0:
                raise ValueError(f"antnet_initialize failed with code {rc}")
            self.context_id = rc
            return

        raise ValueError(
            "AntNetWrapper requires either from_config=..., or app_config=..., "
            "or node_count/min_hops/max_hops."
        )

    @classmethod
    def from_config(cls, path: str) -> AntNetWrapper:
        return cls(from_config=path)

    def run_iteration(self) -> None:
        rc = lib.antnet_run_iteration(self.context_id)
        if rc == ERR_SUCCESS:
            return
        elif rc < 0:
            raise ValueError(f"run_iteration failed with code {rc}")
        else:
            raise RuntimeError(f"run_iteration returned unexpected code {rc}")

    def get_best_path_struct(self):
        max_nodes = 1024
        nodes_buf = ffi.new("int[]", max_nodes)
        len_ptr = ffi.new("int*")
        lat_ptr = ffi.new("int*")

        rc = lib.antnet_get_best_path(self.context_id, nodes_buf, max_nodes, len_ptr, lat_ptr)
        if rc == ERR_SUCCESS:
            return {
                "nodes": [nodes_buf[i] for i in range(len_ptr[0])],
                "total_latency": lat_ptr[0],
            }
        elif rc < 0:
            raise ValueError(f"get_best_path failed with code {rc}")
        else:
            raise RuntimeError(f"get_best_path returned unexpected code {rc}")

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
        if rc == ERR_SUCCESS:
            return
        elif rc < 0:
            raise ValueError(f"update_topology failed with code {rc}")
        else:
            raise RuntimeError(f"update_topology returned unexpected code {rc}")

    def run_all_solvers(self):
        """
        Runs the ACO, random, and brute force solvers in a single call,
        returning a dictionary with results for each approach.
        """
        max_nodes = 1024
        a_nodes = ffi.new("int[]", max_nodes)
        r_nodes = ffi.new("int[]", max_nodes)
        b_nodes = ffi.new("int[]", max_nodes)

        a_len = ffi.new("int*")
        r_len = ffi.new("int*")
        b_len = ffi.new("int*")
        a_lat = ffi.new("int*")
        r_lat = ffi.new("int*")
        b_lat = ffi.new("int*")

        rc = lib.antnet_run_all_solvers(
            self.context_id,
            a_nodes, max_nodes, a_len, a_lat,
            r_nodes, max_nodes, r_len, r_lat,
            b_nodes, max_nodes, b_len, b_lat
        )
        if rc == ERR_SUCCESS:
            return {
                "aco": {
                    "nodes": [a_nodes[i] for i in range(a_len[0])],
                    "total_latency": a_lat[0]
                },
                "random": {
                    "nodes": [r_nodes[i] for i in range(r_len[0])],
                    "total_latency": r_lat[0]
                },
                "brute": {
                    "nodes": [b_nodes[i] for i in range(b_len[0])],
                    "total_latency": b_lat[0]
                },
            }
        elif rc < 0:
            raise ValueError(f"run_all_solvers failed with code {rc}")
        else:
            raise RuntimeError(f"run_all_solvers returned unexpected code {rc}")

    def shutdown(self):
        if self.context_id is not None:
            rc = lib.antnet_shutdown(self.context_id)
            if rc == ERR_SUCCESS:
                self.context_id = None
            elif rc < 0:
                raise ValueError(f"antnet_shutdown failed with code {rc}")
            else:
                raise RuntimeError(f"antnet_shutdown returned unexpected code {rc}")

    def __del__(self):
        try:
            self.shutdown()
        except:
            pass
