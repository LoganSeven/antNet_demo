from qtpy.QtWidgets import QSpinBox
from qtpy.QtGui import QFontMetrics

class UniformSpinBoxManager:
    """
    Manages QSpinBox objects by column in a grid layout.
    Ensures each column is wide enough to display the spin boxes' maximum values,
    plus a margin for spacing and arrow controls.
    """

    def __init__(self, grid_layout):
        """
        grid_layout: the QGridLayout in which spin boxes are placed.
        """
        self._layout = grid_layout
        self._column_map = {}

    def register_spinbox(self, spinbox, column):
        """
        Registers a spin box at the specified column index.
        Multiple spin boxes can be in the same column.
        """
        if column not in self._column_map:
            self._column_map[column] = []
        self._column_map[column].append(spinbox)

    def update_column_widths(self, margin=10):
        """
        For each column containing registered spin boxes, calculates the needed width
        and applies it via setColumnMinimumWidth.
        
        margin: extra space added to accommodate spacing, arrow controls, or suffixes.
        """
        for column, spinboxes in self._column_map.items():
            max_width = 0
            for sp in spinboxes:
                # Acquire the current font metrics
                fm = QFontMetrics(sp.font())
                # Convert the maximum integer range to text for width calculations
                max_text = str(sp.maximum())
                # Spin boxes typically need extra space for the up/down arrows
                # A small margin can also be added for comfort
                text_width = fm.width(max_text)
                needed_width = text_width + margin + 20  # 20 for arrow area, tweak if needed
                if needed_width > max_width:
                    max_width = needed_width

            self._layout.setColumnMinimumWidth(column, max_width)
