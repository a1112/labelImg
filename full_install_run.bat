@echo off
setlocal
cd /d J:\labelImg

echo === Checking Python ===
python --version
echo.

echo === Checking PySide6 ===
python -c "import PySide6; print('PySide6 version:', PySide6.__version__)" 2>&1
if errorlevel 1 (
    echo PySide6 not found, installing...
    python -m pip install PySide6 lxml -i https://pypi.org/simple
)
echo.

echo === Running LabelImg ===
python labelImg.py
echo.

echo === Exit code: %ERRORLEVEL% ===
pause
