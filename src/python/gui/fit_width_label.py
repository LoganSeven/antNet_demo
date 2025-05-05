from qtpy.QtWidgets import QLabel, QSizePolicy
from qtpy.QtCore import Qt
from qtpy.QtGui import QFontDatabase, QFont
from .font_lut import size_for_width  # import the LUT interpolation utility

class FitWidthLabel(QLabel):
    """
    A label that adjusts its font size to fit its current width using
    a hard-coded LUT and linear interpolation from font_lut.py.
    Values outside the LUT range clamp to the endpoints without further resizing.

    Override class-level _LUT if you need a custom mapping.
    """

    # Optional per-instance LUT override; defaults to module's DEFAULT_LUT
    _LUT = None

    def __init__(self, text="", parent=None):
        super().__init__(text, parent)
        # Use system fixed-pitch font for cross-platform consistency
        fixed = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        self.setFont(fixed)

        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)

    def adjust_font_to_width(self):
        """
        Uses size_for_width() to compute point size, then applies it.
        """
        aw = self.width()
        txt = self.text()
        if aw <= 0 or not txt:
            return

        # Choose LUT: instance override or module default
        lut = self._LUT
        new_size = size_for_width(aw, lut)

        # Apply the computed size
        font = QFont(self.font())
        # Try float-size if available
        if hasattr(font, 'setPointSizeF'):
            font.setPointSizeF(new_size)
        else:
            font.setPointSize(int(round(new_size)))
        self.setFont(font)
