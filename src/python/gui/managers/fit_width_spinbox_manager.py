# Relative Path: src/python/gui/managers/fit_width_spinbox_manager.py
from ..widgets.fit_width_spinbox import FitWidthSpinBox

class FitWidthSpinBoxManager:
    """
    Manages multiple FitWidthSpinBox instances in columns.
    Ensures each spinbox's font is resized using its LUT logic.
    We also reapply the original text so that changes to the display
    (caused by resizing) do not overwrite user content.
    """

    def __init__(self):
        self._column_map = {}     # column -> list of FitWidthSpinBox
        self._original_texts = {} # spinbox -> original text

    def register_spinbox(self, spinbox: FitWidthSpinBox, column: int):
        if column not in self._column_map:
            self._column_map[column] = []
        self._column_map[column].append(spinbox)

        # Store the initial text displayed in the spinbox
        self._original_texts[spinbox] = spinbox.text()

    def adjust_font_to_width(self):
        """
        For each spinbox in every column:
         1) Perform the adjust_font_to_width() call so LUT resizing is applied
         2) Re-apply the original text so the user sees unchanged content
        """
        for spinboxes in self._column_map.values():
            for spn in spinboxes:
                spn.adjust_font_to_width()
                orig = self._original_texts[spn]
                # QSpinBox requires lineEdit() to set the actual text
                spn.lineEdit().setText(orig)
