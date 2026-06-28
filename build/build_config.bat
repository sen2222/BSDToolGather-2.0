@echo off
setlocal EnableExtensions

rem ============================================================
rem Edit these values, then run this script once before build_all.
rem Use USE_ENV for QT_BIN or MINGW_BIN to resolve tools from PATH.
rem ============================================================
set "PACKAGE_ROOT=E:\app"
set "QT_BIN=USE_ENV"
set "MINGW_BIN=USE_ENV"
set "DEFAULT_BUILD_MODE=release"

echo.
echo ===== BSDToolGatger build config =====
echo PACKAGE_ROOT:       "%PACKAGE_ROOT%"
echo QT_BIN:             "%QT_BIN%"
echo MINGW_BIN:          "%MINGW_BIN%"
echo DEFAULT_BUILD_MODE: "%DEFAULT_BUILD_MODE%"
echo.

if "%PACKAGE_ROOT%"=="" (
    echo PACKAGE_ROOT is empty.
    exit /b 1
)

if /I not "%DEFAULT_BUILD_MODE%"=="debug" if /I not "%DEFAULT_BUILD_MODE%"=="release" (
    echo DEFAULT_BUILD_MODE must be debug or release.
    exit /b 1
)

call :check_qt_path
if errorlevel 1 exit /b 1

call :check_mingw_path
if errorlevel 1 exit /b 1

call :save_config BSDTG_CONFIGURED 1
call :save_config BSDTG_PACKAGE_ROOT "%PACKAGE_ROOT%"
call :save_config BSDTG_QT_BIN "%QT_BIN%"
call :save_config BSDTG_MINGW_BIN "%MINGW_BIN%"
call :save_config BSDTG_DEFAULT_BUILD_MODE "%DEFAULT_BUILD_MODE%"

echo.
echo Config saved.
echo Next step:
echo   build_all.bat
echo.
endlocal
exit /b 0

:check_qt_path
if /I "%QT_BIN%"=="USE_ENV" exit /b 0
if /I "%QT_BIN%"=="ENV" exit /b 0
if not exist "%QT_BIN%\qmake.exe" (
    echo qmake.exe not found: "%QT_BIN%\qmake.exe"
    exit /b 1
)
if not exist "%QT_BIN%\windeployqt.exe" (
    echo windeployqt.exe not found: "%QT_BIN%\windeployqt.exe"
    exit /b 1
)
exit /b 0

:check_mingw_path
if /I "%MINGW_BIN%"=="USE_ENV" exit /b 0
if /I "%MINGW_BIN%"=="ENV" exit /b 0
if not exist "%MINGW_BIN%\mingw32-make.exe" (
    echo mingw32-make.exe not found: "%MINGW_BIN%\mingw32-make.exe"
    exit /b 1
)
exit /b 0

:save_config
reg add HKCU\Environment /v "%~1" /t REG_SZ /d "%~2" /f >nul
if errorlevel 1 (
    echo Save config failed: %~1
    exit /b 1
)
exit /b 0
