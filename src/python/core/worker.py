# src/python/core/worker.py

import time
from threading import Event
from qtpy.QtCore import QObject

from core.callback_adapter import QCCallbackToSignal
from ffi.backend_api import AntNetWrapper

class Worker(QObject):
    """
    Worker: handles backend C operations asynchronously in a separate thread.
    """

    def __init__(self):
        super().__init__()
        self._stop_event = Event()
        self.callback_adapter = QCCallbackToSignal()

        # Initialize backend C context
        self.backend = AntNetWrapper(node_count=10, min_hops=2, max_hops=5)

    def run(self):
        """
        Main loop for backend processing. Replaces the old iteration logic
        with a single call that runs aco, random, and brute simultaneously.
        """
        while not self._stop_event.is_set():
            time.sleep(0.5)

            # Run all three solvers in one go
            result_dict = self.backend.run_all_solvers()

            # Emit combined dictionary
            self.callback_adapter.on_best_path_callback(result_dict)

            # Emit iteration done
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
