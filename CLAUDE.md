#CLAUDE.md

This file provides guidance to Claude Code(claude.ai / code) when working with code in this repository.

    ##项目概述

    Nut 是一个跨平台、高性能的游戏服务器微服务框架，特点包括：
    - 完全自研构建系统（不依赖CMake） - C++ 20 / 23 现代C++ 代码库 - 基于Python的服务元数据定义
    - 支持 Windows、Linux、macOS -
    全项目采用大驼峰命名规范（PascalCase）

    ##架构设计

    ## #目录结构
    - `Source / Runtime / LibNut /` - 核心库和工具类（C++） - `Source / Runtime / MicroServices /` -
    各个微服务（每个服务包含 Sources /、Configs /、Protos /、Meta / 子目录）
    - `Source / Runtime / ServiceAllocate /` - 服务分配和管理 - `Source / Programs / NutBuildTools /` - C #构建系统实现
    - `ThirdParty /` -
    第三方依赖库（spdlog、tcmalloc）

    ## #服务架构 每个微服务遵循以下结构：
    - `Sources /` - C++ 源代码 - `Configs /` - JSON 配置文件 - `Protos /` - Protocol Buffer 协议定义 - `Meta /` -
    构建元数据（ServiceName.Build.py、ServiceName.Docker.py、ServiceName.Kubernetes
            .py）

        ##构建命令

        ## #项目文件生成
```bash
#生成 IDE 项目文件（Xcode、Visual Studio）
            ./
        GenerateProjectFiles.sh #Unix 系统./
        GenerateProjectFiles
            .bat #Windows 系统

#仅刷新 IntelliSense
            ./
        RefreshIntelliSense.sh
```

        ## #构建系统 构建系统目前处于过渡状态：
```bash
#C #构建工具（当前实现）
        dotnet run-- project Source
        / Programs /
        NutBuildTools-- --target<target> --platform<platform> --configuration<config>

#环境配置（依赖安装、protoc 安装）
            ./
        Setup.sh #Unix 系统./
        Setup
            .bat #Windows 系统
```

                **注意 **：README.md 中提到的 Python 构建脚本（BuildEngine.py、ServiceGenerator.py）尚未实现。当前使用 C
        #NutBuildTools 提供基础构建功能。

        ## #服务元数据格式 服务在 `Meta
        / ServiceName.Build.py` 中定义构建规则：
```python ServiceMeta = {
    "name" : "ServiceName",
    "sources" : ["../Sources/ServiceMain.cpp"],
    "include_dirs" : ["../Sources"],
    "proto_files" : ["../Protos/service.proto"],
    "config_files" : ["../Configs/ServiceConfig.json"],
    "dependencies" : ["protobuf"],
    "output" : "ServiceExecutable"
}
```

                         ##开发流程

                         ## #前置要求
                         -.NET 8.0 + SDK（用于构建工具） - C++ 20 / 23 兼容编译器（GCC 11 +、Clang 13 +、MSVC 2022） -
                         Protocol Buffers 编译器（protoc）

                             ## #添加新服务 1. 在 `Source
                             / Runtime / MicroServices /` 下创建服务目录 2. 遵循标准目录结构（Sources /、Configs
                             /、Protos /、Meta /） 3. 在 `Meta / ServiceName.Build.py` 中定义服务元数据 4. 使用 `./
                             GenerateProjectFiles.sh` 重新生成项目文件

                             ## #代码规范
                         - **文件名 * *：大驼峰（NetworkManager.h、PlayerSession.cpp） -
                         **类名 * *：大驼峰（NetworkManager、PlayerSession） -
                         **接口 * *：以 'I' 开头（INetworkHandler） - **公共函数 * *：大驼峰（InitializeNetwork()） -
                         **私有函数 * *：小驼峰（initializeSocket()） -
                         **成员变量 * *：小驼峰（playerCount、networkSocket） -
                         **常量 * *：大写下划线（MAX_PLAYERS、DEFAULT_PORT）

                                  ##IDE 支持

                                  项目提供完善的 IDE 集成：
                         - 全局 `.clangd` 配置用于代码补全 - 每个服务的 `compile_commands.json` 用于 clangd
                         - 生成的 `.vscode / c_cpp_properties.json` 和 `tasks.json`（Unix 系统）
                         - Xcode 项目文件用于 macOS 开发 -
                         Visual Studio 解决方案用于 Windows 开发

                         ##依赖库

                         - **spdlog * *：高性能日志库 - **tcmalloc * *：Google 内存分配器（ThirdParty / tcmalloc） -
                         **Protocol Buffers * *：用于服务间通信协议

                                              ##重要说明

                         - 项目正从文档中描述的 Python 构建系统过渡到已实现的 C #构建工具
                         - 服务元数据用 Python 定义但由 C #构建系统处理
                         - 添加新服务或修改元数据后始终需要重新生成项目文件 - 使用 `./ Setup.sh` 安装缺失依赖并配置环境