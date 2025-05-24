# src/python/gui/widgets/control_widget.py
from qtpy.QtWidgets import (
    QWidget,
    QGridLayout,
    QSizePolicy,
)
from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QFont

from ..managers.fit_width_label_manager import FitWidthLabelManager
from ..managers.fit_width_button_manager import FitWidthButtonManager
from ..managers.fit_width_spinbox_manager import FitWidthSpinBoxManager
from ..managers.fit_width_checkbox_manager import FitWidthCheckBoxManager

from .fit_width_label import FitWidthLabel
from .fit_width_button import FitWidthButton
from .fit_width_spinbox import FitWidthSpinBox
from .fit_width_checkbox import FitWidthCheckBox

class ControlWidget(QWidget):
    """
    Grid-based control panel for Zone 3.
    Columns and rows configured for responsive resizing.
    Uses FitWidthLabelManager for label padding by column,
    FitWidthButtonManager for button text alignment,
    and UniformSpinBoxManager for consistent spin box column widths (now replaced
    here by FitWidthSpinBoxManager),
    and FitWidthCheckBoxManager for dynamic checkbox text resizing.
    """

    signal_add_10_nodes = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)

        self._layout = QGridLayout(self)
        self._layout.setContentsMargins(8, 8, 8, 8)
        self._layout.setSpacing(8)

        self._base_size = None
        self._base_font_size = None

        self.label_manager = FitWidthLabelManager()
        self.button_manager = FitWidthButtonManager()
        self.spinbox_manager = FitWidthSpinBoxManager()
        self.checkbox_manager = FitWidthCheckBoxManager()

        self._create_ui()

    def _create_ui(self):
        # --- Buttons ---
        button_add_10         = FitWidthButton("Add 10 nodes")
        button_add_100        = FitWidthButton("Add 100 nodes")
        button_del_10         = FitWidthButton("Del 10 nodes")
        button_reset_delay    = FitWidthButton("Reset nodes delay")
        button_reset_topology = FitWidthButton("Reset topology")

        btn_validate_swarms        = FitWidthButton("Validate")
        btn_validate_nodes         = FitWidthButton("Validate")
        btn_validate_min_hops      = FitWidthButton("Validate")
        btn_validate_max_hops      = FitWidthButton("Validate")
        btn_set_default_delay      = FitWidthButton("Set")
        btn_validate_death_delay   = FitWidthButton("Validate")

        # --- Labels ---
        label_swarms        = FitWidthLabel("nb swarms:")
        label_set_nodes     = FitWidthLabel("set nb nodes:")
        label_min_hops      = FitWidthLabel("min hops:")
        label_max_hops      = FitWidthLabel("max hops:")
        label_default_delay = FitWidthLabel("node default delay:")
        label_death_delay   = FitWidthLabel("node death delay:")
        label_node_attack   = FitWidthLabel("node under attack:")

        # --- Spin boxes (FitWidthSpinBox) ---
        spin_swarms        = FitWidthSpinBox()
        spin_set_nodes     = FitWidthSpinBox()
        spin_min_hops      = FitWidthSpinBox()
        spin_max_hops      = FitWidthSpinBox()
        spin_default_delay = FitWidthSpinBox()
        spin_death_delay   = FitWidthSpinBox()
        spin_node_attack   = FitWidthSpinBox()

        # --- Checkboxes (FitWidthCheckBox) ---
        checkbox_simulate_ddos = FitWidthCheckBox("Simulate DDOS")
        checkbox_random_perf   = FitWidthCheckBox("Show random performance")
        checkbox_brute_perf    = FitWidthCheckBox("Show brute force performance")

        # --- Ranges ---
        spin_swarms.setRange(1, 50)
        spin_set_nodes.setRange(2, 1024)
        spin_min_hops.setRange(4, 512)
        spin_max_hops.setRange(5, 1024)
        spin_default_delay.setRange(1, 255)
        spin_death_delay.setRange(1, 1000)
        spin_node_attack.setRange(1, 1023)

        # --- Manager registration for labels ---
        for label, col in [
            (label_swarms, 1), (label_set_nodes, 3), (label_min_hops, 1),
            (label_max_hops, 3), (label_default_delay, 1),
            (label_death_delay, 3), (label_node_attack, 1)
        ]:
            self.label_manager.register_label(label, col)

        # --- Manager registration for buttons ---
        for btn in [
            button_add_10, button_add_100, button_del_10,
            button_reset_delay, button_reset_topology,
            btn_validate_swarms, btn_validate_nodes, btn_validate_min_hops,
            btn_validate_max_hops, btn_set_default_delay, btn_validate_death_delay
        ]:
            self.button_manager.register_button(btn, 0)

        # --- Manager registration for spin boxes ---
        # Register each spinbox in the same column where it appears
        self.spinbox_manager.register_spinbox(spin_swarms,        1)
        self.spinbox_manager.register_spinbox(spin_set_nodes,     3)
        self.spinbox_manager.register_spinbox(spin_min_hops,      1)
        self.spinbox_manager.register_spinbox(spin_max_hops,      3)
        self.spinbox_manager.register_spinbox(spin_default_delay, 1)
        self.spinbox_manager.register_spinbox(spin_death_delay,   3)
        self.spinbox_manager.register_spinbox(spin_node_attack,   1)

        # --- Manager registration for checkboxes ---
        # You can choose the same or different columns; here we place them in column 2 or 0
        self.checkbox_manager.register_checkbox(checkbox_simulate_ddos, 2)
        self.checkbox_manager.register_checkbox(checkbox_random_perf,   0)
        self.checkbox_manager.register_checkbox(checkbox_brute_perf,    2)

        # --- Layout arrangement ---
        # row=0
        self._layout.addWidget(button_add_10, 0, 0)
        self._layout.addWidget(label_swarms, 0, 1)
        self._layout.addWidget(spin_swarms, 1, 1)
        self._layout.addWidget(btn_validate_swarms, 1, 2)

        self._layout.addWidget(label_set_nodes, 0, 3)
        self._layout.addWidget(spin_set_nodes, 1, 3)
        self._layout.addWidget(btn_validate_nodes, 1, 4)

        # row=1
        self._layout.addWidget(button_add_100, 1, 0)

        # row=2
        self._layout.addWidget(label_min_hops, 2, 1)
        self._layout.addWidget(spin_min_hops, 3, 1)
        self._layout.addWidget(btn_validate_min_hops, 3, 2)

        self._layout.addWidget(label_max_hops, 2, 3)
        self._layout.addWidget(spin_max_hops, 3, 3)
        self._layout.addWidget(btn_validate_max_hops, 3, 4)

        self._layout.addWidget(button_del_10, 2, 0)

        # row=3
        self._layout.addWidget(button_reset_delay, 3, 0)

        # row=4
        self._layout.addWidget(label_default_delay, 4, 1)
        self._layout.addWidget(spin_default_delay, 5, 1)
        self._layout.addWidget(btn_set_default_delay, 5, 2)

        self._layout.addWidget(label_death_delay, 4, 3)
        self._layout.addWidget(spin_death_delay, 5, 3)
        self._layout.addWidget(btn_validate_death_delay, 5, 4)

        self._layout.addWidget(button_reset_topology, 4, 0)

        # row=6-7
        self._layout.addWidget(label_node_attack, 6, 1)
        self._layout.addWidget(spin_node_attack, 7, 1)
        self._layout.addWidget(checkbox_simulate_ddos, 7, 2, 1, 2)

        # row=9
        self._layout.addWidget(checkbox_random_perf, 9, 0, 1, 2)
        self._layout.addWidget(checkbox_brute_perf, 9, 2, 1, 3)

        # Column stretching
        self._layout.setColumnStretch(0, 1)
        self._layout.setColumnStretch(2, 1)
        self._layout.setColumnStretch(4, 1)

        # Row stretching
        self._layout.setRowStretch(9, 1)

    def resizeEvent(self, event):
        """
        Ensures that each registered FitWidth* control recalculates its font 
        when this widget is resized.
        """
        super().resizeEvent(event)
        self.label_manager.adjust_font_to_width()
        self.button_manager.adjust_font_to_width()
        self.spinbox_manager.adjust_font_to_width()
        self.checkbox_manager.adjust_font_to_width()
