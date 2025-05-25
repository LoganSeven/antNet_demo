# Relative Path: src/python/gui/widgets/utils/font_lut.py

# src/python/gui/widgets/utils/font_lut.py
from qtpy.QtGui import QFont

# Default LUT: list of (widget_width_px, font_point_size)
# Customize these values to tune mapping globally.
DEFAULT_LUT = [
    (120, 6),  (130, 7),  (140, 8),  (150, 9),  (160, 10),
    (170, 11), (180, 12), (190, 13), (200, 14), (210, 15),
    (220, 16), (230, 17), (240, 18), (250, 19), (260, 20),
    (270, 21), (280, 22), (290, 23), (300, 24),
]


def size_for_width(width_px: float, lut: list = None) -> float:
    """
    Given a widget width in pixels and a lookup table (list of (width, size)),
    clamp to endpoints and interpolate linearly between LUT entries.

    Args:
        width_px: current widget width in pixels
        lut: optional custom LUT, default is DEFAULT_LUT

    Returns:
        Interpolated font size in points (float).
    """
    table = lut or DEFAULT_LUT

    # Clamp below first entry
    if width_px <= table[0][0]:
        return table[0][1]
    # Clamp above last entry
    if width_px >= table[-1][0]:
        return table[-1][1]

    # Interpolate between adjacent entries
    for (w0, s0), (w1, s1) in zip(table, table[1:]):
        if width_px <= w1:
            t = (width_px - w0) / float(w1 - w0)
            return s0 + t * (s1 - s0)

    # Fallback to last entry
    return table[-1][1]
