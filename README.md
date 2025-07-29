#Nut 微服务工程化骨架

##架构简介 - 跨平台高性能游戏服务器微服务框架 - 每个服务独立进程，支持自定义协议与Protobuf -
    构建系统完全自研，工程化、自动化、可扩展

    ##目录结构 详见 ProjectStructure.md

    ##构建系统 -
    统一的 BuildEngine 工程化主干 - 每个服务通过 Meta / ServiceName.Build.py 描述构建规则 -
    支持自动发现、批量 / 单独构建、清理、打包、Docker /
        K8S生成等

        ##工具集

        ## #NutBuildTools(原 BuildSystem) 构建和工程化工具集，包含：
    - 服务生成器 - 项目文件生成器 - 构建引擎 -
    Docker / K8S生成器

        ## #NutHeaderTools 头文件处理工具集，包含：
    - 头文件扫描和分析 - 依赖关系分析 - 头文件验证 -
    依赖图生成

        ##服务生成器 快速生成新服务的基础框架代码：

```bash
#使用服务生成器创建新服务
        python Source
        / Programs / NutBuildTools / BuildScripts /
        ServiceGenerator.py ChatService-- port 50054

#或通过BuildEngine
        python Source
        / Programs / NutBuildTools / BuildScripts /
        BuildEngine.py
```

        生成的服务包含：
    - Sources /（C++ 源码） - Configs /（配置文件） - Protos /（Protobuf协议） - Meta /（构建、Docker、K8S元数据） -
    compile_commands
            .json（IDE支持）

        ##项目文件生成与IDE支持

        ## #Windows
```bash
#生成项目文件（包含IDE配置）
        GenerateProjectFiles
            .bat

#仅刷新IntelliSense
        RefreshIntelliSense
            .bat
```

        ## #*nix(Linux / macOS)
```bash
#生成项目文件（包含IDE配置）
            ./
        GenerateProjectFiles
            .sh

#仅刷新IntelliSense
            ./
        RefreshIntelliSense.sh
```

        ## #手动执行
```bash
#生成项目文件
        python Source
        / Programs / NutBuildTools / BuildScripts /
        ProjectFileGenerator.py

#刷新IntelliSense
        python Source
        / Programs / NutBuildTools / BuildScripts /
        IntelliSenseRefresher.py
```

        ##快速开始 1. 安装依赖
   ```bash pip install
    - r NutBuildTools / Requirements.txt pip install -
    r NutHeaderTools /
        Requirements
            .txt
   ``` 2. 生成项目文件
   ```bash
#* nix
            ./
        GenerateProjectFiles.sh
   ``` 3. 查看所有服务
   ```bash python Source / Programs / NutBuildTools / BuildScripts / BuildEngine.py
   ``` 4. 生成新服务
   ```bash python Source / Programs / NutBuildTools / BuildScripts / ServiceGenerator.py MyService
   ``` 5. 分析头文件（可选）
   ```bash python Source / Programs / NutHeaderTools / HeaderTools.py analyze-- project
    -
    root.
   ``` 6. 后续可扩展 build / clean / package / docker /
        k8s 等命令

        ##IDE支持
    - **根目录 `.clangd`**：全局代码补全和语法检查配置 -
    **每个服务的 `Meta / compile_commands.json`**：clangd编译命令数据库 -
    **VSCode配置 **：自动生成 `.vscode / c_cpp_properties.json` 和 `tasks.json`（仅Linux / macOS / g++ / clang++） -
    **全局 `compile_commands.json`**：项目级别的编译命令数据库

      支持功能：
    - 代码补全、语法检查、跳转 - 现代C++ 20特性 - 智能提示、错误检测 -
    任务集成（VSCode）

    ##贡献 详见 CONTRIBUTING.md