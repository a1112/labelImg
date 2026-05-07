$ErrorActionPreference = "Continue"
$LOG = "J:\labelImg\ps_run_log.txt"

"=== LabelImg Debug Run ===" | Out-File -FilePath $LOG -Encoding UTF8
"Date: $(Get-Date)" | Out-File -FilePath $LOG -Append -Encoding UTF8

# Find Python
"Python locations:" | Out-File -FilePath $LOG -Append -Encoding UTF8
Get-Command python* -ErrorAction SilentlyContinue | Select-Object Source | Out-File -FilePath $LOG -Append -Encoding UTF8

# Test Python
"`nPython version:" | Out-File -FilePath $LOG -Append -Encoding UTF8
python --version 2>&1 | Out-File -FilePath $LOG -Append -Encoding UTF8

# Test PySide6
"`nPySide6 check:" | Out-File -FilePath $LOG -Append -Encoding UTF8
python -c "import PySide6; print('PySide6 version:', PySide6.__version__)" 2>&1 | Out-File -FilePath $LOG -Append -Encoding UTF8

# Run labelImg
"`nRunning labelImg:" | Out-File -FilePath $LOG -Append -Encoding UTF8
cd J:\labelImg
python labelImg.py 2>&1 | Out-File -FilePath $LOG -Append -Encoding UTF8

# Show log
Get-Content $LOG
