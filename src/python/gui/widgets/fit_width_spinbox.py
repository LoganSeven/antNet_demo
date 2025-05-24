# src/python/gui/widgets/fit_width_spinbox.py
from qtpy.QtWidgets import QSpinBox, QSizePolicy
from qtpy.QtCore    import Qt
from qtpy.QtGui     import QFontDatabase, QFont
from .utils.font_lut import size_for_width

class FitWidthSpinBox(QSpinBox):
    """
    QSpinBox that dynamically adjusts its font size to fit its current width.
    Uses a LUT from font_lut.py for interpolation.
    Values outside the LUT range clamp to endpoints without further resizing.
    """

    _LUT = None

    def __init__(self, parent=None):
        super().__init__(parent)
        fixed = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        self.setFont(fixed)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def adjust_font_to_width(self):
        """
        Uses size_for_width() to compute point size, then applies it.
        """
        aw = self.width()
        if aw <= 0:
            return

        new_size = size_for_width(aw, self._LUT)
        font = QFont(self.font())
        if hasattr(font, 'setPointSizeF'):
            font.setPointSizeF(new_size)
        else:
            font.setPointSize(int(round(new_size)))
        self.setFont(font)
