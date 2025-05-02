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

            # Move the worker object to the thread
            worker.moveToThread(thread)
            # When the thread starts, call worker.run()
            thread.started.connect(worker.run)

            # Keep track of them
            self.workers.append((worker, thread))

            # Actually start the thread
            thread.start()

    def stop(self):
        """
        Gracefully stop all workers and threads.
        """
        # 1) Signal each worker to stop its main loop
        for worker, thread in self.workers:
            worker.stop()

        # 2) Tell each thread to quit and wait for it to finish
        for worker, thread in self.workers:
            thread.quit()
            thread.wait()

        # 3) Now that the thread has exited, the worker loop is done.
        #    We can safely shut down the backend context.
        for worker, _ in self.workers:
            worker.shutdown_backend()

        # 4) Clear out references
        self.workers.clear()

    def get_callback_adapters(self):
        """
        Return a list of callback adapters (one per worker).
        """
        return [worker.callback_adapter for worker, _ in self.workers]
