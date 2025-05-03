from qtpy.QtWidgets import QGraphicsView
from qtpy.QtCore import Qt
from .graph_scene import GraphScene


class GraphCanvas(QGraphicsView):
    """
    QGraphicsView that holds a GraphScene. It auto-fits the scene whenever resized.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scene = GraphScene()
        self.setScene(self.scene)

    def resizeEvent(self, event):
        """
        Fits the scene in the view on resize.
        """
        super().resizeEvent(event)
        self.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)
