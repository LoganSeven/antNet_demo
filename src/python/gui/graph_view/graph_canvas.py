# src/python/gui/graph_view/graph_canvas.py
"""
GraphCanvas holds a GraphScene inside a QGraphicsView.
It does not directly manage node counts; instead, MainWindow or other higher-level
components call scene.init_scene_with_nodes(...) after reading configuration.
"""

from qtpy.QtWidgets import QGraphicsView
from qtpy.QtCore import Qt
from .graph_scene import GraphScene

class GraphCanvas(QGraphicsView):
    """
    QGraphicsView that holds a GraphScene. The scene manages the HopMapManager.
    The canvas auto-fits the scene when resized.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scene = GraphScene()
        self.setScene(self.scene)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)
