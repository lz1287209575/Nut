# Windows 编译问题排查指南

如果你在 Windows 上使用 NutBuildTools 时遇到编译失败，请按照以下步骤进行排查：

## 快速诊断

运行诊断脚本：
```cmd
diagnose-windows.bat
```

## 常见问题及解决方案

### 1. 编译器未找到

**症状**: `"未找到可用的 C++ 编译器"`

**解决方案**:
- 安装 Visual Studio 2019 或 2022
- 在安装时确保选择 "C++ build tools" 和 "Windows 10/11 SDK"
- 或者安装 "Build Tools for Visual Studio" (轻量级选项)

### 2. 环境变量问题

**症状**: `"找不到 cl.exe"` 或类似错误

**解决方案**:
- 使用 "Developer Command Prompt for VS" 而不是普通 cmd
- 或者运行 `vcvarsall.bat` 设置环境变量：
  ```cmd
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
  ```

### 3. 缺少 Windows SDK

**症状**: 编译时提示找不到 `windows.h` 或系统库

**解决方案**:
- 通过 Visual Studio Installer 安装 Windows 10/11 SDK
- 确保 SDK 版本与 Visual Studio 兼容

### 4. 权限问题

**症状**: 无法创建输出文件或目录

**解决方案**:
- 以管理员身份运行命令提示符
- 检查项目目录是否有写权限

### 5. 路径中包含空格

**症状**: 编译命令失败，路径被截断

**解决方案**:
- 确保项目路径不包含空格和特殊字符
- 或者将项目移到简短路径如 `C:\Dev\Nut`

## 详细排查步骤

### 第一步：验证环境

1. 检查 .NET SDK:
   ```cmd
   dotnet --version
   ```

2. 检查编译器:
   ```cmd
   where cl.exe
   where clang++.exe
   where g++.exe
   ```

3. 检查项目根目录:
   ```cmd
   dir CLAUDE.md
   ```

### 第二步：运行详细诊断

```cmd
dotnet Binary\NutBuildTools\NutBuildTools.dll --target ServiceAllocate --platform Windows --configuration Debug --log-level Debug
```

### 第三步：分析错误日志

常见错误模式：

1. **编译器错误**:
   - 错误码 C1083: 找不到包含文件
   - 错误码 LNK2019: 无法解析的外部符号

2. **链接器错误**:
   - 缺少系统库 (kernel32.lib, user32.lib)
   - 对象文件格式不兼容

## 环境配置示例

### Visual Studio 2022 配置

1. 安装 Visual Studio 2022 Community
2. 选择工作负载: "使用 C++ 的桌面开发"
3. 在单个组件中确保选择了:
   - MSVC v143 编译器工具集
   - Windows 10/11 SDK (最新版本)
   - CMake tools for Visual Studio

### 环境变量设置

如果使用普通命令提示符，设置以下环境变量：

```cmd
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC
set PATH=%VCINSTALLDIR%\Tools\MSVC\14.30.30705\bin\Hostx64\x64;%PATH%
set INCLUDE=%VCINSTALLDIR%\Tools\MSVC\14.30.30705\include;%INCLUDE%
set LIB=%VCINSTALLDIR%\Tools\MSVC\14.30.30705\lib\x64;%LIB%
```

## 替代方案

如果 MSVC 持续有问题，可以尝试：

1. **使用 Clang**:
   - 安装 LLVM/Clang
   - 设置 PATH 包含 clang++.exe

2. **使用 MinGW**:
   - 安装 MinGW-w64
   - 设置 PATH 包含 g++.exe

## 获取帮助

如果问题仍未解决，请提供以下信息：

1. 运行 `diagnose-windows.bat` 的完整输出
2. Windows 版本 (运行 `winver`)
3. Visual Studio 版本和安装的组件
4. 完整的错误日志 (使用 `--log-level Debug`)

将这些信息发送到项目维护者以获得帮助。