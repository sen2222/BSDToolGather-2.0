@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PROJECT_NAME=BSDToolGatger"
set "QT_BIN=D:\qt-create\6.5.3\mingw_64\bin"
set "MINGW_BIN=D:\qt-create\Tools\mingw1120_64\bin"
set "BUILD_DIR=%ROOT%\build\Desktop_Qt_6_5_3_MinGW_64_bit_Debug"
set "BUILD_EXE=%BUILD_DIR%\debug\%PROJECT_NAME%.exe"
set "APP_DIR=E:\app\%PROJECT_NAME%"
set "APP_EXE=%APP_DIR%\%PROJECT_NAME%.exe"
set "FREETYPE_DLL=%ROOT%\source\freetype\out\bin\libfreetype.dll"
set "FREETYPE_DLL_THIRDPARTY=%ROOT%\source\thirdparty\freetype-mingw\bin\libfreetype.dll"

set "PATH=%MINGW_BIN%;%QT_BIN%;%PATH%"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

pushd "%BUILD_DIR%" || exit /b 1
"%QT_BIN%\qmake.exe" -o Makefile ../../%PROJECT_NAME%.pro -spec win32-g++ CONFIG+=debug CONFIG+=qml_debug
if errorlevel 1 (
    popd
    exit /b 1
)

"%MINGW_BIN%\mingw32-make.exe"
if errorlevel 1 (
    popd
    exit /b 1
)
popd

if not exist "%BUILD_EXE%" (
    echo Build output not found: "%BUILD_EXE%"
    exit /b 1
)

if not exist "%APP_DIR%" (
    mkdir "%APP_DIR%"
)

copy /Y "%BUILD_EXE%" "%APP_EXE%" >nul
if errorlevel 1 exit /b 1

"%QT_BIN%\windeployqt.exe" --debug --compiler-runtime "%APP_EXE%"
if errorlevel 1 exit /b 1

if not exist "%APP_DIR%\config" (
    mkdir "%APP_DIR%\config"
)
copy /Y "%ROOT%\config\*" "%APP_DIR%\config\" >nul
if errorlevel 1 exit /b 1

if exist "%FREETYPE_DLL%" (
    copy /Y "%FREETYPE_DLL%" "%APP_DIR%\" >nul
) else if exist "%FREETYPE_DLL_THIRDPARTY%" (
    copy /Y "%FREETYPE_DLL_THIRDPARTY%" "%APP_DIR%\" >nul
) else if exist "%APP_DIR%\libfreetype.dll" (
    echo Keep existing FreeType DLL: "%APP_DIR%\libfreetype.dll"
) else (
    echo FreeType DLL not found.
    echo Checked: "%FREETYPE_DLL%"
    echo Checked: "%FREETYPE_DLL_THIRDPARTY%"
    exit /b 1
)

echo Package complete: "%APP_DIR%"
endlocal
