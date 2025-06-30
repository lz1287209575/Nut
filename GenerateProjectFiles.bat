@echo off
chcp 65001 >nul
echo ========================================
echo Nut 项目文件生成器 (Windows)
echo ========================================

REM 设置项目根目录
set PROJECT_ROOT=%~dp0
cd /d "%PROJECT_ROOT%"

echo 正在生成项目文件...

REM 调用Python脚本生成项目文件
python BuildSystem\BuildScripts\ProjectFileGenerator.py

echo 项目文件生成完成！
echo.
echo 正在刷新IntelliSense...

REM 调用Python脚本刷新IntelliSense
python BuildSystem\BuildScripts\IntelliSenseRefresher.py

echo IntelliSense刷新完成！
echo.
echo 所有操作已完成！
pause 