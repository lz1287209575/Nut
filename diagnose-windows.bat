@echo off
echo === NutBuildTools Windows 诊断工具 ===
echo.

echo 1. 检查 dotnet 是否可用...
dotnet --version
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: dotnet 未安装或不在 PATH 中
    echo 请安装 .NET 8.0 SDK
    goto :end
)

echo.
echo 2. 检查 NutBuildTools 是否存在...
if not exist "Binary\NutBuildTools\NutBuildTools.dll" (
    echo ERROR: NutBuildTools.dll 不存在
    echo 请先运行: dotnet build Source\Programs\NutBuildTools -c Release --output Binary\NutBuildTools
    goto :end
)

echo.
echo 3. 检查项目根目录...
if not exist "CLAUDE.md" (
    echo ERROR: 未在项目根目录运行此脚本
    echo 请在包含 CLAUDE.md 的目录中运行
    goto :end
)

echo.
echo 4. 检查编译器...
echo 检查 Visual Studio 编译器...
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 找到 MSVC 编译器
    cl.exe
) else (
    echo ! 未找到 MSVC 编译器
)

echo.
echo 检查 Clang 编译器...
where clang++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 找到 Clang 编译器
    clang++.exe --version
) else (
    echo ! 未找到 Clang 编译器
)

echo.
echo 检查 GCC 编译器...
where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ 找到 GCC 编译器
    g++.exe --version
) else (
    echo ! 未找到 GCC 编译器
)

echo.
echo 5. 运行编译测试（详细模式）...
echo 命令: dotnet Binary\NutBuildTools\NutBuildTools.dll --target ServiceAllocate --platform Windows --configuration Debug --log-level Debug
echo.
dotnet Binary\NutBuildTools\NutBuildTools.dll --target ServiceAllocate --platform Windows --configuration Debug --log-level Debug

echo.
echo 诊断完成！
echo.
echo 如果编译失败，请检查：
echo 1. 是否安装了 Visual Studio 2019/2022 并选择了 C++ 工具
echo 2. 是否运行在 "Developer Command Prompt for VS" 中
echo 3. 是否安装了 Windows 10/11 SDK
echo 4. 环境变量 PATH 是否包含编译器路径

:end
pause