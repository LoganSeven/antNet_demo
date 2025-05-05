from qtpy.QtWidgets import QGridLayout
from gui.fit_width_label import FitWidthLabel


class FitWidthLabelManager:
    """
    Manages a set of FitWidthLabel objects arranged by column in a grid layout.
    Ensures all labels in the same column have text padded to the same length
    (same number of characters).
    """

    def __init__(self):
        """
        grid_layout: the QGridLayout that the labels live in.
        """
        self._column_map = {}    # column_index -> list of labels
        self._original_texts = {}  # label -> original text (before padding)

    def register_label(self, label: FitWidthLabel, column: int):
        """
        Registers a FitWidthLabel at the specified column index.
        The manager will handle padding among all labels in this column.
        """
        if column not in self._column_map:
            self._column_map[column] = []
        self._column_map[column].append(label)

        # Store the original text for future re-padding
        self._original_texts[label] = label.text()


    def adjust_font_to_width(self):
        for column, labels in self._column_map.items():
            for lbl in labels:
                lbl.adjust_font_to_width()
                orig = self._original_texts[lbl]
                lbl.setText(orig)