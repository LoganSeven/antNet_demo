from qtpy.QtCore import QObject, QThread
from core.worker import Worker

class CoreManager(QObject):
    """
    CoreManager: manages one or more Workers running in separate threads.
    """

    def __init__(self):
        super().__init__()
        self.workers = []   # List of (Worker, QThread) pairs

    def start(self, num_workers=1):
        """
        Start the specified number of Worker instances.
        """
        for _ in range(num_workers):
            worker = Worker()
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
            thread.quit()
            thread.wait()
        self.workers.clear()

    def get_callback_adapters(self):
        """
        Return a list of callback adapters (one per worker).
        """
        return [worker.callback_adapter for worker, _ in self.workers]
