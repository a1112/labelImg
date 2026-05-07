#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys

print("Test 1: Imports")
try:
    from PySide6.QtWidgets import QApplication, QLabel
    from PySide6.QtCore import Qt
    print("  PySide6 imports OK")
except Exception as e:
    print(f"  ERROR: {e}")
    sys.exit(1)

print("Test 2: Qt enums")
try:
    print(f"  Qt.Horizontal = {Qt.Horizontal}")
    print(f"  Qt.AlignLeft = {Qt.AlignLeft}")
    print("  Qt enums OK")
except Exception as e:
    print(f"  ERROR: {e}")
    sys.exit(1)

print("Test 3: Create app")
try:
    app = QApplication(sys.argv)
    label = QLabel("Hello LabelImg!")
    label.show()
    print("  App created OK")
    print("  Running event loop for 2 seconds...")
    app.exec()
except Exception as e:
    print(f"  ERROR: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("All tests passed!")
