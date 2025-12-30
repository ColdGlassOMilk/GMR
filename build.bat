@echo off
REM GMR Build Launcher for Windows
REM Usage: build.bat [target]
REM   Targets: debug (default), release, web, all, clean, run, serve

setlocal

REM Find MSYS2 installation
set "MSYS2_PATH=C:\msys64"
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    set "MSYS2_PATH=C:\msys32"
)
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found
    pause
    exit /b 1
)

set "SCRIPT_DIR=%~dp0"
set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=debug"

REM For 'run' and 'serve', keep the window open
if "%TARGET%"=="run" goto :run_interactive
if "%TARGET%"=="serve" goto :run_interactive

REM Normal build - close when done
"%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -where "%SCRIPT_DIR%" -c "./build.sh %TARGET%"
goto :eof

:run_interactive
"%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -where "%SCRIPT_DIR%" -c "./build.sh %TARGET%; echo; echo 'Press Enter to close...'; read"
goto :eof

endlocal