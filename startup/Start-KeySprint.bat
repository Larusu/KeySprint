@echo off
setlocal
cd /d "%~dp0.."

py -3 -c "import sys; sys.exit(sys.version_info < (3, 10))" >nul 2>&1
if not errorlevel 1 (
    py -3 launcher.py
    goto finished
)
python -c "import sys; sys.exit(sys.version_info < (3, 10))" >nul 2>&1
if not errorlevel 1 (
    python launcher.py
    goto finished
)

echo Python 3.10 or newer is required.
echo Install Python: https://www.python.org/downloads/
echo Enable the Python launcher or add Python to PATH during installation.
pause
exit /b 1

:finished
if errorlevel 1 pause
