@echo off
setlocal enabledelayedexpansion

rem ================================
rem libusb MinGW build script
rem ================================

set PROJECT_ROOT=%~dp0
set LIBUSB_SRC=%PROJECT_ROOT%libusb-1.0.30
set LIBUSB_BUILD=%PROJECT_ROOT%build\libusb-build-mingw
set LIBUSB_INSTALL=%PROJECT_ROOT%out
set LIBUSB_CONFIG=%PROJECT_ROOT%config

echo.
echo ===== Check tools from PATH =====

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] gcc not found. Please check your PATH.
    pause
    exit /b 1
)

where dlltool >nul 2>nul
if errorlevel 1 (
    echo [ERROR] dlltool not found. Please check your PATH.
    pause
    exit /b 1
)

echo [OK] gcc:
gcc --version

echo.
echo ===== Check libusb source =====

if not exist "%LIBUSB_SRC%\libusb\libusb.h" (
    echo [ERROR] libusb source not found:
    echo %LIBUSB_SRC%
    pause
    exit /b 1
)

if not exist "%LIBUSB_CONFIG%\config.h" (
    echo [ERROR] config.h not found:
    echo %LIBUSB_CONFIG%\config.h
    pause
    exit /b 1
)

echo [OK] libusb source:
echo %LIBUSB_SRC%

echo.
echo ===== Clean old build =====

if exist "%LIBUSB_BUILD%" (
    echo Remove old build dir:
    echo %LIBUSB_BUILD%
    rmdir /s /q "%LIBUSB_BUILD%"
)

if exist "%LIBUSB_INSTALL%" (
    echo Remove old install dir:
    echo %LIBUSB_INSTALL%
    rmdir /s /q "%LIBUSB_INSTALL%"
)

mkdir "%LIBUSB_BUILD%\obj"
mkdir "%LIBUSB_INSTALL%\bin"
mkdir "%LIBUSB_INSTALL%\lib"
mkdir "%LIBUSB_INSTALL%\include\libusb-1.0"
mkdir "%LIBUSB_INSTALL%\lib\pkgconfig"

set CFLAGS=-std=gnu11 -O2 -DNDEBUG -D_WIN32_WINNT=0x0600 -D_CRT_SECURE_NO_WARNINGS -I"%LIBUSB_CONFIG%" -I"%LIBUSB_SRC%\libusb" -I"%LIBUSB_SRC%"

set SOURCES=^
 "%LIBUSB_SRC%\libusb\core.c"^
 "%LIBUSB_SRC%\libusb\descriptor.c"^
 "%LIBUSB_SRC%\libusb\hotplug.c"^
 "%LIBUSB_SRC%\libusb\io.c"^
 "%LIBUSB_SRC%\libusb\strerror.c"^
 "%LIBUSB_SRC%\libusb\sync.c"^
 "%LIBUSB_SRC%\libusb\os\events_windows.c"^
 "%LIBUSB_SRC%\libusb\os\threads_windows.c"^
 "%LIBUSB_SRC%\libusb\os\windows_common.c"^
 "%LIBUSB_SRC%\libusb\os\windows_usbdk.c"^
 "%LIBUSB_SRC%\libusb\os\windows_winusb.c"

echo.
echo ===== Build libusb DLL =====

gcc %CFLAGS% -shared -o "%LIBUSB_INSTALL%\bin\libusb-1.0.dll" %SOURCES% "%LIBUSB_SRC%\libusb\libusb-1.0.def" ^
    -Wl,--out-implib,"%LIBUSB_INSTALL%\lib\libusb-1.0.dll.a" ^
    -Wl,--enable-auto-image-base ^
    -lsetupapi -lole32 -luuid -ladvapi32

if errorlevel 1 (
    echo [ERROR] libusb build failed.
    pause
    exit /b 1
)

echo.
echo ===== Install headers and pkgconfig =====

copy /Y "%LIBUSB_SRC%\libusb\libusb.h" "%LIBUSB_INSTALL%\include\libusb-1.0\libusb.h" >nul

(
echo prefix=%LIBUSB_INSTALL:\=/%
echo exec_prefix=${prefix}
echo libdir=${prefix}/lib
echo includedir=${prefix}/include
echo.
echo Name: libusb-1.0
echo Description: C API for USB device access from Linux, macOS, Windows and OpenBSD/NetBSD
echo Version: 1.0.30
echo Libs: -L${libdir} -lusb-1.0
echo Cflags: -I${includedir}/libusb-1.0
) > "%LIBUSB_INSTALL%\lib\pkgconfig\libusb-1.0.pc"

echo.
echo ===== Check install result =====

if exist "%LIBUSB_INSTALL%\include\libusb-1.0\libusb.h" (
    echo [OK] libusb.h found.
) else (
    echo [ERROR] libusb.h not found.
    pause
    exit /b 1
)

if exist "%LIBUSB_INSTALL%\lib\libusb-1.0.dll.a" (
    echo [OK] libusb-1.0.dll.a found.
) else (
    echo [ERROR] libusb-1.0.dll.a not found.
    pause
    exit /b 1
)

if exist "%LIBUSB_INSTALL%\bin\libusb-1.0.dll" (
    echo [OK] libusb-1.0.dll found.
) else (
    echo [ERROR] libusb-1.0.dll not found.
    pause
    exit /b 1
)

echo.
echo ===== libusb build success =====
echo Install path:
echo %LIBUSB_INSTALL%
echo.

pause
endlocal
