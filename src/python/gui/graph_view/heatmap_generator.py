# src/python/gui/graph_view/heatmap_generator.py
"""
Heatmap generator for the ACO pheromone matrix using one square per node.
Each node emits a visual square beneath its position, with color and intensity
mapped from its outgoing pheromone strength.
This avoids all interpolation and uses pure Qt painting for maximum performance.

Also supports GPU-based (OpenGL) offscreen rendering via ffi.backend_api.
"""

import numpy as np
from qtpy.QtGui import QPixmap, QImage
from qtpy.QtCore import Qt
from ffi.backend_api import render_heatmap_rgba

def rgba_to_qpixmap(rgba_data, width, height):
    """
    Creates a QPixmap from the raw RGBA bytes without a BytesIO intermediate,
    then flips top-to-bottom because glReadPixels is upside-down for Qt usage.
    """
    if not rgba_data or len(rgba_data) < (width * height * 4):
        return QPixmap()

    # Create a QImage from the raw bytes
    # stride = 4 * width for RGBA8888
    image = QImage(rgba_data, width, height, 4 * width, QImage.Format_RGBA8888)

    # Flip vertically
    image = image.mirrored(False, True)

    return QPixmap.fromImage(image)


def _jet_color(value: float, vmin: float, vmax: float, alpha: int):
    """
    Basic jet-style colormap: maps value in [vmin..vmax] to RGB with given alpha.
    Used to assign visible meaning to pheromone strength values.
    """
    if vmax - vmin < 1e-6:
        ratio = 0.0
    else:
        ratio = max(0.0, min(1.0, (value - vmin) / (vmax - vmin)))

    if ratio < 0.25:
        r, g, b = 0, 4 * ratio, 1
    elif ratio < 0.5:
        r, g, b = 0, 1, 1 + 4 * (0.25 - ratio)
    elif ratio < 0.75:
        r, g, b = 4 * (ratio - 0.5), 1, 0
    else:
        r, g, b = 1, 1 + 4 * (0.75 - ratio), 0

    return (int(r * 255), int(g * 255), int(b * 255), alpha)


def generate_heatmap(
    pheromone_matrix: list[float],
    node_positions: list[tuple[float, float]] | None = None,
    size_factor: float = 1.25
) -> QPixmap:
    """
    generate_heatmap: draws a square under each node based on its total outgoing pheromone.
    This version avoids interpolation, matplotlib, or slow raster loops.
    Each node emits a visible square scaled by size_factor and colored via a jet map.
    Returns a QPixmap for overlay in the scene.
    """
    print("Entering generate_heatmap")

    from qtpy.QtGui import QImage, QPainter, QColor
    if not pheromone_matrix or not node_positions:
        return QPixmap()

    n = len(node_positions)
    if n * n != len(pheromone_matrix):
        return QPixmap()

    # Compute outgoing pheromone per node (max over row, ignoring self).
    out_vals = [0.0] * n
    for i in range(n):
        out_vals[i] = max(pheromone_matrix[i*n + j] for j in range(n) if j != i)

    non_zero = [v for v in out_vals if v > 0.0]
    if not non_zero:
        return QPixmap()
    vmin, vmax = min(non_zero), max(non_zero)
    if vmax - vmin < 1e-6:
        vmax = vmin + 1e-6

    max_x = max(x for x, _ in node_positions)
    max_y = max(y for _, y in node_positions)
    width = int(max_x + 100)
    height = int(max_y + 100)
    if width < 1 or height < 1:
        return QPixmap()

    image = QImage(width, height, QImage.Format_ARGB32_Premultiplied)
    image.fill(Qt.transparent)

    painter = QPainter(image)
    painter.setRenderHint(QPainter.Antialiasing, False)

    base_size = 30
    square_size = int(base_size * size_factor)

    count_drawn = 0
    for i, (x, y) in enumerate(node_positions):
        strength = out_vals[i]
        if strength <= 0:
            continue
        r, g, b, a = _jet_color(strength, vmin, vmax, 150)
        color = QColor(r, g, b, a)
        painter.setBrush(color)
        painter.setPen(Qt.NoPen)
        half = square_size // 2
        painter.drawRect(int(x) - half, int(y) - half, square_size, square_size)
        count_drawn += 1

    painter.end()
    #print(f"[DEBUG] Squares drawn: {count_drawn}, range=[{vmin:.4f}, {vmax:.4f}]")
    return QPixmap.fromImage(image)


def prepare_heatmap_input(pheromone_matrix, node_positions, width=800, height=600):
    """
    Prepares the data for GPU offscreen rendering. Scales node coordinates
    into clip space [-1..1], normalizes the pheromone values, then calls
    render_heatmap_rgba. Returns the raw RGBA bytes and the used w/h.
    """
    if not pheromone_matrix or not node_positions:
        return b"", 0, 0

    n = len(node_positions)
    if n * n != len(pheromone_matrix):
        return b"", 0, 0

    # Max outgoing pheromone per node (excluding self)
    out_vals = [max(pheromone_matrix[i * n + j] for j in range(n) if j != i) for i in range(n)]
    non_zero = [v for v in out_vals if v > 0.0]
    if not non_zero:
        return b"", 0, 0

    # Normalize
    vmin = min(non_zero)
    vmax = max(non_zero)
    if vmax - vmin < 1e-6:
        vmax = vmin + 1e-6
    norm_vals = [(v - vmin) / (vmax - vmin) for v in out_vals]

    # Map each (x, y) from scene coords to OpenGL clip space
    def to_clip_space(x, y):
        clip_x = 2.0 * x / width - 1.0
        clip_y = 1.0 - 2.0 * y / height
        return (clip_x, clip_y)

    pts_xy = []
    for (x, y) in node_positions:
        cx, cy = to_clip_space(x, y)
        pts_xy.append(cx)
        pts_xy.append(cy)

    rgba_data = render_heatmap_rgba(pts_xy, norm_vals, width, height)
    return rgba_data, width, height


def generate_heatmap_gl(
    pheromone_matrix: list[float],
    node_positions: list[tuple[float, float]],
    width: int,
    height: int
) -> QPixmap:
    """
    Uses the backend OpenGL pipeline (via render_heatmap_rgba) to create the heatmap.
    Then converts the returned RGBA bytes to QPixmap.
    """
    rgba_data, real_w, real_h = prepare_heatmap_input(pheromone_matrix, node_positions, width, height)
    if not rgba_data:
        return QPixmap()
    return rgba_to_qpixmap(rgba_data, real_w, real_h)
