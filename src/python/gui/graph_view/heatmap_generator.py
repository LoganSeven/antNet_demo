# src/python/gui/graph_view/heatmap_generator.py
"""
Heatmap generator for the ACO pheromone matrix using one square per node.
Each node emits a visual square beneath its position, with color and intensity
mapped from its outgoing pheromone strength.
This avoids all interpolation and uses pure Qt painting for maximum performance.
"""

from qtpy.QtGui import QPixmap, QImage, QPainter, QColor
from qtpy.QtCore import Qt


def _jet_color(value: float, vmin: float, vmax: float, alpha: int) -> QColor:
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

    return QColor(int(r * 255), int(g * 255), int(b * 255), alpha)


def generate_heatmap(
    pheromone_matrix: list[float],
    node_positions: list[tuple[float, float]] | None = None,
    size_factor: float = 1.25
) -> QPixmap:
    """
    generate_heatmap: draws a square under each node based on its total outgoing pheromone.
    This version avoids interpolation, matplotlib, or slow raster loops.
    Each node emits a visible square scaled by size_factor and colored via a jet map.

    Parameters:
    - pheromone_matrix: flat list of n*n floats, from get_pheromone_matrix()
    - node_positions: (x, y) positions for each node, index-matched with matrix rows
    - size_factor: multiplier applied to default square size

    Returns:
    - QPixmap containing the heatmap layer
    """
    print("Entering generate_heatmap")

    if not pheromone_matrix or not node_positions:
        return QPixmap()

    n = len(node_positions)
    if n * n != len(pheromone_matrix):
        return QPixmap()

    # Compute outgoing pheromone per node using max instead of sum
    out_vals = [0.0] * n
    for i in range(n):
        out_vals[i] = max(pheromone_matrix[i * n + j] for j in range(n) if i != j)

    # Normalize range
    non_zero = [v for v in out_vals if v > 0.0]
    if not non_zero:
        return QPixmap()
    vmin, vmax = min(non_zero), max(non_zero)
    if vmax - vmin < 1e-6:
        vmax = vmin + 1e-6

    # Estimate image bounds
    max_x = max(x for x, _ in node_positions)
    max_y = max(y for _, y in node_positions)
    width = int(max_x + 100)
    height = int(max_y + 100)

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
        color = _jet_color(strength, vmin, vmax, alpha=150)
        painter.setBrush(color)
        painter.setPen(Qt.NoPen)
        half = square_size // 2
        painter.drawRect(int(x) - half, int(y) - half, square_size, square_size)
        count_drawn += 1

    painter.end()
    print(f"[DEBUG] Squares drawn: {count_drawn}, range=[{vmin:.4f}, {vmax:.4f}]")
    return QPixmap.fromImage(image)
