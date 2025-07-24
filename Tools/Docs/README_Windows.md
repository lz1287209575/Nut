# Nut 项目文件生成器 - Windows 使用指南

## 概述

本目录包含用于 Windows 系统的项目文件生成脚本，这些脚本调用工程化的 Python 项目生成器来创建各种 IDE 项目文件。

## 前置要求

### 必需环境
- **Python 3.6+**: 从 [python.org](https://python.org) 下载或通过 Microsoft Store 安装
- **.NET 8.0+ SDK**: 从 [dotnet.microsoft.com](https://dotnet.microsoft.com/download) 下载

### 推荐环境  
- **Visual Studio 2022**: 用于 C++ 开发
- **Protocol Buffers**: 用于协议文件处理

## 可用脚本

### 🚀 主要脚本

#### `generate_projects.bat`
**通用项目文件生成器** - 主要入口脚本

```batch
# 生成所有格式的项目文件
generate_projects.bat

# 仅发现项目
generate_projects.bat discover

# 仅生成 Visual Studio 项目文件
generate_projects.bat vs

# 仅生成 XCode 项目文件 (跨平台)
generate_projects.bat xcode

# 显示帮助信息
generate_projects.bat help

# 详细输出模式
generate_projects.bat vs --verbose
```

#### `VSProjectGenerator.bat`
**Visual Studio 专用生成器** - 专门用于生成 Visual Studio 项目文件

```batch
# 生成 Visual Studio 项目文件
VSProjectGenerator.bat
```

#### `XCodeFileGenerator_New.bat`
**XCode 项目生成器** - 跨平台 XCode 项目文件生成

```batch
# 生成 XCode 项目文件 (主要用于 macOS)
XCodeFileGenerator_New.bat
```

### 🔧 系统脚本

#### `../Setup.bat`
**项目初始化脚本** - 检查环境并创建基础项目文件

```batch
# 初始化项目环境
Setup.bat
```

#### `../GenerateProjectFiles.bat`
**兼容性脚本** - 与原有工作流兼容的主入口

```batch
# 使用传统方式生成项目文件
GenerateProjectFiles.bat
```

## 使用流程

### 1. 首次设置

```batch
# 1. 克隆项目后，首先运行环境检查和初始化
Setup.bat

# 2. 或者直接生成项目文件
GenerateProjectFiles.bat
```

### 2. 日常开发

```batch
# 生成 Visual Studio 项目文件进行 Windows 开发
Tools\generate_projects.bat vs

# 重新生成所有项目文件
Tools\generate_projects.bat generate

# 仅查看项目结构
Tools\generate_projects.bat discover
```

### 3. 专门用途

```batch
# 仅生成 Visual Studio 项目
Tools\VSProjectGenerator.bat

# 生成 XCode 项目用于 macOS 开发
Tools\XCodeFileGenerator_New.bat
```

## 生成的文件

### Visual Studio 项目
- `Projects/Runtime/*.vcxproj` - C++ 项目文件
- `Nut.sln` - Visual Studio 解决方案文件
- 现有的 `Source/Programs/*/*.csproj` - C# 项目文件

### XCode 项目 (跨平台)
- `Projects/Runtime/*.xcodeproj` - XCode 项目文件
- `Nut.xcworkspace` - XCode 工作空间文件

## 故障排除

### 常见问题

#### Python 未找到
```
❌ 错误: 未找到 python 或 python3
```
**解决方案**: 
1. 从 [python.org](https://python.org) 下载安装 Python
2. 或通过 Microsoft Store 安装 Python
3. 确保安装时勾选 "Add Python to PATH"

#### .NET SDK 未找到
```
❌ 错误: 未找到 .NET SDK
```
**解决方案**: 从 [dotnet.microsoft.com](https://dotnet.microsoft.com/download) 下载安装 .NET 8.0+

#### 生成器脚本未找到
```
❌ 错误: 未找到生成器脚本
```
**解决方案**: 确保在项目根目录运行脚本，且 `Tools/ProjectGenerator/` 目录完整

### 目录结构变更

项目采用了新的目录结构：
```
Tools/
├── ProjectGenerator/          # 核心工具
├── Scripts/
│   ├── Windows/              # Windows 批处理脚本
│   └── Unix/                 # Unix/Linux/Mac 脚本
├── Docs/                     # 文档
└── requirements.txt          # 依赖文件
```

### 详细诊断

```batch
# 运行详细输出模式获取更多信息
Tools\generate_projects.bat vs --verbose

# 检查 Python 环境
python --version
python3 --version

# 检查 .NET 环境
dotnet --version
```

## 技术架构

### 脚本层次结构
```
Windows BAT Scripts
        ↓
Python ProjectGenerator
        ↓
Core Modules (Discoverer, Parsers, Generators)
        ↓
Output (XCode, Visual Studio, Solution files)
```

### 工程化特色
- **模块化设计**: Python 核心 + BAT 包装器
- **跨平台支持**: 同时生成多种 IDE 格式
- **智能发现**: 自动识别项目类型和结构
- **完善错误处理**: 友好的错误信息和故障排除
- **兼容性保持**: 与原有工作流无缝集成

## 高级用法

### 自定义参数传递
```batch
# 传递额外参数给 Python 生成器
Tools\generate_projects.bat vs --verbose --custom-option value
```

### 批处理集成
```batch
# 在其他批处理脚本中调用
call Tools\generate_projects.bat vs
if %errorlevel% neq 0 (
    echo 项目生成失败
    exit /b 1
)
```

### CI/CD 集成
```batch
# 无交互模式，适合 CI/CD 
Tools\generate_projects.bat generate > build.log 2>&1
```

## 支持

如有问题或建议，请查看：
1. 运行 `Tools\generate_projects.bat help` 获取帮助
2. 检查生成的日志文件
3. 使用 `--verbose` 模式获取详细信息