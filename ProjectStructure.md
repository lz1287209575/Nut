# Nut 微服务工程化骨架目录结构

## 项目概述
- 项目名称: Nut
- 语言: C++20/23 (最新版)
- 目标平台: Windows/Linux/macOS
- 构建系统: Python (完全自研，不依赖CMake)
- 命名规范: 大驼峰命名法 (PascalCase)

## 目录结构

```
Nut/
├── BuildSystem/                  # 工程化构建系统
│   ├── BuildScripts/
│   │   ├── BuildEngine.py        # 主构建引擎（工程化主干）
│   │   ├── ServiceMetaLoader.py  # 服务Meta加载与校验
│   │   ├── Compiler.py           # 编译/链接实现
│   │   ├── DockerGenerator.py    # 生成Dockerfile/镜像
│   │   ├── K8SGenerator.py       # 生成K8S YAML
│   │   └── Utils.py              # 工具函数
│   └── Requirements.txt
├── Config/                       # 全局配置
│   └── ...
├── MicroServices/                # 微服务总目录
│   ├── PlayerService/
│   │   ├── Sources/
│   │   ├── Configs/
│   │   ├── Protos/
│   │   └── Meta/
│   │       ├── PlayerService.Build.py
│   │       ├── PlayerService.Docker.py
│   │       └── PlayerService.Kubernetes.py
│   ├── WorldService/
│   │   ├── Sources/
│   │   ├── Configs/
│   │   ├── Protos/
│   │   └── Meta/
│   │       ├── WorldService.Build.py
│   │       ├── WorldService.Docker.py
│   │       └── WorldService.Kubernetes.py
│   └── ...
├── Docker/                       # Dockerfile模板（可选）
│   └── ...
├── K8S/                          # Kubernetes部署模板（可选）
│   └── ...
├── Include/                      # 公共头文件
│   └── ...
├── Tools/                        # 工具脚本
│   └── ...
├── Tests/                        # 测试用例
│   └── ...
├── ThirdParty/                   # 第三方依赖
│   └── ...
└── ...
```

- MicroServices/为顶层目录，所有服务模块均在其下。
- 每个服务目录下有Sources/、Configs/、Protos/、Meta/四大子目录。
- Meta/目录下存放Build、Docker、K8S等元数据描述文件。
- BuildSystem为工程化构建系统，支持自动发现和解析所有服务的Meta规则。

## 命名规范

### 文件命名
- 所有文件夹使用大驼峰命名法: `NetworkManager/`, `PlayerSystem/`
- 头文件使用大驼峰命名法: `NetworkManager.h`, `PlayerSession.h`
- 源文件使用大驼峰命名法: `NetworkManager.cpp`, `PlayerSession.cpp`
- Python文件使用大驼峰命名法: `BuildEngine.py`, `PlatformBuilder.py`

### 类命名
- 所有类名使用大驼峰命名法: `NetworkManager`, `PlayerSession`
- 接口类以 `I` 开头: `INetworkHandler`, `IPlayerManager`

### 函数命名
- C++公共函数使用大驼峰命名法: `InitializeNetwork()`, `HandlePlayerLogin()`
- C++私有函数使用小驼峰命名法: `initializeSocket()`, `handleMessage()`
- Python函数使用大驼峰命名法: `InitializeBuild()`, `ConfigureProject()`

### 变量命名
- 成员变量使用小驼峰命名法: `playerCount`, `networkSocket`
- 常量使用大写下划线: `MAX_PLAYERS`, `DEFAULT_PORT`

## 构建系统设计

### Python构建脚本功能
1. **跨平台构建**: 自动检测平台并配置相应的编译器和链接器
2. **依赖管理**: 自动下载和配置第三方库
3. **配置生成**: 根据环境生成相应的配置文件
4. **测试集成**: 自动运行单元测试和集成测试
5. **部署打包**: 生成可执行文件和部署包

### 支持的命令
- `python build.py Configure` - 配置项目
- `python build.py Build` - 构建项目
- `python build.py Test` - 运行测试
- `python build.py Clean` - 清理构建文件
- `python build.py Package` - 打包部署

## 平台支持

### Windows
- 编译器: MSVC 2022, MinGW-w64
- 构建工具: 自研Python构建系统

### Linux
- 编译器: GCC 11+, Clang 13+
- 构建工具: 自研Python构建系统

### macOS
- 编译器: Clang (Xcode Command Line Tools)
- 构建工具: 自研Python构建系统