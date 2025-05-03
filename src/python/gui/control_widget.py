from qtpy.QtWidgets import (
    QWidget,
    QGridLayout,
    QPushButton,
    QLabel,
    QSpinBox,
    QCheckBox,
    QSizePolicy,
    QVBoxLayout,
)
from qtpy.QtCore import Qt, QSize


class ControlWidget(QWidget):
    """
    Grid-based control panel for Zone 3.
    Columns and rows configured for responsive resizing.
    Dynamically resizes fonts for labels, buttons, etc.
    """

    def __init__(self, parent=None):
        super().__init__(parent)

        self._init_ui()

        # Base size/font references for dynamic resizing
        self._base_size = None
        # Store either pointSize or pixelSize if pointSize is -1
        self._base_font_size = None

    def _init_ui(self):
        layout = QGridLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        # --- Widgets creation ---
        button_add_10         = QPushButton("Add 10 nodes")
        button_add_100        = QPushButton("Add 100 nodes")
        button_del_10         = QPushButton("Del 10 nodes")
        button_reset_delay    = QPushButton("Reset nodes delay")
        button_reset_topology = QPushButton("Reset topology")

        label_swarms          = QLabel("nb swarms:")
        spin_swarms           = QSpinBox()
        btn_validate_swarms   = QPushButton("Validate")

        label_set_nodes       = QLabel("set nb nodes:")
        spin_set_nodes        = QSpinBox()
        btn_validate_nodes    = QPushButton("Validate")

        label_min_hops        = QLabel("min hops:")
        spin_min_hops         = QSpinBox()
        btn_validate_min_hops = QPushButton("Validate")

        label_max_hops        = QLabel("max hops:")
        spin_max_hops         = QSpinBox()
        btn_validate_max_hops = QPushButton("Validate")

        label_default_delay   = QLabel("node default delay:")
        spin_default_delay    = QSpinBox()
        btn_set_default_delay = QPushButton("Set")

        label_death_delay     = QLabel("node death delay:")
        spin_death_delay      = QSpinBox()
        btn_validate_death_delay = QPushButton("Validate")

        label_node_attack     = QLabel("node under attack:")
        spin_node_attack      = QSpinBox()
        checkbox_simulate_ddos= QCheckBox("Simulate DDOS")

        checkbox_random_perf  = QCheckBox("Show random path search performance")
        checkbox_brute_perf   = QCheckBox("Show brute force path search performance")

        # --- Ranges ---
        spin_swarms.setRange(1, 50)
        spin_set_nodes.setRange(2, 1024)
        spin_min_hops.setRange(4, 512)
        spin_max_hops.setRange(5, 1024)
        spin_default_delay.setRange(1, 255)
        spin_death_delay.setRange(1, 1000)
        spin_node_attack.setRange(1, 1023)

        # --- Size policies ---
        all_buttons = [
            button_add_10, button_add_100, button_del_10,
            button_reset_delay, button_reset_topology,
            btn_validate_swarms, btn_validate_nodes, btn_validate_min_hops,
            btn_validate_max_hops, btn_set_default_delay, btn_validate_death_delay
        ]
        for btn in all_buttons:
            # Expand horizontally, but fixed in vertical dimension
            btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # Spin boxes
        for widget in [
            spin_swarms, spin_set_nodes, spin_min_hops,
            spin_max_hops, spin_default_delay, spin_death_delay,
            spin_node_attack
        ]:
            widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # Checkboxes
        for widget in [checkbox_simulate_ddos, checkbox_random_perf, checkbox_brute_perf]:
            widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # Labels
        all_labels = [
            label_swarms, label_set_nodes, label_min_hops,
            label_max_hops, label_default_delay, label_death_delay,
            label_node_attack
        ]
        for widget in all_labels:
            widget.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
            widget.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)

        # --- Enforce consistent button widths based on "Reset nodes delay" ---
        ref_button_width = button_reset_delay.sizeHint().width()
        for btn in all_buttons:
            btn.setMaximumWidth(ref_button_width)

        # --- Layout placement ---
        # Row 0
        layout.addWidget(button_add_10, 0, 0)

        swarms_layout = QVBoxLayout()
        swarms_layout.addWidget(label_swarms)
        swarms_layout.addWidget(spin_swarms)
        layout.addLayout(swarms_layout, 0, 1)

        layout.addWidget(btn_validate_swarms, 0, 2)

        # Row 1
        layout.addWidget(button_add_100, 1, 0)

        set_nodes_layout = QVBoxLayout()
        set_nodes_layout.addWidget(label_set_nodes)
        set_nodes_layout.addWidget(spin_set_nodes)
        layout.addLayout(set_nodes_layout, 1, 1)

        layout.addWidget(btn_validate_nodes, 1, 2)

        # Row 2
        layout.addWidget(button_del_10, 2, 0)

        min_hops_layout = QVBoxLayout()
        min_hops_layout.addWidget(label_min_hops)
        min_hops_layout.addWidget(spin_min_hops)
        layout.addLayout(min_hops_layout, 2, 1)

        layout.addWidget(btn_validate_min_hops, 2, 2)

        max_hops_layout = QVBoxLayout()
        max_hops_layout.addWidget(label_max_hops)
        max_hops_layout.addWidget(spin_max_hops)
        layout.addLayout(max_hops_layout, 2, 3)

        layout.addWidget(btn_validate_max_hops, 2, 4)

        # Row 3
        layout.addWidget(button_reset_delay, 3, 0)

        default_delay_layout = QVBoxLayout()
        default_delay_layout.addWidget(label_default_delay)
        default_delay_layout.addWidget(spin_default_delay)
        layout.addLayout(default_delay_layout, 3, 1)

        layout.addWidget(btn_set_default_delay, 3, 2)

        death_delay_layout = QVBoxLayout()
        death_delay_layout.addWidget(label_death_delay)
        death_delay_layout.addWidget(spin_death_delay)
        layout.addLayout(death_delay_layout, 3, 3)

        layout.addWidget(btn_validate_death_delay, 3, 4)

        # Row 4
        layout.addWidget(button_reset_topology, 4, 0)

        node_attack_layout = QVBoxLayout()
        node_attack_layout.addWidget(label_node_attack)
        node_attack_layout.addWidget(spin_node_attack)
        layout.addLayout(node_attack_layout, 4, 1)

        layout.addWidget(checkbox_simulate_ddos, 4, 2, 1, 2)

        # Row 5
        layout.addWidget(checkbox_random_perf, 5, 0, 1, 2)
        layout.addWidget(checkbox_brute_perf, 5, 2, 1, 3)

        # --- Column and row stretch factors ---
        layout.setColumnStretch(0, 1)  
        layout.setColumnStretch(2, 1)
        layout.setColumnStretch(4, 1)
        layout.setRowStretch(5, 1)

    def resizeEvent(self, event):
        """
        Dynamically scales font size for all controls (labels, buttons, etc.)
        in Zone 3 based on the widget's resized dimensions.
        """
        super().resizeEvent(event)

        # If base_size not set, initialize it now
        if self._base_size is None:
            self._base_size = self.size()
            current_font = self.font()
            # If pointSize == -1, it means it's specified in pixels, so use pixelSize
            if current_font.pointSize() > 0:
                self._base_font_size = current_font.pointSize()
            else:
                self._base_font_size = current_font.pixelSize() or 12

        if not self._base_size.isValid() or not self._base_font_size:
            return

        # Compute scale factor from the ratio of new size to the base size
        w_ratio = self.width() / self._base_size.width()
        h_ratio = self.height() / self._base_size.height()
        scale = min(w_ratio, h_ratio)  # keep aspect ratio consistent

        # Calculate new font size as integer
        new_font_size = max(1, int(self._base_font_size * scale))

        current_font = self.font()
        # Decide if using pointSize or pixelSize
        # For consistent results across DPI, one can prefer pixelSize
        current_font.setPointSize(new_font_size)

        # Apply to self so children inherit it
        self.setFont(current_font)

        # Optionally force all children to use this new font too
        for child in self.findChildren(QWidget):
            child.setFont(current_font)
