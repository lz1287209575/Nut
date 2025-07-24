@echo off
setlocal enabledelayedexpansion

:: Nut XCode 项目文件生成器 (Windows 版本)
:: 这个脚本调用工程化的 Python 项目生成器

:: 设置控制台代码页为 UTF-8
chcp 65001 >nul

:: 获取项目根目录 (脚本所在目录的上两级目录)
set "PROJECT_ROOT=%~dp0..\.."
set "GENERATOR_SCRIPT=%PROJECT_ROOT%\Tools\ProjectGenerator\tool.py"

echo.
echo 🚀 Nut XCode 项目文件生成器 (工程化版本)
echo 📁 项目根目录: %PROJECT_ROOT%
echo.

:: 检查 Python 环境
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到 python，正在尝试 python3...
    python3 --version >nul 2>&1
    if !errorlevel! neq 0 (
        echo ❌ 错误: 未找到 python 或 python3，请先安装 Python 3.6+
        echo.
        pause
        exit /b 1
    )
    set "PYTHON_CMD=python3"
) else (
    set "PYTHON_CMD=python"
)

:: 检查生成器脚本
if not exist "%GENERATOR_SCRIPT%" (
    echo ❌ 错误: 未找到生成器脚本 %GENERATOR_SCRIPT%
    echo.
    pause
    exit /b 1
)

:: 提醒用户这是为 macOS 设计的
echo ⚠️  注意: XCode 项目文件主要用于 macOS 开发环境
echo 💡 如果您在 Windows 上开发，建议使用:
echo   - %~dpn0.bat generate  (生成所有项目文件，包括 Visual Studio)
echo.

set /p "continue=是否继续生成 XCode 项目文件? (y/N): "
if /i not "%continue%"=="y" if /i not "%continue%"=="yes" (
    echo 操作已取消
    pause
    exit /b 0
)

echo.
echo 🔧 调用 Python 项目生成器...
%PYTHON_CMD% "%GENERATOR_SCRIPT%" xcode --project-root "%PROJECT_ROOT%" %*

if %errorlevel% equ 0 (
    echo.
    echo ✅ XCode 项目文件生成完成！
    echo.
    echo 生成的文件:
    echo   📁 Projects\ - 各个项目的 .xcodeproj 文件
    echo   🗂️ Nut.xcworkspace - 主工作空间文件
    echo.
    echo 使用方法 (需要 macOS):
    echo   1. 将项目传输到 macOS 系统
    echo   2. 使用 Xcode 打开 Nut.xcworkspace
    echo   3. 或者直接打开单个项目的 .xcodeproj 文件
    echo.
    echo 🎉 操作成功完成！
) else (
    echo.
    echo ❌ XCode 项目文件生成失败！
    echo 请检查上方的错误信息进行诊断
)

echo.
pause