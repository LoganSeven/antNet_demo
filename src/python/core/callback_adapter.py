# Relative Path: src/python/core/callback_adapter.py
"""
QCCallbackToSignal bridges C/Worker callbacks to Qt signals, enabling thread-safe UI updates.
Converts direct solver notifications into Qt signals consumable by the main thread.
"""

from typing import List
from qtpy.QtCore import QObject, Signal, Slot

from structs._generated.auto_structs import AntNetPathInfo, RankingEntry


class QCCallbackToSignal(QObject):
    """
    Adapter between backend C/Worker callbacks and Qt signals for the UI.
    Ensures safe, thread-correct communication.
    """

    signal_best_path_updated = Signal(dict)          # AntNetPathInfo (TypedDict emits as dict)
    signal_iteration_done = Signal()
    signal_pheromone_matrix = Signal(list)           # raw list, no generic
    signal_ranking_updated = Signal(list)            # raw list, for list[RankingEntry]

    def __init__(self):
        super().__init__()

    @Slot(dict)
    def on_best_path_callback(self, path_info: AntNetPathInfo):
        """
        Called by backend/worker to notify best path update.
        """
        self.signal_best_path_updated.emit(path_info)

    @Slot()
    def on_iteration_callback(self):
        """
        Called by backend/worker to notify end of one iteration.
        """
        self.signal_iteration_done.emit()

    @Slot(list)
    def on_pheromone_matrix_callback(self, matrix: List[float]):
        """
        Called by backend/worker to send updated pheromone matrix.
        """
        self.signal_pheromone_matrix.emit(matrix)
