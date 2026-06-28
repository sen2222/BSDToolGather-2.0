@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT=%%~fI"

echo ======================================
echo       BSDToolGatger clean script
echo ======================================

echo.
echo [1/4] Clean project output directories...
call :remove_dir "%ROOT%\out"
call :remove_dir "%ROOT%\debug"
call :remove_dir "%ROOT%\release"

echo.
echo [2/4] Clean main generated files...
call :delete_file "%ROOT%\Makefile"
del /q "%ROOT%\Makefile.*" >nul 2>&1
del /q "%ROOT%\*.pro.user" >nul 2>&1
del /q "%ROOT%\*.log" >nul 2>&1
del /q "%ROOT%\*.tmp" >nul 2>&1

echo.
echo [3/4] Clean thirdparty build directories under source...
call :remove_dir "%ROOT%\source\freetype\build"
call :remove_dir "%ROOT%\source\libusb\build"
call :remove_dir "%ROOT%\source\libuvc\build"

echo.
echo [4/4] Clean intermediate files under source...
del /s /q "%ROOT%\source\*.o" "%ROOT%\source\*.obj" "%ROOT%\source\*.res" >nul 2>&1
del /s /q "%ROOT%\source\CMakeCache.txt" "%ROOT%\source\cmake_install.cmake" >nul 2>&1
del /s /q "%ROOT%\source\install_manifest.txt" "%ROOT%\source\CPackConfig.cmake" "%ROOT%\source\CPackSourceConfig.cmake" >nul 2>&1

echo.
echo Keep build scripts:
echo   build\clean_all.bat
echo   build\build_config.bat
echo   build\build_all.bat
echo.
echo Keep thirdparty out directories for link and package:
echo   source\freetype\out
echo   source\libusb\out
echo   source\libuvc\out
echo.
echo Clean complete.
endlocal
exit /b 0

:remove_dir
if exist "%~1" (
    rmdir /s /q "%~1"
    echo   - removed dir: %~1
)
exit /b 0

:delete_file
if exist "%~1" (
    del /q "%~1" >nul 2>&1
    echo   - removed file: %~1
)
exit /b 0
