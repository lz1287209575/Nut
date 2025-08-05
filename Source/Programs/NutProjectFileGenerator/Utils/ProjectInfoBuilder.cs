using NutBuildSystem.BuildTargets;
using NutBuildSystem.Compilation;
using NutBuildSystem.Discovery;
using NutBuildSystem.IO;
using NutBuildSystem.Logging;
using NutBuildSystem.Models;
using NutProjectFileGenerator.Models;

namespace NutProjectFileGenerator.Utils
{
    /// <summary>
    /// 项目信息构建器
    /// </summary>
    public class ProjectInfoBuilder
    {
        /// <summary>
        /// 从nprx项目文件构建解决方案信息
        /// </summary>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>解决方案信息</returns>
        public static async Task<SolutionInfo> BuildSolutionInfoAsync(string projectRoot, ILogger? logger = null)
        {
            logger ??= new ConsoleLogger();
            
            // 首先尝试读取nprx项目文件
            var nutProject = NutProjectReader.FindAndReadProject(projectRoot, logger);
            
            if (nutProject != null)
            {
                logger.Info($"Using nprx project configuration: {nutProject.EngineAssociation}");
                return await BuildSolutionFromNutProject(nutProject, projectRoot, logger);
            }
            
            // 如果没有找到nprx文件，使用传统的Meta文件发现方式
            logger.Warning("No .nprx project file found, falling back to Meta file discovery");
            return await BuildSolutionFromMetaFiles(projectRoot, logger);
        }

        /// <summary>
        /// 从Nut项目文件构建解决方案信息
        /// </summary>
        /// <param name="nutProject">Nut项目配置</param>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>解决方案信息</returns>
        private static async Task<SolutionInfo> BuildSolutionFromNutProject(NutProject nutProject, string projectRoot, ILogger logger)
        {
            var solutionInfo = new SolutionInfo
            {
                Name = nutProject.EngineAssociation,
                SolutionRoot = projectRoot,
                ThirdPartyDirectory = Path.Combine(projectRoot, "ThirdParty"),
                IntermediateDirectory = Path.Combine(projectRoot, nutProject.Paths.Intermediate),
                BinaryDirectory = Path.Combine(projectRoot, nutProject.Paths.Build)
            };

            // 设置全局包含目录（从第三方库插件推导）
            foreach (var plugin in nutProject.Plugins.Where(p => p.Enabled))
            {
                string includeDir = Path.Combine(projectRoot, plugin.Path, "include");
                if (Directory.Exists(includeDir))
                {
                    solutionInfo.GlobalIncludeDirectories.Add(includeDir);
                }
            }

            // 添加标准包含目录
            solutionInfo.GlobalIncludeDirectories.AddRange(new[]
            {
                Path.Combine(projectRoot, "Source"),
                Path.Combine(projectRoot, "Source/Runtime"),
                Path.Combine(projectRoot, nutProject.Paths.Intermediate, "Generated")
            });

            // 设置全局预处理器定义
            solutionInfo.GlobalPreprocessorDefinitions.AddRange(new[]
            {
                "NUT_ENGINE=1",
                "SPDLOG_COMPILED_LIB=1"
            });

            // 使用新的模块发现逻辑
            try
            {
                var modules = await ModuleDiscovery.DiscoverModulesAsync(projectRoot, logger);
                
                foreach (var module in modules)
                {
                    var projectInfo = await ConvertModuleToProjectInfo(module, nutProject, projectRoot, logger);
                    if (projectInfo != null)
                    {
                        solutionInfo.Projects.Add(projectInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                logger.Error($"发现模块时出错: {ex.Message}");
            }

            // 发现 C# 项目
            try
            {
                var csharpProjects = await DiscoverCSharpProjectsAsync(projectRoot, logger);
                solutionInfo.Projects.AddRange(csharpProjects);
            }
            catch (Exception ex)
            {
                logger.Error($"发现C#项目时出错: {ex.Message}");
            }

            // 如果没有发现任何项目，创建默认项目
            if (solutionInfo.Projects.Count == 0)
            {
                solutionInfo.Projects.AddRange(CreateDefaultProjects(projectRoot));
            }

            logger.Info($"Found {solutionInfo.Projects.Count} projects in solution");
            
            return solutionInfo;
        }

        /// <summary>
        /// 从Meta文件构建解决方案信息（回退方法）
        /// </summary>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>解决方案信息</returns>
        private static async Task<SolutionInfo> BuildSolutionFromMetaFiles(string projectRoot, ILogger logger)
        {
            var solutionInfo = new SolutionInfo
            {
                Name = "NutEngine",
                SolutionRoot = projectRoot,
                ThirdPartyDirectory = Path.Combine(projectRoot, "ThirdParty"),
                IntermediateDirectory = Path.Combine(projectRoot, "Intermediate"),
                BinaryDirectory = Path.Combine(projectRoot, "Binary")
            };

            // 设置全局包含目录
            solutionInfo.GlobalIncludeDirectories.AddRange(new[]
            {
                Path.Combine(projectRoot, "Source"),
                Path.Combine(projectRoot, "Source", "Runtime"),
                Path.Combine(projectRoot, "ThirdParty", "spdlog", "include"),
                Path.Combine(projectRoot, "ThirdParty", "tcmalloc", "src"),
                Path.Combine(projectRoot, "Intermediate", "Generated")
            });

            // 设置全局预处理器定义
            solutionInfo.GlobalPreprocessorDefinitions.AddRange(new[]
            {
                "NUT_ENGINE=1",
                "NUT_PLATFORM_MAC=1",
                "SPDLOG_COMPILED_LIB=1"
            });

            // 发现所有构建目标
            try
            {
                var buildTargets = await BuildTargetDiscovery.DiscoverBuildTargetsAsync(projectRoot);
                
                foreach (var buildTarget in buildTargets)
                {
                    var projectInfo = await ConvertBuildTargetToProjectInfo(buildTarget, projectRoot);
                    if (projectInfo != null)
                    {
                        solutionInfo.Projects.Add(projectInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                logger.Error($"发现构建目标时出错: {ex.Message}");
            }

            // 如果没有发现任何项目，创建默认项目
            if (solutionInfo.Projects.Count == 0)
            {
                solutionInfo.Projects.AddRange(CreateDefaultProjects(projectRoot));
            }

            return solutionInfo;
        }

        /// <summary>
        /// 将模块信息转换为项目信息
        /// </summary>
        /// <param name="module">模块信息</param>
        /// <param name="nutProject">Nut项目配置</param>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>项目信息</returns>
        private static async Task<ProjectInfo?> ConvertModuleToProjectInfo(ModuleInfo module, NutProject nutProject, string projectRoot, ILogger logger)
        {
            try
            {
                var projectInfo = new ProjectInfo
                {
                    Name = module.Name,
                    ProjectRoot = projectRoot,
                    OutputName = module.Name
                };

                // 根据模块类型设置项目类型
                projectInfo.Type = module.Type.ToLower() switch
                {
                    "corelib" => ProjectType.StaticLibrary,
                    "executable" => ProjectType.Executable,
                    _ => ProjectType.StaticLibrary
                };

                // 设置输出目录
                projectInfo.OutputDirectory = Path.Combine(projectRoot, nutProject.Paths.Build, module.Name);

                // 添加源文件目录
                if (Directory.Exists(module.SourcesPath))
                {
                    await CollectSourceFiles(projectInfo, module.SourcesPath);
                    projectInfo.IncludeDirectories.Add(module.SourcesPath);
                }

                // 添加Meta和Config文件
                await CollectProjectMetaFiles(projectInfo, module.ModulePath);

                // 如果有构建目标信息，应用配置
                if (module.BuildTarget != null)
                {
                    ApplyBuildTargetConfiguration(projectInfo, module.BuildTarget, module.ModulePath);
                }

                // 添加模块特定的预处理器定义
                projectInfo.PreprocessorDefinitions.Add($"NUT_{module.Name.ToUpper()}=1");

                // 添加第三方库包含目录
                ApplyPluginIncludes(projectInfo, nutProject, projectRoot);

                // 设置标准编译器标志
                projectInfo.CompilerFlags.AddRange(new[]
                {
                    "-std=c++20",
                    "-Wall",
                    "-Wextra"
                });

                logger.Debug($"Converted module: {module.Name} (Type: {module.Type})");
                return projectInfo;
            }
            catch (Exception ex)
            {
                logger.Error($"转换模块 {module.Name} 时出错: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 应用构建目标配置
        /// </summary>
        /// <param name="projectInfo">项目信息</param>
        /// <param name="buildTarget">构建目标</param>
        /// <param name="modulePath">模块路径</param>
        private static void ApplyBuildTargetConfiguration(ProjectInfo projectInfo, INutBuildTarget buildTarget, string modulePath)
        {
            // 应用源文件
            if (buildTarget.Sources != null)
            {
                foreach (var source in buildTarget.Sources)
                {
                    string fullPath = Path.IsPathRooted(source) ? source : Path.Combine(modulePath, source);
                    if (File.Exists(fullPath) && !projectInfo.SourceFiles.Contains(fullPath))
                    {
                        projectInfo.SourceFiles.Add(fullPath);
                    }
                }
            }

            // 应用包含目录
            if (buildTarget.IncludeDirs != null)
            {
                foreach (var includeDir in buildTarget.IncludeDirs)
                {
                    string fullPath = Path.IsPathRooted(includeDir) ? includeDir : Path.Combine(modulePath, includeDir);
                    if (Directory.Exists(fullPath) && !projectInfo.IncludeDirectories.Contains(fullPath))
                    {
                        projectInfo.IncludeDirectories.Add(fullPath);
                    }
                }
            }

            // 注意：INutBuildTarget 接口暂时不包含 PreprocessorDefinitions 属性
            // 如果需要预处理器定义，可以在具体的构建目标实现中添加

            // 应用依赖关系
            if (buildTarget.Dependencies != null)
            {
                foreach (var dependency in buildTarget.Dependencies)
                {
                    if (!projectInfo.Dependencies.Contains(dependency))
                    {
                        projectInfo.Dependencies.Add(dependency);
                    }
                }
            }
        }

        /// <summary>
        /// 应用插件包含目录
        /// </summary>
        /// <param name="projectInfo">项目信息</param>
        /// <param name="nutProject">Nut项目配置</param>
        /// <param name="projectRoot">项目根目录</param>
        private static void ApplyPluginIncludes(ProjectInfo projectInfo, NutProject nutProject, string projectRoot)
        {
            foreach (var plugin in nutProject.Plugins.Where(p => p.Enabled))
            {
                // 添加插件的包含目录
                var possibleIncludeDirs = new[]
                {
                    Path.Combine(projectRoot, plugin.Path, "include"),
                    Path.Combine(projectRoot, plugin.Path, "src"),
                    Path.Combine(projectRoot, plugin.Path)
                };

                foreach (var includeDir in possibleIncludeDirs)
                {
                    if (Directory.Exists(includeDir) && !projectInfo.IncludeDirectories.Contains(includeDir))
                    {
                        projectInfo.IncludeDirectories.Add(includeDir);
                        break; // 只添加第一个存在的目录
                    }
                }
            }
        }

        /// <summary>
        /// 从Meta文件加载配置
        /// </summary>
        /// <param name="projectInfo">项目信息</param>
        /// <param name="metaFilePath">Meta文件路径</param>
        /// <param name="logger">日志记录器</param>
        private static async Task LoadMetaFileConfiguration(ProjectInfo projectInfo, string metaFilePath, ILogger logger)
        {
            try
            {
                if (!File.Exists(metaFilePath))
                {
                    logger.Warning($"Meta file not found: {metaFilePath}");
                    return;
                }

                // 使用现有的构建目标编译器读取Meta文件
                var buildTarget = await BuildTargetCompiler.CompileAndCreateTargetAsync(metaFilePath);
                if (buildTarget != null)
                {
                    // 应用Meta文件中的配置
                    if (buildTarget.Sources != null)
                    {
                        projectInfo.SourceFiles.AddRange(buildTarget.Sources);
                    }

                    if (buildTarget.IncludeDirs != null)
                    {
                        projectInfo.IncludeDirectories.AddRange(buildTarget.IncludeDirs);
                    }

                    // 注意：INutBuildTarget 接口暂时不包含 PreprocessorDefinitions 属性
                    // 如果需要预处理器定义，可以在具体的构建目标实现中添加

                    if (buildTarget.Dependencies != null)
                    {
                        projectInfo.Dependencies.AddRange(buildTarget.Dependencies);
                    }

                    logger.Debug($"Loaded Meta file configuration: {metaFilePath}");
                }
            }
            catch (Exception ex)
            {
                logger.Warning($"Failed to load Meta file {metaFilePath}: {ex.Message}");
            }
        }

        /// <summary>
        /// 将构建目标转换为项目信息
        /// </summary>
        /// <param name="buildTargetInfo">构建目标信息</param>
        /// <param name="projectRoot">项目根目录</param>
        /// <returns>项目信息</returns>
        private static async Task<ProjectInfo?> ConvertBuildTargetToProjectInfo(BuildTargetInfo buildTargetInfo, string projectRoot)
        {
            try
            {
                var projectInfo = new ProjectInfo
                {
                    Name = buildTargetInfo.Name,
                    ProjectRoot = projectRoot,
                    OutputName = buildTargetInfo.OutputName,
                    BuildTarget = null // BuildTargetInfo 不包含 Target 对象
                };

                // 确定项目类型
                projectInfo.Type = buildTargetInfo.TargetType.ToLower() switch
                {
                    "executable" => ProjectType.Executable,
                    "staticlibrary" => ProjectType.StaticLibrary,
                    "dynamiclibrary" => ProjectType.DynamicLibrary,
                    _ => ProjectType.Executable
                };

                // 设置输出目录
                projectInfo.OutputDirectory = Path.Combine(projectRoot, "Binary", projectInfo.Name);

                // 收集源文件和头文件
                if (Directory.Exists(buildTargetInfo.SourcesDirectory))
                {
                    await CollectSourceFiles(projectInfo, buildTargetInfo.SourcesDirectory);
                }

                // 添加Meta和Config文件
                await CollectProjectMetaFiles(projectInfo, buildTargetInfo.BaseDirectory);

                // 设置包含目录
                projectInfo.IncludeDirectories.Add(buildTargetInfo.SourcesDirectory);
                
                // 添加生成的头文件目录
                var generatedDir = Path.Combine(projectRoot, "Intermediate", "Generated");
                var relativeSourcePath = Path.GetRelativePath(projectRoot, buildTargetInfo.SourcesDirectory);
                var generatedSourceDir = Path.Combine(generatedDir, relativeSourcePath);
                if (Directory.Exists(generatedSourceDir))
                {
                    projectInfo.IncludeDirectories.Add(generatedSourceDir);
                }

                // 设置预处理器定义
                projectInfo.PreprocessorDefinitions.AddRange(new[]
                {
                    $"NUT_{projectInfo.Name.ToUpper()}=1"
                });

                // 设置编译器标志
                projectInfo.CompilerFlags.AddRange(new[]
                {
                    "-std=c++20",
                    "-Wall",
                    "-Wextra"
                });

                return projectInfo;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"警告: 转换构建目标 {buildTargetInfo.Name} 时出错: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 收集源文件和头文件
        /// </summary>
        /// <param name="projectInfo">项目信息</param>
        /// <param name="sourcesDirectory">源文件目录</param>
        private static async Task CollectSourceFiles(ProjectInfo projectInfo, string sourcesDirectory)
        {
            if (!Directory.Exists(sourcesDirectory))
                return;

            // 收集源文件
            var sourceExtensions = new[] { ".cpp", ".cc", ".cxx", ".c" };
            var headerExtensions = new[] { ".h", ".hpp", ".hxx" };

            var allFiles = Directory.GetFiles(sourcesDirectory, "*", SearchOption.AllDirectories);

            foreach (var file in allFiles)
            {
                var extension = Path.GetExtension(file).ToLower();
                
                if (sourceExtensions.Contains(extension))
                {
                    projectInfo.SourceFiles.Add(file);
                }
                else if (headerExtensions.Contains(extension) || file.EndsWith(".generate.h"))
                {
                    projectInfo.HeaderFiles.Add(file);
                }
            }

            // 同时收集生成的头文件
            var projectRoot = projectInfo.ProjectRoot;
            var generatedDir = Path.Combine(projectRoot, "Intermediate", "Generated");
            var relativeSourcePath = Path.GetRelativePath(projectRoot, sourcesDirectory);
            var generatedSourceDir = Path.Combine(generatedDir, relativeSourcePath);

            if (Directory.Exists(generatedSourceDir))
            {
                var generatedFiles = Directory.GetFiles(generatedSourceDir, "*.generate.h", SearchOption.AllDirectories);
                projectInfo.HeaderFiles.AddRange(generatedFiles);
            }
        }

        /// <summary>
        /// 收集项目的Meta和Config文件
        /// </summary>
        /// <param name="projectInfo">项目信息</param>
        /// <param name="projectPath">项目路径</param>
        private static async Task CollectProjectMetaFiles(ProjectInfo projectInfo, string projectPath)
        {
            // 收集Meta目录下的文件
            var metaDirectory = Path.Combine(projectPath, "Meta");
            if (Directory.Exists(metaDirectory))
            {
                var metaFiles = Directory.GetFiles(metaDirectory, "*", SearchOption.AllDirectories);
                foreach (var file in metaFiles)
                {
                    // 将Meta文件添加到HeaderFiles中，这样它们会显示在项目中
                    if (!projectInfo.HeaderFiles.Contains(file))
                    {
                        projectInfo.HeaderFiles.Add(file);
                    }
                }
            }

            // 收集Config目录下的文件
            var configDirectory = Path.Combine(projectPath, "Config");
            if (Directory.Exists(configDirectory))
            {
                var configFiles = Directory.GetFiles(configDirectory, "*", SearchOption.AllDirectories);
                foreach (var file in configFiles)
                {
                    // 将Config文件添加到HeaderFiles中，这样它们会显示在项目中
                    if (!projectInfo.HeaderFiles.Contains(file))
                    {
                        projectInfo.HeaderFiles.Add(file);
                    }
                }
            }

            // 收集Configs目录下的文件（兼容性）
            var configsDirectory = Path.Combine(projectPath, "Configs");
            if (Directory.Exists(configsDirectory))
            {
                var configsFiles = Directory.GetFiles(configsDirectory, "*", SearchOption.AllDirectories);
                foreach (var file in configsFiles)
                {
                    // 将Configs文件添加到HeaderFiles中，这样它们会显示在项目中
                    if (!projectInfo.HeaderFiles.Contains(file))
                    {
                        projectInfo.HeaderFiles.Add(file);
                    }
                }
            }

            // 收集Protos目录下的文件
            var protosDirectory = Path.Combine(projectPath, "Protos");
            if (Directory.Exists(protosDirectory))
            {
                var protoFiles = Directory.GetFiles(protosDirectory, "*", SearchOption.AllDirectories);
                foreach (var file in protoFiles)
                {
                    // 将Proto文件添加到HeaderFiles中，这样它们会显示在项目中
                    if (!projectInfo.HeaderFiles.Contains(file))
                    {
                        projectInfo.HeaderFiles.Add(file);
                    }
                }
            }
        }

        /// <summary>
        /// 发现C#项目
        /// </summary>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>C#项目列表</returns>
        private static async Task<List<ProjectInfo>> DiscoverCSharpProjectsAsync(string projectRoot, ILogger logger)
        {
            var csharpProjects = new List<ProjectInfo>();
            
            // 搜索所有.csproj文件
            var csprojFiles = Directory.GetFiles(projectRoot, "*.csproj", SearchOption.AllDirectories);
            
            foreach (var csprojFile in csprojFiles)
            {
                try
                {
                    var projectInfo = await ParseCSharpProjectAsync(csprojFile, projectRoot, logger);
                    if (projectInfo != null)
                    {
                        csharpProjects.Add(projectInfo);
                        logger.Debug($"发现C#项目: {projectInfo.Name}");
                    }
                }
                catch (Exception ex)
                {
                    logger.Warning($"解析C#项目文件失败 {csprojFile}: {ex.Message}");
                }
            }
            
            logger.Info($"发现 {csharpProjects.Count} 个C#项目");
            return csharpProjects;
        }

        /// <summary>
        /// 解析C#项目文件
        /// </summary>
        /// <param name="csprojPath">csproj文件路径</param>
        /// <param name="projectRoot">项目根目录</param>
        /// <param name="logger">日志记录器</param>
        /// <returns>项目信息</returns>
        private static async Task<ProjectInfo?> ParseCSharpProjectAsync(string csprojPath, string projectRoot, ILogger logger)
        {
            try
            {
                var content = await File.ReadAllTextAsync(csprojPath);
                var projectDir = Path.GetDirectoryName(csprojPath) ?? "";
                var projectName = Path.GetFileNameWithoutExtension(csprojPath);
                
                var projectInfo = new ProjectInfo
                {
                    Name = projectName,
                    ProjectRoot = projectRoot,
                    OutputName = projectName,
                    OutputDirectory = Path.Combine(projectRoot, "Binary", projectName)
                };

                // 确定项目类型（通过OutputType）
                if (content.Contains("<OutputType>Exe</OutputType>"))
                {
                    projectInfo.Type = ProjectType.CSharpExecutable;
                }
                else
                {
                    projectInfo.Type = ProjectType.CSharpLibrary;
                }

                // 收集源文件
                var sourceFiles = Directory.GetFiles(projectDir, "*.cs", SearchOption.AllDirectories)
                    .Where(f => !f.Contains("/bin/") && !f.Contains("/obj/") && !f.Contains("\\bin\\") && !f.Contains("\\obj\\"))
                    .ToList();
                
                projectInfo.SourceFiles.AddRange(sourceFiles);

                logger.Debug($"C#项目 {projectName}: {projectInfo.Type}, {sourceFiles.Count} 个源文件");
                return projectInfo;
            }
            catch (Exception ex)
            {
                logger.Warning($"解析C#项目文件失败 {csprojPath}: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 创建默认项目列表
        /// </summary>
        /// <param name="projectRoot">项目根目录</param>
        /// <returns>默认项目列表</returns>
        private static List<ProjectInfo> CreateDefaultProjects(string projectRoot)
        {
            var projects = new List<ProjectInfo>();

            // LibNut 静态库
            var libNutSources = Path.Combine(projectRoot, "Source", "Runtime", "LibNut", "Sources");
            if (Directory.Exists(libNutSources))
            {
                var libNut = new ProjectInfo
                {
                    Name = "LibNut",
                    Type = ProjectType.StaticLibrary,
                    ProjectRoot = projectRoot,
                    OutputName = "LibNut",
                    OutputDirectory = Path.Combine(projectRoot, "Binary", "LibNut")
                };

                libNut.IncludeDirectories.Add(libNutSources);
                libNut.PreprocessorDefinitions.Add("NUT_LIBNUT=1");
                libNut.CompilerFlags.AddRange(new[] { "-std=c++20", "-Wall", "-Wextra" });

                projects.Add(libNut);
            }

            // 查找微服务
            var microServicesDir = Path.Combine(projectRoot, "Source", "Runtime", "MicroServices");
            if (Directory.Exists(microServicesDir))
            {
                var serviceDirs = Directory.GetDirectories(microServicesDir);
                foreach (var serviceDir in serviceDirs)
                {
                    var serviceName = Path.GetFileName(serviceDir);
                    var sourcesDir = Path.Combine(serviceDir, "Sources");
                    
                    if (Directory.Exists(sourcesDir))
                    {
                        var service = new ProjectInfo
                        {
                            Name = serviceName,
                            Type = ProjectType.Executable,
                            ProjectRoot = projectRoot,
                            OutputName = serviceName,
                            OutputDirectory = Path.Combine(projectRoot, "Binary", serviceName)
                        };

                        service.IncludeDirectories.Add(sourcesDir);
                        service.PreprocessorDefinitions.Add($"NUT_{serviceName.ToUpper()}=1");
                        service.CompilerFlags.AddRange(new[] { "-std=c++20", "-Wall", "-Wextra" });
                        service.Dependencies.Add("LibNut");

                        projects.Add(service);
                    }
                }
            }

            return projects;
        }
    }
}