import os
from cffi import FFI

ffi = FFI()

# ---------- C header interface ----------
# Matches the current backend.h and backend_topology.h:
ffi.cdef(
    """
    // from backend.h
    int antnet_initialize(int node_count, int min_hops, int max_hops);
    int antnet_run_iteration(int context_id);
    int antnet_shutdown(int context_id);
    int antnet_get_best_path(
        int context_id,
        int* out_nodes,
        int max_size,
        int* out_path_len,
        int* out_total_latency
    );

    // from backend_topology.h
    typedef struct {
        int node_id;
        int delay_ms;
    } NodeData;

    typedef struct {
        int from_id;
        int to_id;
    } EdgeData;

    int antnet_update_topology(
        int context_id,
        const NodeData* nodes,
        int num_nodes,
        const EdgeData* edges,
        int num_edges
    );
    """
)

# ---------- Build instructions ----------
this_dir    = os.path.dirname(__file__)
lib_source  = os.path.abspath(os.path.join(this_dir, "../../c/backend.c"))
topo_source = os.path.abspath(os.path.join(this_dir, "../../c/backend_topology.c"))
include_dir = os.path.abspath(os.path.join(this_dir, "../../../include"))

ffi.set_source(
    "backend_cffi",               # name of the generated module (.so/.pyd)
    '#include "backend.h"',       # top-level header
    sources=[lib_source, topo_source],  # the .c files to compile
    include_dirs=[include_dir],         # directory for #include
)

if __name__ == "__main__":
    build_dir = os.path.abspath(os.path.join(this_dir, "../../../build/python"))
    os.makedirs(build_dir, exist_ok=True)
    ffi.compile(verbose=True, tmpdir=build_dir)
