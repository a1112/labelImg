@echo off
echo Starting LabelImg (PySide6)...
cd /d J:\labelImg
python -u labelImg.py %* 2>&1
echo Exit code: %ERRORLEVEL%
pause
