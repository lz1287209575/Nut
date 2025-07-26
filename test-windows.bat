@echo off
echo === Windows NutBuildTools 测试 ===
echo.

rem 设置详细日志级别
set LOG_LEVEL=Debug

echo 测试编译器检测...
dotnet Binary\NutBuildTools\NutBuildTools.dll --target ServiceAllocate --platform Windows --configuration Debug --log-level Debug

echo.
echo 如果上面的命令失败，请查看详细的错误信息。
echo 常见问题解决方法：
echo 1. 确保安装了 Visual Studio 和 C++ 工具链
echo 2. 确保安装了 Windows 10 SDK
echo 3. 运行 "Developer Command Prompt for VS" 而不是普通命令提示符
echo 4. 检查环境变量 PATH 是否包含编译器路径

pause