from .backend_cffi import ffi, lib

class AntNetWrapper:
    def __init__(self, node_count, min_hops, max_hops):
        self.ctx = lib.antnet_init(node_count, min_hops, max_hops)
        if self.ctx == ffi.NULL:
            raise RuntimeError("Failed to initialize AntNet context.")

    def run_iteration(self):
        lib.antnet_run_iteration(self.ctx)

    def get_best_path_struct(self):
        raw = lib.antnet_get_best_path_struct(self.ctx)
        if raw == ffi.NULL:
            return None
        # Read nodes array
        nodes = [raw.nodes[i] for i in range(raw.node_count)]
        return {
            "nodes": nodes,
            "total_latency": raw.total_latency
        }

    def shutdown(self):
        if self.ctx:
            lib.antnet_shutdown(self.ctx)
            self.ctx = None

    def __del__(self):
        self.shutdown()
