# Relative Path: src/python/core/core_manager.py
"""
CoreManager orchestrates one or more Worker instances in separate threads.
Facilitates starting/stopping workers, distributing topology updates, and retrieving adapters.
Primary controller for multi-threaded backend usage in the Python GUI.
"""

from typing import TypedDict, List
from qtpy.QtCore import QObject, QThread

from core.worker import Worker
from structs._generated.auto_structs import NodeData, EdgeData


class TopologyData(TypedDict):
    nodes: List[NodeData]
    edges: List[EdgeData]


class CoreManager(QObject):
    """
    CoreManager: manages one or more Workers running in separate threads.
    Can start workers either from an .ini config file or from default/app-provided configuration.
    """

    def __init__(self):
        super().__init__()
        self.workers: list[tuple[Worker, QThread]] = []

    def start(self, num_workers: int = 1, from_config: str | None = None):
        """
        Start the specified number of Worker instances.
        If from_config is provided, workers will load configuration from the given .ini file.
        """
        for _ in range(num_workers):
            worker = Worker(from_config=from_config)
            thread = QThread()

            worker.moveToThread(thread)
            thread.started.connect(worker.run)

            self.workers.append((worker, thread))
            thread.start()

    def stop(self):
        """
        Gracefully stop all workers and threads.
        """
        for worker, thread in self.workers:
            worker.stop()

        for _, thread in self.workers:
            thread.quit()
            thread.wait()

        for worker, _ in self.workers:
            worker.shutdown_backend()

        self.workers.clear()

    def get_callback_adapters(self):
        """
        Return a list of callback adapters (one per worker).
        """
        return [worker.callback_adapter for worker, _ in self.workers]

    def update_topology(self, topology_data: TopologyData):
        """
        Broadcasts updated topology to all workers.
        """
        nodes = topology_data["nodes"]
        edges = topology_data["edges"]

        for worker, _ in self.workers:
            worker.update_topology(nodes, edges)
