import sys
import os

project_root = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(project_root, "src", "python"))


from gui.main_window import MainWindow
from PyQt5.QtWidgets import QApplication

def main():
    print("👉 Starting AntNet Demo GUI…")
    app = QApplication(sys.argv)
    window = MainWindow()
    window.resize(800, 600)
    window.show()
    print("✅ Window shown, entering event loop")
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()