# src/python/core/worker.py

import time
from threading import Event
from qtpy.QtCore import QObject

from core.callback_adapter import QCCallbackToSignal
from ffi.backend_api import AntNetWrapper

class Worker(QObject):
    """
    Worker: handles backend C operations asynchronously in a separate thread.
    Calls C functions via CFFI and emits Qt signals via a callback adapter.
    """

    def __init__(self):
        super().__init__()
        self._stop_event = Event()
        self.callback_adapter = QCCallbackToSignal()

        # Initialize backend C context
        self.backend = AntNetWrapper(node_count=10, min_hops=2, max_hops=5)

    def run(self):
        """
        Main loop for backend processing.
        """
        while not self._stop_event.is_set():
            time.sleep(0.5)  # Control loop pacing

            # Run one iteration
            self.backend.run_iteration()

            # Fetch best path
            path_info = self.backend.get_best_path_struct()

            # Emit "best path updated"
            if path_info is not None:
                self.callback_adapter.on_best_path_callback(path_info)

            # Emit "iteration done"
            self.callback_adapter.on_iteration_callback()

    def stop(self):
        """
        Signal the worker to stop its main loop.
        """
        self._stop_event.set()

    def shutdown_backend(self):
        """
        Shut down the backend if it exists.
        """
        if self.backend is not None:
            self.backend.shutdown()
            self.backend = None

    def update_topology(self, nodes, edges):
        """
        Update the topology in the backend context.
        """
        if self.backend:
            self.backend.update_topology(nodes, edges)
