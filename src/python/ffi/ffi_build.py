from cffi import FFI
import os

ffi = FFI()

# ---------- C header interface ----------
ffi.cdef("""
    typedef struct AntNetContext AntNetContext;

    typedef struct {
        int* nodes;
        int  node_count;
        int  total_latency;
    } AntNetPathInfo;

    AntNetContext* antnet_init(int node_count, int min_hops, int max_hops);
    void            antnet_run_iteration(AntNetContext* ctx);
    const AntNetPathInfo* antnet_get_best_path_struct(AntNetContext* ctx);
    void            antnet_shutdown(AntNetContext* ctx);
""")

# ---------- Build instructions ----------
this_dir   = os.path.dirname(__file__)
lib_source = os.path.abspath(os.path.join(this_dir, "../../c/backend.c"))
include_dir= os.path.abspath(os.path.join(this_dir, "../../../include"))

ffi.set_source(
    "backend_cffi",               # module name
    '#include "backend.h"',       # C header to include
    sources=[lib_source],
    include_dirs=[include_dir],
)

if __name__ == "__main__":
    build_dir = os.path.abspath(os.path.join(this_dir, "../../../build/python"))
    os.makedirs(build_dir, exist_ok=True)
    ffi.compile(verbose=True, tmpdir=build_dir)
