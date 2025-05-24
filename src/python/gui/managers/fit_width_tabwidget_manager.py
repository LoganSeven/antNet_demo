# src/python/gui/managers/fit_width_tabwidget_manager.py
from ..widgets.tab_sheet_widget import TabSheetWidget

class FitWidthTabWidgetManager:
    """
    Manages one or more TabSheetWidget instances, ensuring that each tab
    can recalc its font size on demand (like FitWidthLabel or FitWidthButton).
    We also restore the original tab text each time, preserving content.
    """

    def __init__(self):
        self._tabwidgets = []
        # For each registered TabSheetWidget, we store a list of tab titles
        # in the order they were added.
        self._original_titles_map = {}

    def register_tabwidget(self, tabwidget: TabSheetWidget):
        """
        Registers a TabSheetWidget for resizing. We record the current tab titles
        so they can be restored after adjusting fonts.
        """
        if tabwidget not in self._tabwidgets:
            self._tabwidgets.append(tabwidget)
            titles = []
            for i in range(tabwidget.count()):
                titles.append(tabwidget.tabText(i))
            self._original_titles_map[tabwidget] = titles

    def adjust_font_to_width(self):
        """
        Calls `adjust_font_to_width()` on each tab widget, then re-applies
        the saved tab titles to preserve text consistency under certain QTabWidget
        paint/resize edge cases.
        """
        for tw in self._tabwidgets:
            tw.adjust_font_to_width()
            # Re-apply stored tab titles
            saved_titles = self._original_titles_map.get(tw, [])
            if len(saved_titles) == tw.count():
                for i, t in enumerate(saved_titles):
                    tw.setTabText(i, t)
            else:
                # If the number of tabs changed in the meantime,
                # we skip re-applying any titles that no longer match.
                pass
