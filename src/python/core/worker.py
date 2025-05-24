# src/python/core/worker.py
"""
Worker: handles backend C operations asynchronously in a separate thread.
Can be initialized either from an INI config file (preferred for consistency),
or from a provided AppConfig dictionary.
"""

import time
from threading import Event, Lock
from typing import cast
from qtpy.QtCore import QObject

from core.callback_adapter import QCCallbackToSignal
from ffi.backend_api import AntNetWrapper
from structs._generated.auto_structs import AppConfig, NodeData, EdgeData


class Worker(QObject):
    """
    Worker: handles backend C operations asynchronously in a separate thread.
    Can be initialized either from an INI config file (preferred for consistency),
    or from a provided AppConfig dictionary.
    """

    def __init__(self, from_config: str | None = None, app_config: AppConfig | None = None):
        super().__init__()
        self._stop_event = Event()
        self._ctx_lock = Lock()
        self._topology_ready = False

        self.callback_adapter = QCCallbackToSignal()

        if from_config:
            self.backend = AntNetWrapper(from_config=from_config)
        else:
            if app_config is None:
                app_config = cast(AppConfig, {
                    "nb_ants": 3,
                    "set_nb_nodes": 64,
                    "min_hops": 5,
                    "max_hops": 32,
                    "default_min_delay": 10,
                    "default_max_delay": 250,
                    "death_delay": 9999,
                    "under_attack_id": -1,
                    "attack_started": False,
                    "simulate_ddos": False,
                    "show_random_performance": True,
                    "show_brute_performance": True,
                    "ranking_alpha": 0.4,
                    "ranking_beta": 0.4,
                    "ranking_gamma": 0.2,
                    "ant_alpha": 1.0,
                    "ant_beta": 2.0,
                    "ant_Q": 500.0,
                    "ant_evaporation": 0.1
                })
            self.backend = AntNetWrapper(app_config=app_config)

    def run(self):
        """
        Main loop for backend processing. Waits until topology is ready.
        """
        while not self._stop_event.is_set():
            time.sleep(0.15)

            if not self._topology_ready:
                continue  # Wait until topology is updated

            try:
                result_dict = self.backend.run_all_solvers()
            except ValueError as e:
                print(f"[ERROR][Worker] run_all_solvers failed: {e}")
                continue

            with self._ctx_lock:
                try:
                    pheromones = self.backend.get_pheromone_matrix()
                except ValueError:
                    pheromones = []

            self.callback_adapter.on_best_path_callback(result_dict)
            self.callback_adapter.on_iteration_callback()
            self.callback_adapter.on_pheromone_matrix_callback(pheromones)
            try:
                ranking = self.backend.get_algo_ranking()
                self.callback_adapter.signal_ranking_updated.emit(ranking)
            except Exception as e:
                print(f"[ERROR][Worker] Failed to emit ranking: {e}")

    def stop(self):
        self._stop_event.set()

    def shutdown_backend(self):
        if self.backend is not None:
            self.backend.shutdown()
            self.backend = None

    def update_topology(self, nodes: list[NodeData], edges: list[EdgeData]):
        """
        Push a new topology into the backend (node list and edge list).
        """
        if self.backend:
            self.backend.update_topology(nodes, edges)
            self._topology_ready = True
