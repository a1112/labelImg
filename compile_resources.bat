@echo off
echo Compiling resources for PySide6...
pyside6-rcc resources.qrc -o resources.py
pyside6-rcc resources.qrc -o libs\resources.py
echo Done!
pause
