@echo off
REM NutBuildTools 快捷调用脚本 (Windows)
REM 使用方法: nutbuild.bat --target TestTarget --platform Windows --configuration Debug

set "SCRIPT_DIR=%~dp0"
set "NUTBUILDTOOLS_DLL=%SCRIPT_DIR%Binary\NutBuildTools\NutBuildTools.dll"

if not exist "%NUTBUILDTOOLS_DLL%" (
    echo 错误: 未找到 NutBuildTools.dll
    echo 请确保已构建 NutBuildTools 项目
    exit /b 1
)

REM 传递所有参数给 NutBuildTools
dotnet "%NUTBUILDTOOLS_DLL%" %*