from qtpy.QtWidgets import QPushButton, QSizePolicy
from qtpy.QtGui     import QFontDatabase, QFont
from .font_lut     import size_for_width

class FitWidthButton(QPushButton):
    """
    QPushButton that dynamically adjusts its font size to fit the current width.
    """

    # Make sure this lives at class scope!
    _LUT = None

    def __init__(self, text="", parent=None):
        super().__init__(text, parent)
        fixed = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        self.setFont(fixed)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def adjust_font_to_width(self):
        aw = self.width()
        txt = self.text()
        if aw <= 0 or not txt:
            return

        new_size = size_for_width(aw, self._LUT)
        f = QFont(self.font())
        if hasattr(f, "setPointSizeF"):
            f.setPointSizeF(new_size)
        else:
            f.setPointSize(int(round(new_size)))
        self.setFont(f)
