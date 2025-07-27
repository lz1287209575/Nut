@echo off
setlocal enabledelayedexpansion

:: Nut 项目文件生成器 (Windows 版本)
:: 这个脚本调用工程化的 Python 项目生成器

:: 设置控制台代码页为 UTF-8
chcp 65001 >nul

:: 获取项目根目录 (脚本所在目录的上两级目录)
set "PROJECT_ROOT=%~dp0..\.."
set "GENERATOR_SCRIPT=%PROJECT_ROOT%\Tools\ProjectGenerator\tool.py"

echo.
echo 🚀 Nut 项目文件生成器 (工程化版本)
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
        echo 💡 安装建议:
        echo   1. 从 https://python.org 下载并安装 Python
        echo   2. 或使用 Microsoft Store 安装 Python
        echo   3. 确保安装时勾选 "Add Python to PATH"
        echo.
        pause
        exit /b 1
    )
    set "PYTHON_CMD=python3"
) else (
    set "PYTHON_CMD=python"
)

:: 显示 Python 版本信息
echo ✅ 检测到 Python 环境:
%PYTHON_CMD% --version
echo.

:: 检查生成器脚本
if not exist "%GENERATOR_SCRIPT%" (
    echo ❌ 错误: 未找到生成器脚本 %GENERATOR_SCRIPT%
    echo.
    echo 💡 请确保以下文件存在:
    echo   - %GENERATOR_SCRIPT%
    echo   - %PROJECT_ROOT%\Tools\ProjectGenerator\ 目录
    echo.
    pause
    exit /b 1
)

:: 解析命令行参数
set "COMMAND=generate"
set "EXTRA_ARGS="

:: 如果有参数，使用第一个参数作为命令
if not "%~1"=="" (
    set "COMMAND=%~1"
    
    :: 收集额外参数
    shift
    :parse_args
    if not "%~1"=="" (
        set "EXTRA_ARGS=!EXTRA_ARGS! %~1"
        shift
        goto parse_args
    )
)

:: 显示帮助信息
if "%COMMAND%"=="help" (
    echo 📖 使用方法:
    echo   %~nx0                    # 生成所有格式的项目文件
    echo   %~nx0 discover           # 仅发现项目，不生成文件
    echo   %~nx0 vs                 # 仅生成 Visual Studio 项目文件
    echo   %~nx0 xcode              # 仅生成 XCode 项目文件 ^(macOS^)
    echo   %~nx0 generate           # 生成所有格式的项目文件
    echo.
    echo 📋 可用命令:
    echo   discover  - 发现并列出项目
    echo   vs        - 生成 Visual Studio 项目文件
    echo   xcode     - 生成 XCode 项目文件
    echo   generate  - 生成所有项目文件
    echo   help      - 显示此帮助信息
    echo.
    pause
    exit /b 0
)

:: 调用 Python 生成器
echo 🔧 调用 Python 项目生成器...
echo 💻 执行命令: %PYTHON_CMD% "%GENERATOR_SCRIPT%" %COMMAND% --project-root "%PROJECT_ROOT%" %EXTRA_ARGS%
echo.

%PYTHON_CMD% "%GENERATOR_SCRIPT%" %COMMAND% --project-root "%PROJECT_ROOT%" %EXTRA_ARGS%

:: 检查执行结果
if %errorlevel% equ 0 (
    echo.
    echo ✅ 项目文件生成完成！
    echo.
    
    :: 根据命令类型显示不同的后续提示
    if "%COMMAND%"=="xcode" (
        echo 🍎 XCode 项目文件已生成:
        echo   📁 ProjectFiles\ - 各个项目的 .xcodeproj 文件
        echo   🗂️ ProjectFiles\Nut.xcworkspace - 主工作空间文件
        echo.
        echo 💡 使用方法 ^(需要 macOS^):
        echo   1. 使用 Xcode 打开 ProjectFiles\Nut.xcworkspace
        echo   2. 或者直接打开单个项目的 .xcodeproj 文件
    ) else if "%COMMAND%"=="vs" (
        echo 🔵 Visual Studio 项目文件已生成:
        echo   📁 ProjectFiles\ - 各个项目的 .vcxproj 文件
        echo   📄 ProjectFiles\Nut.sln - Visual Studio 解决方案文件
        echo.
        echo 💡 使用方法:
        echo   1. 双击 ProjectFiles\Nut.sln 用 Visual Studio 打开
        echo   2. 或者直接打开单个项目的 .vcxproj 文件
    ) else if "%COMMAND%"=="discover" (
        echo 🔍 项目发现完成！
        echo.
        echo 💡 后续操作:
        echo   - 运行 %~nx0 生成所有项目文件
        echo   - 运行 %~nx0 vs 生成 Visual Studio 项目文件
        echo   - 运行 %~nx0 xcode 生成 XCode 项目文件
    ) else (
        echo 📁 生成的文件:
        echo   📁 ProjectFiles\ - 各个项目文件
        echo   📄 ProjectFiles\Nut.sln - Visual Studio 解决方案文件
        echo   🗂️ ProjectFiles\Nut.xcworkspace - XCode 工作空间文件
        echo.
        echo 💡 使用方法:
        echo   - Windows: 双击 ProjectFiles\Nut.sln 用 Visual Studio 打开
        echo   - macOS: 使用 Xcode 打开 ProjectFiles\Nut.xcworkspace
    )
    
    echo.
    echo 🎉 操作成功完成！
) else (
    echo.
    echo ❌ 项目文件生成失败！
    echo.
    echo 💡 故障排除:
    echo   1. 检查 Python 环境是否正确安装
    echo   2. 确保项目根目录结构完整
    echo   3. 查看上方的错误信息进行诊断
    echo.
    echo 🔍 如需详细信息，请运行:
    echo   %PYTHON_CMD% "%GENERATOR_SCRIPT%" %COMMAND% --verbose --project-root "%PROJECT_ROOT%"
)

echo.
pause