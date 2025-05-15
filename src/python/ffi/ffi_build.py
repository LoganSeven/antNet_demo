# src/python/ffi/ffi_build.py

import os
from cffi import FFI

# Replace the relative import with an absolute import that matches your folder structure.
# Adjust "src.python.ffi.cdef_string" if your real package name differs.
from src.python.ffi.cdef_string import CDEF_SOURCE  # auto-generated struct + function declarations

ffi = FFI()
ffi.cdef(CDEF_SOURCE)

# ---------- Build instructions ----------
this_dir = os.path.dirname(__file__)
src_c_dir = os.path.abspath(os.path.join(this_dir, "../../c"))
include_dir = os.path.abspath(os.path.join(this_dir, "../../../include"))

lib_source    = os.path.join(src_c_dir, "backend.c")
topo_source   = os.path.join(src_c_dir, "backend_topology.c")
random_source = os.path.join(src_c_dir, "random_algo.c")
brute_source  = os.path.join(src_c_dir, "cpu_brute_force.c")
aco_source    = os.path.join(src_c_dir, "cpu_ACOv1.c")
config_mgr    = os.path.join(src_c_dir, "config_manager.c")
hopmap_source = os.path.join(src_c_dir, "hop_map_manager.c")
heatmap_renderer = os.path.join(src_c_dir, "heatmap_renderer.c")
heatmap_renderer_async = os.path.join(src_c_dir, "heatmap_renderer_async.c")  # new

ini_c         = os.path.join(this_dir, "../../../third_party/ini.c")

ffi.set_source(
    "backend_cffi",  # name of the generated Python extension module
    (
        '#include "backend.h"\n'
        '#include "backend_topology.h"\n'
        '#include "config_manager.h"\n'
        '#include "hop_map_manager.h"\n'
        '#include "algo/cpu/cpu_ACOv1.h"\n'
        '#include "cpu_brute_force.h"\n'
        '#include "random_algo.h"\n'
    ),
    sources=[
        lib_source,
        topo_source,
        random_source,
        brute_source,
        aco_source,
        config_mgr,
        hopmap_source,
        heatmap_renderer,
        heatmap_renderer_async,  # added
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
