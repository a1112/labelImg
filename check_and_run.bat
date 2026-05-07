@echo off
echo === Checking PySide6 ===
python -c "import PySide6; print('PySide6 version:', PySide6.__version__)" 2>&1
if errorlevel 1 (
    echo PySide6 not found, installing...
    pip install PySide6 lxml
)
echo === Starting labelImg ===
cd /d J:\labelImg
python labelImg.py
