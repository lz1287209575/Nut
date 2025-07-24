@echo off
setlocal enabledelayedexpansion

:: Nut Visual Studio 项目文件生成器 (Windows 版本)
:: 这个脚本调用工程化的 Python 项目生成器专门生成 Visual Studio 项目文件

:: 设置控制台代码页为 UTF-8
chcp 65001 >nul

:: 获取项目根目录 (脚本所在目录的上两级目录)
set "PROJECT_ROOT=%~dp0..\.."
set "GENERATOR_SCRIPT=%PROJECT_ROOT%\Tools\ProjectGenerator\tool.py"

echo.
echo 🔵 Nut Visual Studio 项目文件生成器 (工程化版本)
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

echo 🔧 调用 Python 项目生成器 (Visual Studio 模式)...
%PYTHON_CMD% "%GENERATOR_SCRIPT%" vs --project-root "%PROJECT_ROOT%" %*

if %errorlevel% equ 0 (
    echo.
    echo ✅ Visual Studio 项目文件生成完成！
    echo.
    echo 生成的文件:
    echo   📁 Projects\ - 各个项目的 .vcxproj 文件
    echo   📄 Nut.sln - Visual Studio 解决方案文件
    echo.
    echo 使用方法:
    echo   1. 双击 Nut.sln 用 Visual Studio 打开
    echo   2. 或者直接打开单个项目的 .vcxproj 文件
    echo   3. 在 Visual Studio 中构建和调试项目
    echo.
    echo 💡 提示:
    echo   - 确保已安装 Visual Studio 2022 或更新版本
    echo   - 项目使用 C++20 标准，需要现代编译器
    echo   - 可以在 Visual Studio 中直接编辑源文件
    echo.
    echo 🎉 操作成功完成！
) else (
    echo.
    echo ❌ Visual Studio 项目文件生成失败！
    echo 请检查上方的错误信息进行诊断
)

echo.
pause