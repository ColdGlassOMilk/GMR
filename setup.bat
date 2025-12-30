@echo off
REM GMR Setup Launcher for Windows
REM This batch file launches the setup script in the correct MSYS2 environment.
REM Just double-click this file to run setup!

setlocal

REM Find MSYS2 installation
set "MSYS2_PATH=C:\msys64"
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    set "MSYS2_PATH=C:\msys32"
)
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at C:\msys64 or C:\msys32
    echo Please install MSYS2 from https://www.msys2.org/
    pause
    exit /b 1
)

echo ========================================
echo GMR Setup
echo ========================================
echo.
echo This will set up the complete GMR development environment.
echo Running in MSYS2 MinGW64...
echo.

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"

REM Launch MSYS2 MinGW64 and run setup.sh
"%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -where "%SCRIPT_DIR%" -c "./setup.sh; echo; echo 'Press Enter to close...'; read"

endlocal