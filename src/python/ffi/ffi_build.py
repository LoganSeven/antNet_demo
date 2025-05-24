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
#include "algo/cpu/cpu_ACOv1_path_reorder.h"
#include "algo/cpu/cpu_ACOv1_shared_structs.h"
#include "algo/cpu/cpu_ACOv1_threaded.h"
#include "algo/cpu/cpu_brute_force.h"
#include "algo/cpu/cpu_random_algo.h"
#include "algo/cpu/cpu_random_algo_path_reorder.h"
#include "cffi_entrypoint.h"
#include "consts/error_codes.h"
#include "core/backend_init.h"
#include "core/backend_params.h"
#include "core/backend_solvers.h"
#include "core/backend_thread_defs.h"
#include "core/backend_topology.h"
#include "managers/config_manager.h"
#include "managers/cpu_acoV1_algo_manager.h"
#include "managers/cpu_brute_force_algo_manager.h"
#include "managers/cpu_random_algo_manager.h"
#include "managers/hop_map_manager.h"
#include "managers/ranking_manager.h"
#include "rendering/heatmap_renderer.h"
#include "rendering/heatmap_renderer_async.h"
#include "types/antnet_aco_v1_params.h"
#include "types/antnet_aco_v1_types.h"
#include "types/antnet_brute_force_types.h"
#include "types/antnet_config_types.h"
#include "types/antnet_network_types.h"
#include "types/antnet_path_types.h"
#include "types/antnet_ranking_types.h"
#include "types/antnet_sasa_types.h"
""",
    sources=[
        os.path.join(src_c_dir, "managers/cpu_acoV1_algo_manager.c"),
        os.path.join(src_c_dir, "managers/ranking_manager.c"),
        os.path.join(src_c_dir, "managers/cpu_random_algo_manager.c"),
        os.path.join(src_c_dir, "managers/config_manager.c"),
        os.path.join(src_c_dir, "managers/cpu_brute_force_algo_manager.c"),
        os.path.join(src_c_dir, "managers/hop_map_manager.c"),
        os.path.join(src_c_dir, "core/backend.c"),
        os.path.join(src_c_dir, "core/backend_params.c"),
        os.path.join(src_c_dir, "core/backend_topology.c"),
        os.path.join(src_c_dir, "core/backend_init.c"),
        os.path.join(src_c_dir, "core/backend_solvers.c"),
        os.path.join(src_c_dir, "rendering/heatmap_renderer_async.c"),
        os.path.join(src_c_dir, "rendering/heatmap_renderer.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_ACOv1_shared_structs.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_brute_force.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_ACOv1_threaded.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_random_algo_path_reorder.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_ACOv1_path_reorder.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_ACOv1.c"),
        os.path.join(src_c_dir, "algo/cpu/cpu_random_algo.c"),
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
