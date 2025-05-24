from qtpy.QtWidgets import QGridLayout
from ..widgets.fit_width_button import FitWidthButton


class FitWidthButtonManager:
    """
    Aligns all FitWidthButtons in the same column by equalizing their text length.
    """

    def __init__(self):
        self._column_map = {}
        self._original_texts = {}

    def register_button(self, button: FitWidthButton, column: int):
        if column not in self._column_map:
            self._column_map[column] = []
        self._column_map[column].append(button)
        self._original_texts[button] = button.text()

    def adjust_font_to_width(self):
        for buttons in self._column_map.values():
            for btn in buttons:
                btn.adjust_font_to_width()
                orig = self._original_texts[btn]
                btn.setText(orig)