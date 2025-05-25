# Relative Path: src/python/gui/graph_view/graph_canvas.py
"""
GraphCanvas is a QGraphicsView containing a GraphScene.
Resizes dynamically and updates node positions, edges, and overlays with the sceneRect.
Serves as the primary display surface for the entire network graph.
"""


from qtpy.QtWidgets import QGraphicsView
from qtpy.QtGui import QPainter
from qtpy.QtCore import Qt

from .graph_scene import GraphScene

class GraphCanvas(QGraphicsView):
    """
    QGraphicsView that holds a GraphScene. The scene manages the HopMapManager.
    When resized, it updates the sceneRect to fill 100% horizontally,
    then calls scene.recalc_node_positions(...) so the nodes remain
    consistent in size but spread out across the new width.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scene = GraphScene()
        self.setScene(self.scene)

        # Enable antialiasing and smooth transforms
        self.setRenderHints(
            QPainter.Antialiasing |
            QPainter.TextAntialiasing |
            QPainter.SmoothPixmapTransform
        )

        # No scale transformation
        self.setTransformationAnchor(QGraphicsView.NoAnchor)
        self.setResizeAnchor(QGraphicsView.NoAnchor)

    def resizeEvent(self, event):
        """
        Called whenever the GraphCanvas is resized (including splitter drags).
        We adjust the sceneRect, then re-run recalc_node_positions so that
        the scene can reposition nodes, labels, and edges accordingly.
        """
        super().resizeEvent(event)
        w = self.viewport().width()
        h = self.viewport().height()

        # Update the sceneRect to match the new size
        self.scene.setSceneRect(0, 0, w, h)

        # Recalculate node positions in that new space
        self.scene.recalc_node_positions(w, h)
