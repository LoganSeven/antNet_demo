# Relative Path: src/python/gui/widgets/fit_width_checkbox.py
from qtpy.QtWidgets import QCheckBox, QSizePolicy
from qtpy.QtCore    import Qt
from qtpy.QtGui     import QFontDatabase, QFont
from .utils.font_lut import size_for_width

class FitWidthCheckBox(QCheckBox):
    """
    QCheckBox that dynamically adjusts its font size to fit the current width.
    Uses size_for_width() for LUT-based interpolation.
    """

    _LUT = None

    def __init__(self, text="", parent=None):
        super().__init__(text, parent)
        fixed = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        self.setFont(fixed)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def adjust_font_to_width(self):
        """
        Computes the font size for the current width, then applies it.
        """
        aw = self.width()
        txt = self.text()
        if aw <= 0 or not txt:
            return

        new_size = size_for_width(aw, self._LUT)
        font = QFont(self.font())
        if hasattr(font, 'setPointSizeF'):
            font.setPointSizeF(new_size)
        else:
            font.setPointSize(int(round(new_size)))
        self.setFont(font)
