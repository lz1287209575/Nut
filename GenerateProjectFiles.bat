@echo off
setlocal enabledelayedexpansion

:: 设置控制台代码页为 UTF-8
chcp 65001 >nul

echo ========================================
echo Nut 项目文件生成器 (Windows)
echo ========================================

:: 设置项目根目录
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

echo.
echo 🔧 正在调用工程化项目生成器...
echo.

::调用Python项目生成器
call "Tools\Scripts\Windows\generate_projects.bat" generate

echo.
echo ========================================
echo 初始化完成！
echo ========================================
echo.
echo 生成的项目文件：
echo   📁 ProjectFiles\ - 各个项目文件目录
echo   📄 ProjectFiles\Nut.sln - Visual Studio 解决方案
echo   🗂️ ProjectFiles\Nut.xcworkspace - XCode 工作空间文件 (macOS)
echo.
echo 使用方法：
echo   🚀 运行项目: 双击 ProjectFiles\Nut.sln 用 Visual Studio 打开
echo   🔄 重新生成项目文件: 运行 GenerateProjectFiles.bat
echo   🧹 仅发现项目: Tools\Scripts\Windows\generate_projects.bat discover
echo.
pause 