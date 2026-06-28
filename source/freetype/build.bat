@echo off
setlocal enabledelayedexpansion

rem ================================
rem FreeType MinGW build script
rem ================================

rem 当前脚本所在目录，作为工程根目录
set PROJECT_ROOT=%~dp0

rem FreeType 源码目录
set FREETYPE_SRC=%PROJECT_ROOT%\freetype-2.14.3

rem FreeType 编译目录
set FREETYPE_BUILD=%PROJECT_ROOT%build\freetype-build-mingw

rem FreeType 安装目录，Qt 工程最终引用这个目录
set FREETYPE_INSTALL=%PROJECT_ROOT%out


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
echo ===== Check FreeType source =====

if not exist "%FREETYPE_SRC%\CMakeLists.txt" (
    echo [ERROR] FreeType source not found:
    echo %FREETYPE_SRC%
    pause
    exit /b 1
)

echo [OK] FreeType source:
echo %FREETYPE_SRC%


echo.
echo ===== Clean old build =====

if exist "%FREETYPE_BUILD%" (
    echo Remove old build dir:
    echo %FREETYPE_BUILD%
    rmdir /s /q "%FREETYPE_BUILD%"
)

mkdir "%FREETYPE_BUILD%"


echo.
echo ===== Configure FreeType =====

cd /d "%FREETYPE_BUILD%"

cmake -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%FREETYPE_INSTALL%" ^
    -DBUILD_SHARED_LIBS=ON ^
    -DFT_DISABLE_HARFBUZZ=TRUE ^
    -DFT_DISABLE_BROTLI=TRUE ^
    -DFT_DISABLE_BZIP2=TRUE ^
    -DFT_DISABLE_PNG=TRUE ^
    -DFT_DISABLE_ZLIB=TRUE ^
    "%FREETYPE_SRC%"

if errorlevel 1 (
    echo [ERROR] cmake configure failed.
    pause
    exit /b 1
)


echo.
echo ===== Build FreeType =====

mingw32-make -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] build failed.
    pause
    exit /b 1
)


echo.
echo ===== Install FreeType =====

mingw32-make install

if errorlevel 1 (
    echo [ERROR] install failed.
    pause
    exit /b 1
)


echo.
echo ===== Check install result =====

if exist "%FREETYPE_INSTALL%\include\freetype2\ft2build.h" (
    echo [OK] ft2build.h found.
) else (
    echo [ERROR] ft2build.h not found.
    pause
    exit /b 1
)

if exist "%FREETYPE_INSTALL%\lib\libfreetype.dll.a" (
    echo [OK] libfreetype.dll.a found.
) else (
    echo [WARNING] libfreetype.dll.a not found.
)

if exist "%FREETYPE_INSTALL%\bin\libfreetype-6.dll" (
    echo [OK] libfreetype-6.dll found.
) else (
    echo [WARNING] libfreetype-6.dll not found.
)

echo.
echo ===== FreeType build success =====
echo Install path:
echo %FREETYPE_INSTALL%
echo.

pause
endlocal