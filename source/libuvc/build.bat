@echo off
setlocal enabledelayedexpansion

rem ================================
rem libuvc MinGW build script
rem ================================

set PROJECT_ROOT=%~dp0
set LIBUVC_SRC=%PROJECT_ROOT%libuvc-0.0.7
set LIBUVC_BUILD=%PROJECT_ROOT%build\libuvc-build-mingw
set LIBUVC_INSTALL=%PROJECT_ROOT%out
set LIBUSB_INSTALL=%PROJECT_ROOT%..\libusb\out
set LIBUSB_IMPORT=%LIBUSB_INSTALL%\lib\libusb-1.0.dll.a
set LIBUSB_INCLUDE=%LIBUSB_INSTALL%\include\libusb-1.0
set LIBUSB_DLL=%LIBUSB_INSTALL%\bin\libusb-1.0.dll
set LIBUSB_TARGET_FILE=%LIBUVC_BUILD%\libusb_import.cmake

echo.
echo ===== Check tools from PATH =====

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] gcc not found. Please check your PATH.
    pause
    exit /b 1
)

where mingw32-make >nul 2>nul
if errorlevel 1 (
    echo [ERROR] mingw32-make not found. Please check your PATH.
    pause
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake not found. Please check your PATH.
    pause
    exit /b 1
)

echo [OK] gcc:
gcc --version

echo.
echo [OK] mingw32-make:
mingw32-make --version

echo.
echo [OK] cmake:
cmake --version

echo.
echo ===== Check libuvc source =====

if not exist "%LIBUVC_SRC%\CMakeLists.txt" (
    echo [ERROR] libuvc source not found:
    echo %LIBUVC_SRC%
    pause
    exit /b 1
)

if not exist "%LIBUSB_IMPORT%" (
    echo [ERROR] libusb import library not found:
    echo %LIBUSB_IMPORT%
    echo Please run ..\libusb\build.bat first.
    pause
    exit /b 1
)

if not exist "%LIBUSB_INCLUDE%\libusb.h" (
    echo [ERROR] libusb header not found:
    echo %LIBUSB_INCLUDE%\libusb.h
    echo Please run ..\libusb\build.bat first.
    pause
    exit /b 1
)

echo [OK] libuvc source:
echo %LIBUVC_SRC%

echo.
echo ===== Clean old build =====

if exist "%LIBUVC_BUILD%" (
    echo Remove old build dir:
    echo %LIBUVC_BUILD%
    rmdir /s /q "%LIBUVC_BUILD%"
)

mkdir "%LIBUVC_BUILD%"

echo.
echo ===== Create libusb imported target =====

(
echo if(NOT TARGET LibUSB::LibUSB^)
echo   add_library(LibUSB::LibUSB UNKNOWN IMPORTED^)
echo   set_target_properties(LibUSB::LibUSB PROPERTIES
echo     IMPORTED_LOCATION "%LIBUSB_IMPORT:\=/%"
echo     INTERFACE_INCLUDE_DIRECTORIES "%LIBUSB_INCLUDE:\=/%"
echo   ^)
echo endif(^)
) > "%LIBUSB_TARGET_FILE%"

echo.
echo ===== Configure libuvc =====

cd /d "%LIBUVC_BUILD%"

cmake -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%LIBUVC_INSTALL%" ^
    -DCMAKE_PROJECT_INCLUDE="%LIBUSB_TARGET_FILE%" ^
    -DCMAKE_BUILD_TARGET=Shared ^
    -DBUILD_EXAMPLE=OFF ^
    -DBUILD_TEST=OFF ^
    -DENABLE_UVC_DEBUGGING=OFF ^
    "%LIBUVC_SRC%"

if errorlevel 1 (
    echo [ERROR] cmake configure failed.
    pause
    exit /b 1
)

echo.
echo ===== Build libuvc =====

mingw32-make -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] build failed.
    pause
    exit /b 1
)

echo.
echo ===== Install libuvc =====

mingw32-make install

if errorlevel 1 (
    echo [ERROR] install failed.
    pause
    exit /b 1
)

echo.
echo ===== Copy runtime dependencies =====

if exist "%LIBUSB_DLL%" (
    copy /Y "%LIBUSB_DLL%" "%LIBUVC_INSTALL%\bin\" >nul
)

echo.
echo ===== Check install result =====

if exist "%LIBUVC_INSTALL%\include\libuvc\libuvc.h" (
    echo [OK] libuvc.h found.
) else (
    echo [ERROR] libuvc.h not found.
    pause
    exit /b 1
)

if exist "%LIBUVC_INSTALL%\lib\libuvc.dll.a" (
    echo [OK] libuvc.dll.a found.
) else (
    echo [ERROR] libuvc.dll.a not found.
    pause
    exit /b 1
)

if exist "%LIBUVC_INSTALL%\bin\libuvc.dll" (
    echo [OK] libuvc.dll found.
) else (
    echo [ERROR] libuvc.dll not found.
    pause
    exit /b 1
)

echo.
echo ===== libuvc build success =====
echo Install path:
echo %LIBUVC_INSTALL%
echo.

pause
endlocal
