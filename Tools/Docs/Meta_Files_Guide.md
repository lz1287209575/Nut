# Nut 项目 Meta 文件指南

## 概述

Nut 项目使用 C# Meta 文件系统来定义项目的构建配置、Docker 配置和 Kubernetes 部署配置。这些文件位于每个项目的 `Meta/` 目录下。

## 支持的 Meta 文件类型

### 1. 构建 Meta 文件 (*.Build.cs)

用于定义项目的基本构建配置，包括源文件、依赖、输出等。

#### 1.1 NutServiceTarget - 微服务项目

```csharp
using NutBuildTools.BuildSystem;
using System.Collections.Generic;

public class PlayerServiceTarget : NutServiceTarget
{
    public PlayerServiceTarget() : base()
    {
        Name = "PlayerService";
        ServiceType = "executable";
        OutputName = "PlayerServiceMain";
        
        Sources = new List<string>
        {
            "../Sources/PlayerServiceMain.cpp"
        };
        
        IncludeDirs = new List<string>
        {
            "../Sources"
        };
        
        ProtoFiles = new List<string>
        {
            "../Protos/playerservice.proto"
        };
        
        ConfigFiles = new List<string>
        {
            "../Configs/PlayerServiceConfig.json"
        };
        
        Dependencies = new List<string>
        {
            "protobuf", "grpc"
        };
    }
}
```

#### 1.2 NutStaticLibraryTarget - 静态库项目

```csharp
public class LibNutTarget : NutStaticLibraryTarget
{
    public LibNutTarget() : base()
    {
        Name = "LibNut";
        LibraryType = "static_library";
        OutputName = "LibNut";
        
        IncludeDirs = new List<string>
        {
            "../Sources"
        };
    }
}
```

#### 1.3 NutExecutableTarget - 可执行程序项目

```csharp
public class ServiceAllocateTarget : NutExecutableTarget
{
    public ServiceAllocateTarget() : base()
    {
        Name = "ServiceAllocate";
        ExecutableType = "executable";
        OutputName = "ServiceAllocate";
        
        Dependencies = new List<string>
        {
            "LibNut"
        };
    }
}
```

### 2. Docker Meta 文件 (*.Docker.cs)

用于定义 Docker 镜像构建配置。

```csharp
public class PlayerServiceDockerTarget : NutDockerTarget
{
    public PlayerServiceDockerTarget() : base()
    {
        Name = "PlayerService";
        BaseImage = "ubuntu:22.04";
        
        CopyFiles = new List<string>
        {
            "../Sources/PlayerServiceMain",
            "../Configs/PlayerServiceConfig.json"
        };
        
        ExposePorts = new List<int?>
        {
            50051
        };
        
        Entrypoint = new List<string>
        {
            "./PlayerServiceMain",
            "--config",
            "PlayerServiceConfig.json"
        };
    }
}
```

### 3. Kubernetes Meta 文件 (*.Kubernetes.cs)

用于定义 Kubernetes 部署配置。

```csharp
public class PlayerServiceKubernetesTarget : NutKubernetesTarget
{
    public PlayerServiceKubernetesTarget() : base()
    {
        Name = "PlayerService";
        DeploymentName = "playerservice-service";
        Replicas = 2;
        ContainerPort = 50051;
        
        Environment = new Dictionary<string, string>
        {
            { "CONFIG_PATH", "/app/PlayerServiceConfig.json" },
            { "GRPC_PORT", "50051" },
            { "LOG_LEVEL", "INFO" }
        };
    }
}
```

## 属性说明

### 基础属性 (所有目标类型)

- **Name**: 项目名称
- **Platform**: 目标平台 (默认: "Any")
- **Configuration**: 构建配置 (默认: "Debug")
- **Sources**: 源文件列表
- **IncludeDirs**: 头文件包含目录
- **Dependencies**: 依赖库列表
- **OutputName**: 输出文件名

### 服务专用属性 (NutServiceTarget)

- **ProtoFiles**: Protocol Buffer 文件列表
- **ConfigFiles**: 配置文件列表
- **ServiceType**: 服务类型 (executable, library等)

### Docker 专用属性 (NutDockerTarget)

- **BaseImage**: 基础镜像
- **CopyFiles**: 需要复制的文件列表
- **ExposePorts**: 暴露的端口列表
- **Entrypoint**: 容器启动命令

### Kubernetes 专用属性 (NutKubernetesTarget)

- **DeploymentName**: 部署名称
- **Replicas**: 副本数量
- **ContainerPort**: 容器端口
- **Environment**: 环境变量字典

## 迁移指南

### 从 Python Meta 文件迁移

原有的 Python Meta 文件（*.Build.py）已被 C# Meta 文件替代：

**Python (旧):**
```python
ServiceMeta = {
    "name": "PlayerService",
    "sources": ["../Sources/PlayerServiceMain.cpp"],
    "dependencies": ["protobuf"]
}
```

**C# (新):**
```csharp
public class PlayerServiceTarget : NutServiceTarget
{
    public PlayerServiceTarget() : base()
    {
        Name = "PlayerService";
        Sources = new List<string> { "../Sources/PlayerServiceMain.cpp" };
        Dependencies = new List<string> { "protobuf" };
    }
}
```

## 最佳实践

1. **命名规范**: Meta 文件类名应以项目名 + Target 结尾
2. **文档注释**: 为每个目标类添加 XML 文档注释
3. **路径使用**: 使用相对路径引用项目内的文件
4. **依赖管理**: 明确声明所有外部依赖
5. **配置分离**: 将构建、Docker、Kubernetes 配置分别定义在不同文件中

## 项目生成器支持

新的 C# Meta 文件系统完全集成到 ProjectGenerator 工具中：

```bash
# 发现项目（会自动识别新的 C# Meta 文件）
Tools/generate_projects.sh discover

# 生成 Visual Studio 项目
Tools/generate_projects.sh vs

# 生成 XCode 项目
Tools/generate_projects.sh xcode
```

ProjectGenerator 会自动：
- 解析 C# Meta 文件中的目标类型
- 识别源文件、配置文件、Proto 文件
- 生成相应的 IDE 项目文件
- 创建工作空间/解决方案文件