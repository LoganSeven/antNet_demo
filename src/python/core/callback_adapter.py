# src/python/core/callback_adapter.py
from qtpy.QtCore import QObject, Signal, Slot

class QCCallbackToSignal(QObject):
    """
    Adapter between backend C/Worker callbacks and Qt signals for the UI.
    Ensures safe, thread-correct communication.
    """

    signal_best_path_updated = Signal(object)  # e.g., path_info dict
    signal_iteration_done = Signal()

    # NEW: signal to pass the pheromone matrix as a Python list[float]
    signal_pheromone_matrix = Signal(list)

    def __init__(self):
        super().__init__()

    @Slot(object)
    def on_best_path_callback(self, path_info):
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

    # NEW: slot to receive pheromone matrix from the worker
    @Slot(list)
    def on_pheromone_matrix_callback(self, matrix):
        """
        Called by backend/worker to send updated pheromone matrix.
        """
        self.signal_pheromone_matrix.emit(matrix)
