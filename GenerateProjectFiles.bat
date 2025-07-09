@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Nut 项目文件生成器 (Windows)
echo ========================================

REM 设置项目根目录
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

REM 1. 生成项目文件
echo 正在生成项目文件...
Tools\ProjectFileGenerator.bat generate

echo 项目文件生成完成！
echo.

REM 2. 刷新IntelliSense
echo 正在刷新IntelliSense...
if exist "Source\Programs\NutBuildTools\Scripts\IntelliSenseRefresher.py" (
    python Source\Programs\NutBuildTools\Scripts\IntelliSenseRefresher.py
    echo IntelliSense刷新完成！
) else (
    echo 未找到 IntelliSenseRefresher.py，跳过 IntelliSense 刷新
)

echo.
echo 生成的项目文件：
echo   📁 Nut.sln - Visual Studio 解决方案
echo   📁 Nut.xcodeproj - Xcode 项目
echo.
echo 所有操作已完成！

pause 