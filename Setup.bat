@echo off
chcp 65001 >nul
echo ========================================
echo Nut 项目初始化脚本 (Windows)
echo ========================================

REM 设置项目根目录
set PROJECT_ROOT=%~dp0
cd /d "%PROJECT_ROOT%"

echo [1/4] 安装Python依赖...
pip install -r BuildSystem\Requirements.txt

REM 检查g++/cl.exe
where g++.exe >nul 2>nul
if %errorlevel%==0 (
    echo [2/4] 检测到g++编译器
) else (
    where cl.exe >nul 2>nul
    if %errorlevel%==0 (
        echo [2/4] 检测到cl.exe编译器
    ) else (
        echo [2/4] 未检测到g++或cl.exe，请手动安装MinGW或Visual Studio C++工具集！
    )
)

REM 检查Tools\protoc\bin\protoc.exe
if exist Tools\protoc\bin\protoc.exe (
    echo [3/4] 检测到本地Protobuf编译器 (Tools\protoc\bin\protoc.exe)
) else (
    echo [3/4] 未检测到本地protoc，正在自动下载安装...
    setlocal enabledelayedexpansion
    set ARCH=
    wmic os get osarchitecture | find "64" >nul && set ARCH=win64
    wmic os get osarchitecture | find "32" >nul && set ARCH=win32
    if "%ARCH%"=="" set ARCH=win64
    set PROTOC_URL=https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protoc-21.12-!ARCH!.zip
    echo 下载: !PROTOC_URL!
    powershell -Command "Invoke-WebRequest -Uri !PROTOC_URL! -OutFile protoc.zip"
    powershell -Command "Expand-Archive -Path protoc.zip -DestinationPath Tools\\protoc"
    del protoc.zip
    endlocal
    if exist Tools\protoc\bin\protoc.exe (
        echo [3/4] Protobuf (protoc) 安装成功
    ) else (
        echo [3/4] Protobuf (protoc) 安装失败，请手动安装
    )
)

REM 安装Python protobuf包
echo [4/4] 安装Python protobuf包...
pip install protobuf

echo.
echo 初始化完成！
echo 请将 Tools\\protoc\\bin 添加到你的 PATH 环境变量，或直接用本地protoc
pause 