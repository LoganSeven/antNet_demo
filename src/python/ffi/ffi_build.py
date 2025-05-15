import os
from cffi import FFI
from src.python.ffi.cdef_string import CDEF_SOURCE  # auto-generated structs + declarations

ffi = FFI()
ffi.cdef(CDEF_SOURCE)

# ---------- Build instructions ----------
this_dir    = os.path.dirname(__file__)
src_c_dir   = os.path.abspath(os.path.join(this_dir, "../../c"))
include_dir = os.path.abspath(os.path.join(this_dir, "../../../include"))
ini_c       = os.path.join(this_dir, "../../../third_party/ini.c")

ffi.set_source(
    "backend_cffi",
    f"""#include "./algo/cpu/cpu_ACOv1.h"
#include "antnet_aco_v1_types.h"
#include "antnet_brute_force_types.h"
#include "antnet_config_types.h"
#include "antnet_network_types.h"
#include "antnet_path_types.h"
#include "backend.h"
#include "backend_thread_defs.h"
#include "backend_topology.h"
#include "config_manager.h"
#include "cpu_brute_force.h"
#include "error_codes.h"
#include "heatmap_renderer.h"
#include "heatmap_renderer_async.h"
#include "hop_map_manager.h"
#include "random_algo.h"
""",
    sources=[
        os.path.join(src_c_dir, "heatmap_renderer_async.c"),
        os.path.join(src_c_dir, "random_algo.c"),
        os.path.join(src_c_dir, "backend.c"),
        os.path.join(src_c_dir, "cpu_brute_force.c"),
        os.path.join(src_c_dir, "heatmap_renderer.c"),
        os.path.join(src_c_dir, "backend_topology.c"),
        os.path.join(src_c_dir, "config_manager.c"),
        os.path.join(src_c_dir, "cpu_ACOv1.c"),
        os.path.join(src_c_dir, "hop_map_manager.c"),
        ini_c
    ],
    include_dirs=[
        include_dir,
        os.path.join(this_dir, "../../../third_party")
    ],
    libraries=["EGL", "GLESv2"],
)

if __name__ == "__main__":
    build_dir = os.path.abspath(os.path.join(this_dir, "../../../build/python"))
    os.makedirs(build_dir, exist_ok=True)
    ffi.compile(verbose=True, tmpdir=build_dir)
