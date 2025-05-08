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
    """

    def __init__(self):
        super().__init__()
        self._stop_event = Event()
        self.callback_adapter = QCCallbackToSignal()

        # Example usage of the auto-generated AppConfig typed dict.
        # A minimal default config is provided here for demonstration.
        default_config: AppConfig = {
            "nb_swarms": 1,
            "set_nb_nodes": 10,
            "min_hops": 2,
            "max_hops": 5,
            "default_delay": 20,
            "death_delay": 9999,
            "under_attack_id": -1,
            "attack_started": False,
            "simulate_ddos": False,
            "show_random_performance": True,
            "show_brute_performance": True
        }

        # Initialize the backend C context using the typed dict.
        # If you want to load settings from a file instead, pass from_config="path/to.ini"
        # to the AntNetWrapper constructor.
        self.backend = AntNetWrapper(app_config=default_config)

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
