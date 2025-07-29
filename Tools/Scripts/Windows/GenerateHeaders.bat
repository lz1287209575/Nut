@echo off
REM GenerateHeaders.bat - 运行NutHeaderTools生成.generate.h文件
REM 集成到Nut引擎构建流程中

setlocal enabledelayedexpansion

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."

REM 定义路径
set "HEADER_TOOL_PROJECT=%PROJECT_ROOT%\Source\Programs\NutHeaderTools\NutHeaderTools.csproj"
set "SOURCE_DIRS=%PROJECT_ROOT%\Source\Runtime\LibNut\Sources;%PROJECT_ROOT%\Source\Runtime\MicroServices;%PROJECT_ROOT%\Source\Runtime\ServiceAllocate\Sources"

echo 🔧 Nut引擎头文件生成工具
echo =========================================

REM 检查.NET是否安装
dotnet --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 未找到.NET SDK，请安装.NET 8.0或更高版本
    exit /b 1
)

REM 构建NutHeaderTools
echo 🏗️  构建NutHeaderTools...
dotnet build "%HEADER_TOOL_PROJECT%" --configuration Release --verbosity quiet

if errorlevel 1 (
    echo ❌ NutHeaderTools构建失败
    exit /b 1
)

echo ✅ NutHeaderTools构建成功
echo.

echo 🔍 开始生成头文件...

set "TOTAL_GENERATED=0"

REM 处理每个源目录
for %%D in (%SOURCE_DIRS%) do (
    if exist "%%D" (
        echo 📁 处理目录: %%~nxD
        
        REM 运行NutHeaderTools
        dotnet run --project "%HEADER_TOOL_PROJECT%" --configuration Release -- "%%D" 2>&1 > temp_output.txt
        
        if !errorlevel! equ 0 (
            REM 检查生成的文件数量
            findstr /c:"成功生成" temp_output.txt >nul
            if !errorlevel! equ 0 (
                echo    ✅ 头文件生成成功
                set /a TOTAL_GENERATED+=1
            ) else (
                echo    ℹ️  该目录无需生成文件
            )
        ) else (
            echo    ❌ 处理失败: %%D
            type temp_output.txt
        )
        
        if exist temp_output.txt del temp_output.txt
    ) else (
        echo    ⚠️  跳过不存在的目录: %%D
    )
)

echo.
echo =========================================
echo 🎉 头文件生成完成!
echo 📊 总计处理: %TOTAL_GENERATED% 个目录

if %TOTAL_GENERATED% gtr 0 (
    echo.
    echo 💡 提示: 已生成新的头文件，建议运行以下命令重新生成IDE项目文件:
    echo    GenerateProjectFiles.bat
)

echo.
pause