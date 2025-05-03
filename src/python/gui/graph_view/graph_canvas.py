from qtpy.QtWidgets import QGraphicsView
from qtpy.QtCore import Qt
from .graph_scene import GraphScene

class GraphCanvas(QGraphicsView):
    """
    QGraphicsView that holds a GraphScene. It auto-fits the scene whenever resized.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        # Pass total_nodes=16, or an appropriate value
        self.scene = GraphScene(total_nodes=16)
        self.setScene(self.scene)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)