@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT=%%~fI"

set "PROJECT_NAME=BSDToolGatger"

call :read_config BSDTG_CONFIGURED CONFIGURED
if not "%CONFIGURED%"=="1" (
    echo Build config not found.
    echo Please run build_config.bat first.
    exit /b 1
)

call :read_config BSDTG_PACKAGE_ROOT PACKAGE_ROOT
call :read_config BSDTG_QT_BIN CONFIG_QT_BIN
call :read_config BSDTG_MINGW_BIN CONFIG_MINGW_BIN
call :read_config BSDTG_DEFAULT_BUILD_MODE CONFIG_BUILD_MODE

if "%PACKAGE_ROOT%"=="" (
    echo PACKAGE_ROOT is empty. Run build_config.bat again.
    exit /b 1
)

set "BUILD_MODE=%~1"
if "%BUILD_MODE%"=="" set "BUILD_MODE=%CONFIG_BUILD_MODE%"
if "%BUILD_MODE%"=="" set "BUILD_MODE=release"

if /I "%BUILD_MODE%"=="debug" (
    set "BUILD_MODE=debug"
) else if /I "%BUILD_MODE%"=="release" (
    set "BUILD_MODE=release"
) else (
    echo Usage: build_all.bat [debug^|release]
    exit /b 1
)

call :resolve_qt_tools
if errorlevel 1 exit /b 1

call :resolve_mingw_tools
if errorlevel 1 exit /b 1

set "PATH=%MINGW_BIN%;%QT_BIN%;%PATH%"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if "%JOBS%"=="" set "JOBS=4"

set "APP_DIR=%PACKAGE_ROOT%\%PROJECT_NAME%"
set "APP_EXE=%APP_DIR%\%PROJECT_NAME%.exe"
set "BUILD_DIR=%ROOT%\out\Package_%BUILD_MODE%"
set "BUILD_EXE=%BUILD_DIR%\%BUILD_MODE%\%PROJECT_NAME%.exe"

set "FREETYPE_DLL=%ROOT%\source\freetype\out\bin\libfreetype.dll"
set "FREETYPE_DLL_ALT=%ROOT%\source\freetype\out\bin\libfreetype-6.dll"
set "FREETYPE_DLL_THIRDPARTY=%ROOT%\source\thirdparty\freetype-mingw\bin\libfreetype.dll"
set "LIBUSB_DLL=%ROOT%\source\libusb\out\bin\libusb-1.0.dll"
set "LIBUVC_DLL=%ROOT%\source\libuvc\out\bin\libuvc.dll"
set "LIBUVC_LIBUSB_DLL=%ROOT%\source\libuvc\out\bin\libusb-1.0.dll"

echo.
echo ===== Build and package %PROJECT_NAME% (%BUILD_MODE%) =====
echo ROOT:      "%ROOT%"
echo QMAKE:     "%QMAKE_EXE%"
echo MAKE:      "%MAKE_EXE%"
echo DEPLOY:    "%WINDEPLOYQT_EXE%"
echo BUILD_DIR: "%BUILD_DIR%"
echo APP_DIR:   "%APP_DIR%"
echo.

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

pushd "%BUILD_DIR%" || exit /b 1
if "%BUILD_MODE%"=="debug" (
    "%QMAKE_EXE%" -o Makefile "%ROOT%\%PROJECT_NAME%.pro" -spec win32-g++ CONFIG+=debug CONFIG+=qml_debug
) else (
    "%QMAKE_EXE%" -o Makefile "%ROOT%\%PROJECT_NAME%.pro" -spec win32-g++ CONFIG+=release CONFIG-=debug
)
if errorlevel 1 (
    popd
    exit /b 1
)

"%MAKE_EXE%" -j%JOBS%
if errorlevel 1 (
    popd
    exit /b 1
)
popd

if not exist "%BUILD_EXE%" (
    echo Build output not found: "%BUILD_EXE%"
    exit /b 1
)

if not exist "%APP_DIR%" mkdir "%APP_DIR%"

copy /Y "%BUILD_EXE%" "%APP_EXE%" >nul
if errorlevel 1 exit /b 1

if "%BUILD_MODE%"=="debug" (
    "%WINDEPLOYQT_EXE%" --debug --compiler-runtime "%APP_EXE%"
) else (
    "%WINDEPLOYQT_EXE%" --compiler-runtime "%APP_EXE%"
)
if errorlevel 1 exit /b 1

if exist "%ROOT%\config" (
    xcopy "%ROOT%\config" "%APP_DIR%\config" /E /I /Y >nul
    if errorlevel 2 exit /b 1
)

if exist "%FREETYPE_DLL%" (
    call :copy_required_dll "FreeType" "%FREETYPE_DLL%" "libfreetype.dll"
) else if exist "%FREETYPE_DLL_ALT%" (
    call :copy_required_dll "FreeType" "%FREETYPE_DLL_ALT%" "libfreetype.dll"
) else if exist "%FREETYPE_DLL_THIRDPARTY%" (
    call :copy_required_dll "FreeType" "%FREETYPE_DLL_THIRDPARTY%" "libfreetype.dll"
) else (
    echo FreeType DLL not found.
    exit /b 1
)
if errorlevel 1 exit /b 1

if exist "%LIBUVC_LIBUSB_DLL%" (
    call :copy_required_dll "libusb from libuvc" "%LIBUVC_LIBUSB_DLL%" "libusb-1.0.dll"
) else (
    call :copy_required_dll "libusb" "%LIBUSB_DLL%" "libusb-1.0.dll"
)
if errorlevel 1 exit /b 1

call :copy_required_dll "libuvc" "%LIBUVC_DLL%" "libuvc.dll"
if errorlevel 1 exit /b 1

echo.
echo Package complete: "%APP_DIR%"
endlocal
exit /b 0

:read_config
set "%~2="
for /f "tokens=1,2,*" %%A in ('reg query HKCU\Environment /v "%~1" 2^>nul') do (
    if /I "%%A"=="%~1" set "%~2=%%C"
)
exit /b 0

:resolve_qt_tools
if /I "%CONFIG_QT_BIN%"=="USE_ENV" goto find_qt_from_env
if /I "%CONFIG_QT_BIN%"=="ENV" goto find_qt_from_env
set "QT_BIN=%CONFIG_QT_BIN%"
if "%QT_BIN%"=="" (
    echo QT_BIN is empty. Run build_config.bat again.
    exit /b 1
)
if "%QT_BIN:~-1%"=="\" set "QT_BIN=%QT_BIN:~0,-1%"
set "QMAKE_EXE=%QT_BIN%\qmake.exe"
set "WINDEPLOYQT_EXE=%QT_BIN%\windeployqt.exe"
if not exist "%QMAKE_EXE%" (
    echo qmake.exe not found: "%QMAKE_EXE%"
    exit /b 1
)
if not exist "%WINDEPLOYQT_EXE%" (
    echo windeployqt.exe not found: "%WINDEPLOYQT_EXE%"
    exit /b 1
)
exit /b 0

:find_qt_from_env
if defined QT_BIN (
    if exist "%QT_BIN%\qmake.exe" set "QMAKE_EXE=%QT_BIN%\qmake.exe"
)
if not defined QMAKE_EXE (
    for %%I in (qmake.exe) do if not "%%~$PATH:I"=="" set "QMAKE_EXE=%%~$PATH:I"
)
if not defined QMAKE_EXE (
    echo qmake.exe not found from environment PATH.
    exit /b 1
)
for %%I in ("%QMAKE_EXE%") do set "QT_BIN=%%~dpI"
if "%QT_BIN:~-1%"=="\" set "QT_BIN=%QT_BIN:~0,-1%"
set "WINDEPLOYQT_EXE=%QT_BIN%\windeployqt.exe"
if not exist "%WINDEPLOYQT_EXE%" (
    echo windeployqt.exe not found: "%WINDEPLOYQT_EXE%"
    exit /b 1
)
exit /b 0

:resolve_mingw_tools
if /I "%CONFIG_MINGW_BIN%"=="USE_ENV" goto find_mingw_from_env
if /I "%CONFIG_MINGW_BIN%"=="ENV" goto find_mingw_from_env
set "MINGW_BIN=%CONFIG_MINGW_BIN%"
if "%MINGW_BIN%"=="" (
    echo MINGW_BIN is empty. Run build_config.bat again.
    exit /b 1
)
if "%MINGW_BIN:~-1%"=="\" set "MINGW_BIN=%MINGW_BIN:~0,-1%"
set "MAKE_EXE=%MINGW_BIN%\mingw32-make.exe"
if not exist "%MAKE_EXE%" (
    echo mingw32-make.exe not found: "%MAKE_EXE%"
    exit /b 1
)
exit /b 0

:find_mingw_from_env
if defined MINGW_BIN (
    if exist "%MINGW_BIN%\mingw32-make.exe" set "MAKE_EXE=%MINGW_BIN%\mingw32-make.exe"
)
if not defined MAKE_EXE (
    for %%I in (mingw32-make.exe) do if not "%%~$PATH:I"=="" set "MAKE_EXE=%%~$PATH:I"
)
if not defined MAKE_EXE (
    echo mingw32-make.exe not found from environment PATH.
    exit /b 1
)
for %%I in ("%MAKE_EXE%") do set "MINGW_BIN=%%~dpI"
if "%MINGW_BIN:~-1%"=="\" set "MINGW_BIN=%MINGW_BIN:~0,-1%"
exit /b 0

:copy_required_dll
set "DLL_NAME=%~1"
set "DLL_SRC=%~2"
set "DLL_DST=%~3"
if not exist "%DLL_SRC%" (
    echo %DLL_NAME% DLL not found: "%DLL_SRC%"
    exit /b 1
)
copy /Y "%DLL_SRC%" "%APP_DIR%\%DLL_DST%" >nul
if errorlevel 1 (
    echo Copy %DLL_NAME% DLL failed: "%DLL_SRC%"
    exit /b 1
)
echo Copy %DLL_NAME% DLL: "%DLL_DST%"
exit /b 0
