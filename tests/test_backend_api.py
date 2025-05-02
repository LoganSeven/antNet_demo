# tests/test_backend_api.py
import os
import sys

# Add build and src/python paths dynamically
this_dir = os.path.dirname(__file__)
project_root = os.path.abspath(os.path.join(this_dir, ".."))
sys.path.insert(0, os.path.join(project_root, "build/python"))
sys.path.insert(0, os.path.join(project_root, "src/python"))

from ffi.backend_api import AntNetWrapper
from backend_cffi import ffi, lib

def test_basic_backend_integration():
    wrapper = AntNetWrapper(10, 2, 5)
    wrapper.run_iteration()
    result = wrapper.get_best_path_struct()

    assert result is not None
    assert isinstance(result["nodes"], list)
    assert isinstance(result["total_latency"], int)
    assert len(result["nodes"]) == 4
    wrapper.shutdown()
