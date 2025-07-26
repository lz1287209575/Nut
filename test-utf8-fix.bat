@echo off
echo === 测试 UTF-8 编码修复 ===
echo.

echo 检查 NutBuildTools 是否已构建...
if not exist "Binary\NutBuildTools\NutBuildTools.dll" (
    echo 构建 NutBuildTools...
    dotnet build Source\Programs\NutBuildTools -c Release --output Binary\NutBuildTools
    if %ERRORLEVEL% NEQ 0 (
        echo 构建失败！
        pause
        exit /b 1
    )
)

echo.
echo 重新生成项目文件（包含中间目录修复）...
python Tools\ProjectGenerator\cli.py generate

echo.
echo 测试 LibNut 编译（应该包含 /utf-8 标志）...
dotnet Binary\NutBuildTools\NutBuildTools.dll --target LibNut --platform Windows --configuration Debug --log-level Debug

echo.
echo 如果看到以下内容说明修复成功：
echo 1. 编译命令包含 "/utf-8" 标志
echo 2. 没有 "Unicode support requires compiling with /utf-8" 错误
echo 3. Visual Studio 项目使用独立的中间目录（IntDir）

echo.
echo 检查 Visual Studio 项目中间目录设置...
findstr "IntDir" Projects\Runtime\LibNut.vcxproj
findstr "OutDir" Projects\Runtime\LibNut.vcxproj

pause