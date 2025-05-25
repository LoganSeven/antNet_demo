# Relative Path: src/python/gui/widgets/tab_sheet_widget.py
from qtpy.QtWidgets import QTabWidget, QWidget
from qtpy.QtGui import QFontDatabase, QFont
from qtpy.QtCore import Qt
from .utils.font_lut import size_for_width

class TabSheetWidget(QTabWidget):
    """
    A QTabWidget subclass that adjusts its font size to fit the current width
    using a LUT and linear interpolation. Provides convenience methods to
    add/delete tabs and insert widgets on a specific tab.
    """

    _LUT = None

    def __init__(self, parent=None):
        super().__init__(parent)
        # Use the same system fixed-pitch font as other FitWidth controls
        fixed_font = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        self.setFont(fixed_font)

        # Allow it to expand horizontally
        self.setMinimumWidth(200)
        self.setTabPosition(QTabWidget.North)
        self.setElideMode(Qt.ElideNone)

    def add_tabsheet(self, title: str) -> int:
        """
        Creates a new tab with the given title and returns its index.
        """
        # A blank QWidget as placeholder container
        page = QWidget()
        idx = self.addTab(page, title)
        return idx

    def delete_tabsheet(self, index: int):
        """
        Removes the tab at the given index if valid. Otherwise does nothing.
        """
        if 0 <= index < self.count():
            self.removeTab(index)

    def add_widget_to_sheet(self, index: int, widget: QWidget):
        """
        Places 'widget' in the tab content at the specified index
        if it exists. Otherwise no-op.
        """
        if 0 <= index < self.count():
            page = self.widget(index)
            if page:
                page_layout = page.layout()
                if not page_layout:
                    # Only create a layout if none exists
                    from qtpy.QtWidgets import QVBoxLayout
                    page_layout = QVBoxLayout(page)
                    page.setLayout(page_layout)
                page_layout.addWidget(widget)

    def adjust_font_to_width(self):
        """
        Adjusts the tab bar font size based on the overall width of this QTabWidget.
        Uses the LUT from font_lut.py to compute the font point size.
        """
        aw = self.width()
        if aw <= 0:
            return

        new_size = size_for_width(aw, self._LUT)
        f = QFont(self.font())
        if hasattr(f, "setPointSizeF"):
            f.setPointSizeF(new_size)
        else:
            f.setPointSize(int(round(new_size)))
        self.setFont(f)

        # Force the tab bar to recalc
        if self.tabBar():
            self.tabBar().setFont(f)
