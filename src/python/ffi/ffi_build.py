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
    f"""#include "core/backend.h"
#include "algo/cpu/cpu_ACOv1.h"
#include "algo/cpu/cpu_brute_force.h"
#include "algo/cpu/random_algo.h"
#include "backend_thread_defs.h"
#include "cffi_entrypoint.h"
#include "consts/error_codes.h"
#include "core/backend_topology.h"
#include "managers/config_manager.h"
#include "managers/hop_map_manager.h"
#include "rendering/heatmap_renderer.h"
#include "rendering/heatmap_renderer_async.h"
#include "types/antnet_aco_v1_params.h"
#include "types/antnet_aco_v1_types.h"
#include "types/antnet_brute_force_types.h"
#include "types/antnet_config_types.h"
#include "types/antnet_network_types.h"
#include "types/antnet_path_types.h"
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
