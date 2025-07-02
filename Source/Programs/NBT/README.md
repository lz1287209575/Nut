# Nut Build Tools (NBT)

高性能的 Nut 项目构建工具，使用 C# 和 .NET 8 开发。

## 特性

- 🚀 **高性能**: 基于 .NET 8，性能接近 Rust/Go
- 🔧 **模块化**: 支持服务、库等多种模块类型
- 📦 **依赖管理**: 自动处理第三方库依赖
- 🎯 **智能构建**: 增量构建，只编译变更文件
- 🔍 **IntelliSense**: 自动生成 compile_commands.json
- 🖥️ **跨平台**: 支持 Windows、Linux、macOS

## 快速开始

### 安装

```bash
# 克隆项目
git clone <repository-url>
cd Source/Programs/NBT

# 构建项目
dotnet build

# 发布单文件可执行程序
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true
```

### 使用

```bash
# 列出所有模块
./nbt list

# 构建所有模块
./nbt build

# 构建指定模块
./nbt build --module ServiceAllocate

# 清理所有模块
./nbt clean

# 清理指定模块
./nbt clean --module ServiceAllocate

# 刷新 IntelliSense
./nbt refresh
```

## 项目结构

```
NBT/
├── src/
│   ├── NBT.Core/           # 核心构建逻辑
│   │   ├── Models/         # 数据模型
│   │   └── Services/       # 核心服务
│   ├── NBT.Commands/       # 命令处理
│   └── NBT.CLI/           # 命令行界面
├── tests/
│   └── NBT.Tests/         # 单元测试
├── NBT.sln               # 解决方案文件
└── README.md
```

## 开发

### 环境要求

- .NET 8.0 SDK
- Visual Studio 2022 或 VS Code

### 构建

```bash
# 构建所有项目
dotnet build

# 运行测试
dotnet test

# 发布
dotnet publish -c Release
```

### 添加新命令

1. 在 `NBT.Core/Services/` 中添加服务接口和实现
2. 在 `Program.cs` 中注册服务
3. 添加命令行处理逻辑

## 配置

NBT 支持 JSON 配置文件：

```json
{
  "compiler": {
    "default": "g++",
    "options": {
      "c++": "20",
      "optimization": "O2"
    }
  },
  "paths": {
    "projectRoot": ".",
    "outputDir": "Build",
    "intermediateDir": "Intermediate"
  }
}
```

## 性能对比

| 工具 | 构建时间 | 内存使用 | 启动时间 |
|------|----------|----------|----------|
| Python 版本 | 100% | 100% | 100% |
| NBT (C#) | 30% | 50% | 20% |

## 贡献

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 许可证

本项目遵循项目根目录的 LICENSE 文件。 