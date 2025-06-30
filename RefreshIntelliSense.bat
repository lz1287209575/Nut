@echo off
chcp 65001 >nul
echo ========================================
echo Nut IntelliSense刷新器 (Windows)
echo ========================================

REM 设置项目根目录
set PROJECT_ROOT=%~dp0
cd /d "%PROJECT_ROOT%"

echo 正在刷新IntelliSense...

REM 调用Python脚本刷新IntelliSense
python BuildSystem\BuildScripts\IntelliSenseRefresher.py

echo IntelliSense刷新完成！
pause 