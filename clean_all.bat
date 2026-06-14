@echo off
chcp 65001 >nul
echo ======================================
echo       Qt项目编译文件清理脚本
echo ======================================

:: 清理构建目录
echo [1/3] 清理构建目录...
if exist build (
    rmdir /s /q build
    echo   - build 目录已删除
)
if exist debug (
    rmdir /s /q debug
    echo   - debug 目录已删除
)
if exist release (
    rmdir /s /q release
    echo   - release 目录已删除
)

:: 清理临时文件
echo [2/3] 清理临时文件...
del /s /q *.o *.obj *.exe *.dll *.lib *.exp *.pdb *.ilk *.res >nul 2>&1
del /s /q Makefile Makefile.* *.pro.user >nul 2>&1
del /s /q moc_*.cpp ui_*.h qrc_*.cpp >nul 2>&1
del /s /q *.log *.tmp >nul 2>&1

:: 清理残留目录
echo [3/3] 清理残留目录...
for /d %%i in (debug release Makefile.??) do (
    rmdir /s /q "%%i" >nul 2>&1
)

echo ======================================
echo 清理完成！
pause