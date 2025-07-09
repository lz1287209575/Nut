@echo off
setlocal enabledelayedexpansion

echo =========================================
echo Nut 项目初始化脚本 (Windows)
echo =========================================

REM 设置项目根目录
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

echo [1/4] 检查.NET环境...
dotnet --version >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到 .NET SDK，请先安装 .NET 8.0+
    echo 下载地址: https://dotnet.microsoft.com/download
    pause
    exit /b 1
)
echo .NET SDK 环境检查通过
dotnet --version

echo [2/4] 检查C++编译器...
where cl >nul 2>&1
if errorlevel 1 (
    echo 警告: 未检测到 Visual Studio 编译器 (cl.exe)
    echo 请确保已安装 Visual Studio 或 Visual Studio Build Tools
) else (
    echo 检测到 Visual Studio 编译器
)

echo [3/4] 检查Protobuf编译器 (protoc)...
where protoc >nul 2>&1
if errorlevel 1 (
    echo 警告: 未检测到 protoc，请手动安装 Protobuf
    echo 下载地址: https://github.com/protocolbuffers/protobuf/releases
) else (
    echo 检测到 Protobuf 编译器 (protoc)
)

echo [4/4] 创建基础IDE项目文件...
echo 正在创建基础项目文件...
Tools\ProjectFileGenerator.bat setup

echo.
echo =========================================
echo 初始化完成！
echo =========================================
echo.
echo 创建的基础项目文件：
echo   📁 Nut.sln - Visual Studio 解决方案
echo   📁 Nut.xcodeproj - Xcode 项目
echo.
echo 使用方法：
echo   🚀 运行项目: Setup.bat
echo   🔄 重新生成项目文件: GenerateProjectFiles.bat
echo   🧹 清理构建: RefreshIntelliSense.bat
echo.

pause 