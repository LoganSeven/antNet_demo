# src/python/ffi/ffi_build.py
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

    int antnet_run_all_solvers(
        int context_id,
        int* out_nodes_aco,    int max_size_aco,    int* out_len_aco,    int* out_latency_aco,
        int* out_nodes_random, int max_size_random, int* out_len_random, int* out_latency_random,
        int* out_nodes_brute,  int max_size_brute,  int* out_len_brute,  int* out_latency_brute
    );

    // new config-based init
    int antnet_init_from_config(const char* config_path);

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
this_dir     = os.path.dirname(__file__)
src_c_dir    = os.path.abspath(os.path.join(this_dir, "../../c"))
include_dir  = os.path.abspath(os.path.join(this_dir, "../../../include"))

# Paths to all .c files that must be compiled
lib_source       = os.path.join(src_c_dir, "backend.c")
topo_source      = os.path.join(src_c_dir, "backend_topology.c")
random_source    = os.path.join(src_c_dir, "random_algo.c")
brute_source     = os.path.join(src_c_dir, "cpu_brute_force.c")
aco_source       = os.path.join(src_c_dir, "cpu_ACOv1.c")
config_mgr       = os.path.join(src_c_dir, "config_manager.c")
hopmap_source    = os.path.join(src_c_dir, "hop_map_manager.c")
ini_c            = os.path.join(this_dir, "../../../third_party/ini.c")  # If needed, but typically only config_manager needs it.

ffi.set_source(
    "backend_cffi",              # name of the generated module (.so/.pyd)
    '#include "backend.h"',      # top-level header
    sources=[
        lib_source,
        topo_source,
        random_source,
        brute_source,
        aco_source,
        config_mgr,
        hopmap_source,
        # We compile ini.c if it is not already compiled into a separate library.
        ini_c
    ],
    include_dirs=[include_dir, os.path.join(this_dir, "../../../third_party")],  # directories for #include
)

if __name__ == "__main__":
    build_dir = os.path.abspath(os.path.join(this_dir, "../../../build/python"))
    os.makedirs(build_dir, exist_ok=True)
    ffi.compile(verbose=True, tmpdir=build_dir)
