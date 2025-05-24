# src/python/gui/fit_width_checkbox_manager.py
from ..widgets.fit_width_checkbox import FitWidthCheckBox

class FitWidthCheckBoxManager:
    """
    Manages multiple FitWidthCheckBox objects by column index.
    On resize, we adjust each checkbox's font and then reapply
    the original text to prevent content changes.
    """

    def __init__(self):
        self._column_map = {}     # column -> list of FitWidthCheckBox
        self._original_texts = {} # checkbox -> original text

    def register_checkbox(self, checkbox: FitWidthCheckBox, column: int):
        if column not in self._column_map:
            self._column_map[column] = []
        self._column_map[column].append(checkbox)
        self._original_texts[checkbox] = checkbox.text()

    def adjust_font_to_width(self):
        for checkboxes in self._column_map.values():
            for cb in checkboxes:
                cb.adjust_font_to_width()
                orig = self._original_texts[cb]
                cb.setText(orig)
