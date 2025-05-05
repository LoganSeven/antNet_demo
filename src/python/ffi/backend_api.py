# src/python/ffi/backend_api.py

from .backend_cffi import ffi, lib

class AntNetWrapper:
    """
    AntNetWrapper: provides a high-level Python interface to the C backend.
    Uses integer context IDs and exposes best path information as a dict.
    """

    def __init__(self, node_count, min_hops, max_hops):
        self.context_id = lib.antnet_initialize(node_count, min_hops, max_hops)
        if self.context_id < 0:
            raise RuntimeError("Failed to initialize AntNet context.")

    def run_iteration(self):
        rc = lib.antnet_run_iteration(self.context_id)
        if rc < 0:
            raise RuntimeError("Backend failed during run_iteration")

    def get_best_path_struct(self):
        """
        Calls C backend to retrieve best path data.
        Returns a dictionary with 'nodes' and 'total_latency', or None on error.
        """
        max_nodes = 1024
        out_nodes = ffi.new("int[]", max_nodes)
        out_path_len = ffi.new("int*")
        out_total_latency = ffi.new("int*")

        rc = lib.antnet_get_best_path(
            self.context_id,
            out_nodes,
            max_nodes,
            out_path_len,
            out_total_latency
        )

        if rc < 0:
            return None

        nodes = [out_nodes[i] for i in range(out_path_len[0])]
        return {
            "nodes": nodes,
            "total_latency": out_total_latency[0]
        }

    def update_topology(self, nodes, edges):
        """
        Calls the antnet_update_topology function to replace node/edge data.
        """
        num_nodes = len(nodes)
        num_edges = len(edges)

        # Create C arrays
        node_array = ffi.new("NodeData[]", num_nodes)
        edge_array = ffi.new("EdgeData[]", num_edges)

        for i, nd in enumerate(nodes):
            node_array[i].node_id = nd["node_id"]
            node_array[i].delay_ms = nd["delay_ms"]

        for j, ed in enumerate(edges):
            edge_array[j].from_id = ed["from_id"]
            edge_array[j].to_id   = ed["to_id"]

        rc = lib.antnet_update_topology(
            self.context_id,
            node_array,
            num_nodes,
            edge_array,
            num_edges
        )
        if rc < 0:
            raise RuntimeError("Backend failed to update topology")

    def shutdown(self):
        if self.context_id is not None:
            lib.antnet_shutdown(self.context_id)
            self.context_id = None

    def __del__(self):
        # Ensure context is cleaned up
        self.shutdown()
