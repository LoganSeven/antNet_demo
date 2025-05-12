# src/python/core/worker.py

import time
from threading import Event
from qtpy.QtCore import QObject

from core.callback_adapter import QCCallbackToSignal
from ffi.backend_api import AntNetWrapper
from structs._generated.auto_structs import AppConfig

class Worker(QObject):
    """
    Worker: handles backend C operations asynchronously in a separate thread.
    Can be initialized either from an INI config file (preferred for consistency),
    or from a provided AppConfig dictionary.
    """

    def __init__(self, from_config: str | None = None, app_config: AppConfig | None = None):
        super().__init__()
        self._stop_event = Event()
        self.callback_adapter = QCCallbackToSignal()

        # Initialize the backend depending on init mode
        if from_config:
            self.backend = AntNetWrapper(from_config=from_config)
        else:
            if app_config is None:
                # Default AppConfig fallback
                app_config = {
                    "nb_swarms": 1,
                    "set_nb_nodes": 64,
                    "min_hops": 5,
                    "max_hops": 32,
                    "default_delay": 20,
                    "death_delay": 9999,
                    "under_attack_id": -1,
                    "attack_started": False,
                    "simulate_ddos": False,
                    "show_random_performance": True,
                    "show_brute_performance": True
                }
            self.backend = AntNetWrapper(app_config=app_config)

    def run(self):
        """
        Main loop for backend processing. Calls the backend to perform one
        iteration of each algorithm (ACO, random, brute) and emits results.
        """
        while not self._stop_event.is_set():
            time.sleep(1)

            result_dict = self.backend.run_all_solvers()

            self.callback_adapter.on_best_path_callback(result_dict)
            self.callback_adapter.on_iteration_callback()

    def stop(self):
        """
        Signal the worker to stop its main loop.
        """
        self._stop_event.set()

    def shutdown_backend(self):
        """
        Shut down the backend context safely and free all resources.
        """
        if self.backend is not None:
            self.backend.shutdown()
            self.backend = None

    def update_topology(self, nodes, edges):
        """
        Push a new topology into the backend (node list and edge list).
        """
        if self.backend:
            self.backend.update_topology(nodes, edges)
