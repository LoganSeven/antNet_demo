from qtpy.QtWidgets import (
    QWidget,
    QGridLayout,
    QPushButton,
    QSpinBox,
    QCheckBox,
    QSizePolicy,
    QVBoxLayout,
    QLabel,
)
from qtpy.QtCore import Qt
from math import floor

from gui.uniform_spinbox_manager import UniformSpinBoxManager  # Adjust import path as needed


class FitWidthLabel(QLabel):
    """
    A label that adjusts its font size to fit the available width.
    This does not focus on height scaling.
    """

    def __init__(self, text="", parent=None):
        super().__init__(text, parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        # Storing a default base font size for manual re-scaling
        if self.font().pointSize() > 0:
            self._base_font_size = self.font().pointSize()
        else:
            self._base_font_size = 12

    def resizeEvent(self, event):
        """
        Called whenever the label is resized.
        Dynamically adjusts the font size so that the text fits the label's width.
        """
        super().resizeEvent(event)
        self.adjust_font_to_width()

    def adjust_font_to_width(self):
        """
        Finds the largest font size that allows the entire text to fit within the label's current width.
        A simple binary search is used for demonstration.
        """
        available_width = self.width()
        if available_width <= 0:
            return

        low, high = 1, 200
        best_fit = low

        while low <= high:
            mid = (low + high) // 2
            test_font = self.font()
            test_font.setPointSize(mid)
            # A new QFontMetrics is created to measure text width
            from qtpy.QtGui import QFontMetrics
            fm = QFontMetrics(test_font)
            text_width = fm.width(self.text())

            if text_width <= available_width:
                best_fit = mid
                low = mid + 1
            else:
                high = mid - 1

        # Apply the best fitting size
        final_font = self.font()
        final_font.setPointSize(best_fit)
        self.setFont(final_font)


class ControlWidget(QWidget):
    """
    Grid-based control panel for Zone 3.
    Columns and rows configured for responsive resizing.
    Dynamically resizes fonts for buttons and spinboxes if desired.
    Labels specifically adjust width-based font size via FitWidthLabel.
    This also enforces uniform column widths for spin boxes.
    """

    def __init__(self, parent=None):
        super().__init__(parent)

        # Base references for dynamic resizing
        self._base_size = None
        self._base_font_size = None

        self._init_ui()

    def _init_ui(self):
        layout = QGridLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        # Create a manager for uniform spin box widths
        self._spinbox_manager = UniformSpinBoxManager(layout)

        # --- Widgets creation ---
        button_add_10         = QPushButton("Add 10 nodes")
        button_add_100        = QPushButton("Add 100 nodes")
        button_del_10         = QPushButton("Del 10 nodes")
        button_reset_delay    = QPushButton("Reset nodes delay")
        button_reset_topology = QPushButton("Reset topology")

        label_swarms          = FitWidthLabel("nb swarms:")
        spin_swarms           = QSpinBox()
        btn_validate_swarms   = QPushButton("Validate")

        label_set_nodes       = FitWidthLabel("set nb nodes:")
        spin_set_nodes        = QSpinBox()
        btn_validate_nodes    = QPushButton("Validate")

        label_min_hops        = FitWidthLabel("min hops:")
        spin_min_hops         = QSpinBox()
        btn_validate_min_hops = QPushButton("Validate")

        label_max_hops        = FitWidthLabel("max hops:")
        spin_max_hops         = QSpinBox()
        btn_validate_max_hops = QPushButton("Validate")

        label_default_delay   = FitWidthLabel("node default delay:")
        spin_default_delay    = QSpinBox()
        btn_set_default_delay = QPushButton("Set")

        label_death_delay     = FitWidthLabel("node death delay:")
        spin_death_delay      = QSpinBox()
        btn_validate_death_delay = QPushButton("Validate")

        label_node_attack     = FitWidthLabel("node under attack:")
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

        # Spin boxes are registered with the UniformSpinBoxManager for each relevant column
        # The columns below match how they are placed in the layout
        # (row, column) is determined by layout.addLayout(..., row, column_index)
        # The spin boxes appear in columns 1 or 3 depending on the row setup.

        # Will register each spin box with the appropriate column index.
        # Row 0 => spin_swarms in column 1
        # Row 1 => spin_set_nodes in column 1
        # Row 2 => spin_min_hops in column 1, spin_max_hops in column 3
        # Row 3 => spin_default_delay in column 1, spin_death_delay in column 3
        # Row 4 => spin_node_attack in column 1
        # The sublayout usage means the spin box is effectively placed at those columns.

        self._spinbox_manager.register_spinbox(spin_swarms, 1)
        self._spinbox_manager.register_spinbox(spin_set_nodes, 1)
        self._spinbox_manager.register_spinbox(spin_min_hops, 1)
        self._spinbox_manager.register_spinbox(spin_max_hops, 3)
        self._spinbox_manager.register_spinbox(spin_default_delay, 1)
        self._spinbox_manager.register_spinbox(spin_death_delay, 3)
        self._spinbox_manager.register_spinbox(spin_node_attack, 1)

        # --- Size policies for buttons, checkboxes, spin boxes ---
        all_buttons = [
            button_add_10, button_add_100, button_del_10,
            button_reset_delay, button_reset_topology,
            btn_validate_swarms, btn_validate_nodes, btn_validate_min_hops,
            btn_validate_max_hops, btn_set_default_delay, btn_validate_death_delay
        ]
        for btn in all_buttons:
            btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        for widget in [
            spin_swarms, spin_set_nodes, spin_min_hops,
            spin_max_hops, spin_default_delay, spin_death_delay,
            spin_node_attack
        ]:
            widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        for widget in [checkbox_simulate_ddos, checkbox_random_perf, checkbox_brute_perf]:
            widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        fit_width_labels = [
            label_swarms, label_set_nodes, label_min_hops,
            label_max_hops, label_default_delay, label_death_delay,
            label_node_attack
        ]
        for lbl in fit_width_labels:
            lbl.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)

        # Enforce consistent button widths based on "Reset nodes delay"
        ref_button_width = button_reset_delay.sizeHint().width()
        for btn in all_buttons:
            btn.setMaximumWidth(ref_button_width)

        # --- Layout placement ---
        layout.addWidget(button_add_10, 0, 0)
        swarms_layout = QVBoxLayout()
        swarms_layout.addWidget(label_swarms)
        swarms_layout.addWidget(spin_swarms)
        layout.addLayout(swarms_layout, 0, 1)
        layout.addWidget(btn_validate_swarms, 0, 2)

        layout.addWidget(button_add_100, 1, 0)
        set_nodes_layout = QVBoxLayout()
        set_nodes_layout.addWidget(label_set_nodes)
        set_nodes_layout.addWidget(spin_set_nodes)
        layout.addLayout(set_nodes_layout, 1, 1)
        layout.addWidget(btn_validate_nodes, 1, 2)

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

        layout.addWidget(button_reset_topology, 4, 0)
        node_attack_layout = QVBoxLayout()
        node_attack_layout.addWidget(label_node_attack)
        node_attack_layout.addWidget(spin_node_attack)
        layout.addLayout(node_attack_layout, 4, 1)
        layout.addWidget(checkbox_simulate_ddos, 4, 2, 1, 2)

        layout.addWidget(checkbox_random_perf, 5, 0, 1, 2)
        layout.addWidget(checkbox_brute_perf, 5, 2, 1, 3)

        # --- Column and row stretch factors ---
        layout.setColumnStretch(0, 1)
        layout.setColumnStretch(2, 1)
        layout.setColumnStretch(4, 1)
        layout.setRowStretch(5, 1)

    def resizeEvent(self, event):
        """
        Optionally scales font size for the overall widget,
        then updates spin box columns for uniform widths.
        FitWidthLabel objects do their own width-based scaling.
        """
        super().resizeEvent(event)

        # If base_size not set, initialize it here
        if self._base_size is None:
            self._base_size = self.size()
            current_font = self.font()
            if current_font.pointSize() > 0:
                self._base_font_size = current_font.pointSize()
            else:
                self._base_font_size = current_font.pixelSize() or 12

        if not self._base_size.isValid() or not self._base_font_size:
            return

        # Compute scale factor for the entire widget if desired
        w_ratio = self.width() / self._base_size.width()
        h_ratio = self.height() / self._base_size.height()
        scale = min(w_ratio, h_ratio)

        new_font_size = max(1, int(self._base_font_size * scale))
        current_font = self.font()
        current_font.setPointSize(new_font_size)
        self.setFont(current_font)

        # Apply to all children except FitWidthLabel if desired
        for child in self.findChildren(QWidget):
            if not isinstance(child, FitWidthLabel):
                child.setFont(current_font)

        # Update column widths for spin boxes based on the new font
        self._spinbox_manager.update_column_widths(margin=10)
