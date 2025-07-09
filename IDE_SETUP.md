# Nut IDE 设置指南

## 🚀 快速开始

Nut 项目现在支持类似 UE 的 IDE 项目文件生成系统，可以自动生成 Visual Studio 解决方案和 Xcode 项目文件。

### 系统要求

- **.NET 8.0 SDK** 或更高版本
- **C++ 编译器** (Windows: MSVC, macOS/Linux: GCC/Clang)
- **Protobuf 编译器** (可选，用于协议缓冲区)

### 首次设置

#### Windows 用户
```bash
# 运行 Windows 设置脚本
Setup.bat
```

#### macOS/Linux 用户
```bash
# 运行 Unix 设置脚本
./Setup.sh
```

### 重新生成项目文件

#### Windows 用户
```bash
# 重新生成项目文件
GenerateProjectFiles.bat
```

#### macOS/Linux 用户
```bash
# 重新生成项目文件
./GenerateProjectFiles.sh
```

## 📁 生成的文件

设置完成后，会在项目根目录生成以下文件：

- **`Nut.sln`** - Visual Studio 解决方案文件
- **`Nut.xcodeproj/`** - Xcode 项目文件夹

## 🛠️ 支持的 IDE

### Visual Studio / Visual Studio Code
1. 打开 `Nut.sln` 文件
2. 解决方案包含所有 C++ 和 C# 项目，按文件夹分组
3. 支持智能感知和调试

### Xcode
1. 打开 `Nut.xcodeproj` 文件夹
2. 包含所有 C++ 项目，按文件夹分组
3. 支持 macOS 和 iOS 开发

### JetBrains Rider / CLion
1. 打开项目根目录
2. 自动识别生成的项目文件
3. 支持跨平台开发

## 🔧 项目发现机制

系统会自动发现以下类型的项目：

### C++ 项目
- 扫描 `Source/` 目录下的所有 `.cpp` 文件
- 按目录组织项目结构
- 自动包含对应的头文件

### C# 项目
- 扫描所有 `.csproj` 文件
- 支持 .NET 项目配置
- 自动解析项目依赖

## 📂 项目分组功能

### 自动分组
系统会根据 `Source/` 目录下的一级文件夹自动对项目进行分组：

```
Source/
├── Programs/          # 📂 Programs 分组
│   └── NutBuildTools/
├── Runtime/           # 📂 Runtime 分组
│   ├── LibNut/
│   ├── MicroServices/
│   └── ServiceAllocate/
└── Other/             # 📂 Other 分组（如果存在）
```

### 分组效果
在 Visual Studio 中，项目会按以下方式组织：

```
Nut.sln
├── 📂 Programs
│   └── NutBuildTools (C#)
├── 📂 Runtime
│   ├── Sources (C++)
│   └── UnitTest (C++)
└── 📂 Other
    └── (其他项目)
```

## 🏗️ 架构设计

### 纯脚本实现
为了避免循环依赖，项目文件生成器使用纯脚本实现：

```
Tools/
├── ProjectFileGenerator.sh    # Unix 版本
└── ProjectFileGenerator.bat   # Windows 版本
```

### 项目发现接口
系统提供了可扩展的项目发现机制：

```bash
# 自定义项目发现逻辑
discover_projects() {
    # 扫描 C++ 项目
    # 扫描 C# 项目
    # 按分组组织
}
```

### 分组逻辑
分组基于 Source 目录的一级文件夹：

```bash
get_project_group() {
    # 提取第一级目录名
    # 返回分组名称
}
```

## 📋 设置脚本功能

### Setup 脚本执行以下步骤：

1. **环境检查**
   - 检查 .NET SDK 环境
   - 检查 C++ 编译器 (g++/clang++/MSVC)
   - 检查 Protobuf 编译器

2. **项目文件生成**
   - 创建基础项目文件结构
   - 生成 Visual Studio 解决方案
   - 生成 Xcode 项目文件

### GenerateProjectFiles 脚本执行以下步骤：

1. **项目发现**
   - 扫描 Source 目录
   - 发现 C++ 和 C# 项目
   - 按一级文件夹分组

2. **项目文件更新**
   - 更新 Nut.sln 文件
   - 添加项目到对应分组
   - 更新 Nut.xcodeproj 文件

## 🔄 更新项目文件

当添加新文件或修改项目结构时，需要重新生成项目文件：

```bash
# Unix 系统
./GenerateProjectFiles.sh

# Windows 系统
GenerateProjectFiles.bat
```

## 🐛 故障排除

### 常见问题

1. **.NET SDK 未找到**
   - 确保已安装 .NET 8.0 SDK 或更高版本
   - 下载地址: https://dotnet.microsoft.com/download
   - 检查 PATH 环境变量

2. **编译器未找到**
   - Windows: 安装 Visual Studio 或 Build Tools
   - macOS: 安装 Xcode Command Line Tools
   - Linux: 安装 gcc/g++ 或 clang/clang++

3. **项目文件生成失败**
   - 检查文件权限
   - 确保脚本可执行
   - 查看错误日志

### 手动生成

如果自动生成失败，可以手动运行：

```bash
# Unix 系统
Tools/ProjectFileGenerator.sh generate

# Windows 系统
Tools\ProjectFileGenerator.bat generate
```

## 📝 自定义配置

### 修改项目发现规则

编辑 `Tools/ProjectFileGenerator.sh` 文件中的 `discover_projects` 函数：

```bash
discover_projects() {
    # 自定义项目发现逻辑
    # 扫描特定文件类型
    # 自定义分组规则
}
```

### 修改分组逻辑

编辑 `get_project_group` 函数：

```bash
get_project_group() {
    # 自定义分组规则
    # 基于路径、文件名等
}
```

### 添加新的 IDE 支持

在脚本中添加新的生成方法：

```bash
generate_your_ide_project() {
    # 生成你的 IDE 项目文件
    # 支持分组功能
}
```

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进项目文件生成系统！

---

**注意**: 这个系统模仿了 UE 的设置流程，提供了跨平台的 IDE 支持。生成的项目文件会自动包含所有发现的项目，并按一级文件夹进行分组，方便在不同 IDE 中进行开发。系统使用纯脚本实现，避免了循环依赖问题。 