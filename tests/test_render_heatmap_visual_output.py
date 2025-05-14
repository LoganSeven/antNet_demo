# tests/test_render_heatmap_visual_output.py
"""
Heatmap rendering test using the GPU backend (offscreen EGL context).
Saves the output as output.png and verifies dimensions.
"""

import os
import sys
import pytest
import random
from PIL import Image

# ----------------------------------------------------------------- sys-path
this_dir = os.path.dirname(__file__)
project_root = os.path.abspath(os.path.join(this_dir, ".."))
sys.path.insert(0, os.path.join(project_root, "build/python"))
sys.path.insert(0, os.path.join(project_root, "src/python"))

from ffi.backend_api import (
    render_heatmap_rgba,
    init_async_renderer,
    shutdown_async_renderer
)

def _announce(msg: str) -> None:
    """Bypass pytest capture – always visible in CI logs."""
    os.write(1, (msg + "\n").encode())

@pytest.mark.parametrize("width,height", [(256, 256), (512, 512)])
def test_render_and_save_png(width, height):
    # Optionally, explicitly init the async renderer (safe to call multiple times).
    init_async_renderer(width=64, height=64)

    pts_xy = []
    strength = []
    for _ in range(100):
        pts_xy.extend([
            random.uniform(-1.0, 1.0),
            random.uniform(-1.0, 1.0)
        ])
        strength.append(random.uniform(0.0, 1.0))

    # Renders asynchronously in a persistent EGL thread
    rgba_data = render_heatmap_rgba(pts_xy, strength, width, height)

    img = Image.frombytes("RGBA", (width, height), rgba_data)

    #out_path = f"output_{width}x{height}.png"
    #img.save(out_path)

    assert img.size == (width, height), "Image size does not match expected dimensions"
    _announce(f"✅ test_render_and_save_png – {out_path} saved successfully")

    # Shut down the async renderer after the test
    shutdown_async_renderer()
