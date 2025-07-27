@echo off
setlocal enabledelayedexpansion

:: 设置控制台代码页为 UTF-8
chcp 65001 >nul

echo =========================================
echo Nut 项目初始化脚本 (Windows)
echo =========================================

:: 设置项目根目录
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

echo.
echo [1/4] 检查 Python 环境...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 未找到 python，正在尝试 python3...
    python3 --version >nul 2>&1
    if !errorlevel! neq 0 (
        echo ❌ 错误: 未找到 python 或 python3，请先安装 Python 3.6+
        echo 💡 下载地址: https://python.org 或 Microsoft Store
        pause
        exit /b 1
    )
    set "PYTHON_CMD=python3"
    echo ✅ 检测到 Python 3
    python3 --version
) else (
    set "PYTHON_CMD=python"
    echo ✅ 检测到 Python
    python --version
)

echo.
echo [2/4] 检查 .NET 环境...
dotnet --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到 .NET SDK，请先安装 .NET 8.0+
    echo 💡 下载地址: https://dotnet.microsoft.com/download
    pause
    exit /b 1
) else (
    echo ✅ 检测到 .NET SDK
    dotnet --version
)

echo.
echo [3/4] 检查 C++ 编译器...
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ⚠️ 警告: 未检测到 Visual Studio 编译器 (cl.exe)
    echo 💡 请确保已安装 Visual Studio 或 Visual Studio Build Tools
) else (
    echo ✅ 检测到 Visual Studio 编译器
)

echo.
echo [4/4] 检查 Protobuf 编译器 (protoc)...
where protoc >nul 2>&1
if %errorlevel% neq 0 (
    echo ⚠️ 警告: 未检测到 protoc，请手动安装 Protobuf
    echo 💡 下载地址: https://github.com/protocolbuffers/protobuf/releases
) else (
    echo ✅ 检测到 Protobuf 编译器 (protoc)
)

echo.
echo [5/5] 创建基础 IDE 项目文件...
echo 🔧 正在创建基础项目文件...
call "Tools\Scripts\Windows\generate_projects.bat" generate

echo.
echo =========================================
echo 初始化完成！
echo =========================================
echo.
echo 创建的基础项目文件：
echo   📁 ProjectFiles\ - 各个项目文件目录
echo   📄 ProjectFiles\Nut.sln - Visual Studio 解决方案
echo   🗂️ ProjectFiles\Nut.xcworkspace - XCode 工作空间文件 (macOS)
echo.
echo 使用方法：
echo   🚀 开发项目: 双击 ProjectFiles\Nut.sln 用 Visual Studio 打开
echo   🔄 重新生成项目文件: 运行 GenerateProjectFiles.bat
echo   🧹 仅发现项目: Tools\Scripts\Windows\generate_projects.bat discover
echo.
pause 