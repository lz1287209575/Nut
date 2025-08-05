
using System;
using System.Collections.Generic;
using System.CommandLine;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using NutBuildTools.BuildSystem;
using NutBuildSystem.Logging;
using NutBuildSystem.CommandLine;
using NutBuildSystem.IO;
using NutBuildSystem.Discovery;
using NutBuildSystem.Models;

namespace NutBuildTools
{
    /// <summary>
    /// NutBuildTools - Nut引擎构建工具
    /// 基于Roslyn和现代化的构建系统，支持跨平台构建
    /// </summary>
    class NutBuildToolsApp : CommandLineApplication
    {
        private ILogger logger = LoggerFactory.Default;

        public NutBuildToolsApp() : base("NutBuildTools - Nut引擎构建工具")
        {
        }

        static async Task<int> Main(string[] args)
        {
            var app = new NutBuildToolsApp();
            return await app.RunAsync(args);
        }

        protected override void ConfigureCommands()
        {
            builder.AddCommonGlobalOptions();

            builder.AddGlobalOption(BuildOptions.Target);
            builder.AddGlobalOption(BuildOptions.Platform);
            builder.AddGlobalOption(BuildOptions.Configuration);
            builder.AddGlobalOption(BuildOptions.Jobs);
            builder.AddGlobalOption(BuildOptions.Clean);
            builder.AddGlobalOption(BuildOptions.Rebuild);

            // 构建命令
            builder.AddSubCommand("build", "构建项目", cmd =>
            {
                // 构建命令不需要额外的选项，使用全局选项
            });

            // 清理命令
            var cleanOptionsOption = new Option<string>("--clean-options", "-co")
            {
                Description = "清理选项 (Default/All/BuildFiles/OutputFiles/GeneratedFiles)"
            };
            cleanOptionsOption.SetDefaultValue("Default");

            builder.AddSubCommand("clean", "清理构建产物", cmd =>
            {
                cmd.AddOption(cleanOptionsOption);
            });

            // 构建命令处理程序
            builder.SetHandlerForSubCommand("build", ExecuteBuildAsync);

            // 清理命令处理程序
            builder.SetHandlerForSubCommand("clean", ExecuteCleanAsync);

            // 设置默认处理程序（显示帮助）
            builder.SetDefaultHandler(async (context) =>
            {
                logger = context.Logger;
                logger.Info("NutBuildTools v1.0 - Nut Engine Build System");
                logger.Info("使用 --help 查看可用命令");
                return 0;
            });
        }

        /// <summary>
        /// 执行构建命令
        /// </summary>
        private async Task<int> ExecuteBuildAsync(CommandContext context)
        {
            logger = context.Logger;

            try
            {
                logger.Info("NutBuildTools v1.0 - 开始构建");
                logger.Info("=====================================");

                // 获取命令行参数
                var targetName = context.ParseResult.GetValueForOption(BuildOptions.Target);
                var platform = context.ParseResult.GetValueForOption(BuildOptions.Platform) ?? GetDefaultPlatform();
                var configuration = context.ParseResult.GetValueForOption(BuildOptions.Configuration) ?? "Debug";
                var clean = context.ParseResult.GetValueForOption(BuildOptions.Clean);

                logger.Info($"构建配置:");
                logger.Info($"  目标: {targetName ?? "所有模块"}");
                logger.Info($"  平台: {platform}");
                logger.Info($"  配置: {configuration}");
                logger.Info($"  清理构建: {clean}");

                // 发现项目和模块
                var projectRoot = FindProjectRoot();
                var modules = await DiscoverModulesAsync(projectRoot);

                if (modules.Count == 0)
                {
                    logger.Warning("未发现任何可构建的模块");
                    return 1;
                }

                // 过滤要构建的模块
                var targetModules = FilterTargetModules(modules, targetName);
                if (targetModules.Count == 0)
                {
                    logger.Error($"未找到目标模块: {targetName}");
                    return 1;
                }

                logger.Info($"找到 {targetModules.Count} 个模块需要构建");

                // 构建每个模块
                int successCount = 0;
                foreach (var module in targetModules)
                {
                    logger.Info($"🔨 构建模块: {module.Name}");

                    var success = await BuildModuleAsync(module, platform, configuration, clean);
                    if (success)
                    {
                        successCount++;
                        logger.Info($"✅ 模块 {module.Name} 构建成功");
                    }
                    else
                    {
                        logger.Error($"❌ 模块 {module.Name} 构建失败");
                    }
                }

                logger.Info("=====================================");
                logger.Info($"构建完成: {successCount}/{targetModules.Count} 个模块构建成功");

                return successCount == targetModules.Count ? 0 : 1;
            }
            catch (Exception ex)
            {
                logger.Error($"构建失败: {ex.Message}", ex);
                return 1;
            }
        }

        /// <summary>
        /// 执行清理命令
        /// </summary>
        private async Task<int> ExecuteCleanAsync(CommandContext context)
        {
            logger = context.Logger;

            try
            {
                logger.Info("NutBuildTools v1.0 - 开始清理");
                logger.Info("=====================================");

                var projectRoot = FindProjectRoot();
                var cleanOptions = context.ParseResult.GetValueForOption(new Option<string>("--clear-option")) as string ?? "Default";

                logger.Info($"清理选项: {cleanOptions}");

                // 实现清理逻辑
                var success = await CleanProjectAsync(projectRoot, cleanOptions);

                logger.Info("=====================================");
                logger.Info(success ? "清理完成" : "清理失败");

                return success ? 0 : 1;
            }
            catch (Exception ex)
            {
                logger.Error($"清理失败: {ex.Message}", ex);
                return 1;
            }
        }

        private NutTarget CreateTargetFromMetadata(string targetName, string platform, string configuration)
        {
            logger.Info($"创建构建目标: {targetName}");

            try
            {
                var projectRoot = FindProjectRoot();
                var targetInstance = FindAndCreateTarget(projectRoot, targetName, platform, configuration);

                if (targetInstance == null)
                {
                    // 如果找不到元数据，创建默认目标
                    logger.Warning($"未找到目标 {targetName} 的元数据，使用默认配置");
                    targetInstance = new NutExecutableTarget()
                    {
                        Name = targetName,
                        Platform = platform,
                        Configuration = configuration,
                        Sources = new List<string> { $"Source/Runtime/{targetName}/Sources" },
                        IncludeDirs = new List<string> { $"Source/Runtime/{targetName}/Sources" },
                        Dependencies = new List<string>(),
                        OutputName = targetName
                    };
                }

                return targetInstance;
            }
            catch (Exception ex)
            {
                logger.Error($"创建构建目标失败: {ex.Message}", ex);
                throw;
            }
        }

        private NutTarget FindAndCreateTarget(string projectRoot, string targetName, string platform, string configuration)
        {
            // 搜索可能的元数据文件位置
            var possiblePaths = new[]
            {
                Path.Combine(projectRoot, "Source", "Runtime", targetName, "Meta", $"{targetName}.Build.cs"),
                Path.Combine(projectRoot, "Source", "Programs", targetName, "Meta", $"{targetName}.Build.cs"),
                Path.Combine(projectRoot, "Source", "Runtime", "LibNut", "Meta", "LibNut.Build.cs")
            };

            foreach (var metaPath in possiblePaths)
            {
                if (File.Exists(metaPath))
                {
                    logger.Info($"找到元数据文件: {metaPath}");
                    return ParseBuildMetadata(metaPath, targetName, platform, configuration, projectRoot);
                }
            }

            return null;
        }

        private NutTarget ParseBuildMetadata(string metaPath, string targetName, string platform, string configuration, string projectRoot)
        {
            try
            {
                // 读取元数据文件内容
                var content = File.ReadAllText(metaPath);
                logger.Debug($"解析元数据文件: {metaPath}");

                // 获取元数据文件所在目录
                var metaDir = Path.GetDirectoryName(metaPath);
                var projectDir = Path.GetDirectoryName(metaDir);

                // 创建基础目标（假设大多数是可执行文件）
                var target = new NutExecutableTarget()
                {
                    Name = targetName,
                    Platform = platform,
                    Configuration = configuration,
                    OutputName = targetName
                };

                // 设置默认的源文件目录
                var sourcesDir = Path.Combine(projectDir, "Sources");
                if (Directory.Exists(sourcesDir))
                {
                    target.Sources.Add(sourcesDir);
                    target.IncludeDirs.Add(sourcesDir);
                }

                // 添加通用包含目录
                target.IncludeDirs.Add(Path.Combine(projectRoot, "Source"));
                target.IncludeDirs.Add(Path.Combine(projectRoot, "Source", "Runtime", "LibNut", "Sources"));
                target.IncludeDirs.Add(Path.Combine(projectRoot, "Intermediate", "Generated", "Runtime", "LibNut", "Sources"));
                target.IncludeDirs.Add(Path.Combine(projectRoot, "Intermediate", "Generated", "Runtime", "LibNut", "Sources", "Containers"));
                target.IncludeDirs.Add(Path.Combine(projectRoot, "Intermediate", "Generated", "Runtime", "LibNut", "Sources", "Core"));

                // 解析依赖（简单的字符串匹配）
                if (content.Contains("spdlog"))
                {
                    target.Dependencies.Add("spdlog");
                }
                if (content.Contains("tcmalloc"))
                {
                    target.Dependencies.Add("tcmalloc");
                }
                if (content.Contains("protobuf"))
                {
                    target.Dependencies.Add("protobuf");
                }

                // 检查是否是静态库项目
                if (content.Contains("static_library") || targetName.Contains("Lib"))
                {
                    var staticTarget = new NutStaticLibraryTarget()
                    {
                        Name = target.Name,
                        Platform = target.Platform,
                        Configuration = target.Configuration,
                        Sources = target.Sources,
                        IncludeDirs = target.IncludeDirs,
                        Dependencies = target.Dependencies,
                        OutputName = target.OutputName
                    };
                    return staticTarget;
                }

                logger.Info($"成功解析构建目标: {target.Name} ({target.GetType().Name})");
                logger.Info($"  源目录: {string.Join(", ", target.Sources)}");
                logger.Info($"  包含目录: {string.Join(", ", target.IncludeDirs)}");
                logger.Info($"  依赖库: {string.Join(", ", target.Dependencies)}");

                return target;
            }
            catch (Exception ex)
            {
                logger.Warning($"解析元数据文件失败: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 查找项目根目录（通过nprx文件）
        /// </summary>
        private string FindProjectRoot()
        {
            var projectFile = NutProjectReader.FindProjectFile(Environment.CurrentDirectory);
            if (projectFile == null)
            {
                throw new InvalidOperationException("未找到nprx项目文件，请在项目根目录运行此工具");
            }

            var projectRoot = Path.GetDirectoryName(projectFile) ?? Environment.CurrentDirectory;
            logger.Debug($"找到项目根目录: {projectRoot}");
            return projectRoot;
        }

        /// <summary>
        /// 发现项目中的所有模块
        /// </summary>
        private async Task<List<ModuleInfo>> DiscoverModulesAsync(string projectRoot)
        {
            logger.Info("🔍 发现项目模块...");
            var modules = await ModuleDiscovery.DiscoverModulesAsync(projectRoot, logger);

            logger.Info($"发现 {modules.Count} 个模块:");
            foreach (var module in modules)
            {
                string metaStatus = !string.IsNullOrEmpty(module.MetaFilePath) ? "✓" : "✗";
                logger.Info($"  {metaStatus} {module.Name} ({module.Type})");
            }

            return modules;
        }

        /// <summary>
        /// 从 ModuleInfo 创建 NutTarget
        /// </summary>
        private NutTarget? CreateNutTargetFromModule(ModuleInfo module, string platform, string configuration)
        {
            try
            {
                NutTarget target;
                
                // 根据模块类型创建相应的 NutTarget
                switch (module.Type.ToLower())
                {
                    case "executable":
                    case "corelib":
                        target = new NutExecutableTarget();
                        break;
                    case "staticlibrary":
                        target = new NutStaticLibraryTarget();
                        break;
                    default:
                        logger.Warning($"未知的模块类型: {module.Type}，使用默认可执行目标");
                        target = new NutExecutableTarget();
                        break;
                }

                // 设置基本属性
                target.Name = module.Name;
                target.Platform = platform;
                target.Configuration = configuration;
                target.OutputName = module.Name;

                // 添加源文件目录
                if (Directory.Exists(module.SourcesPath))
                {
                    target.Sources.Add(module.SourcesPath);
                    target.IncludeDirs.Add(module.SourcesPath);
                }

                // 如果有构建目标信息，应用其配置
                if (module.BuildTarget != null)
                {
                    // 直接应用构建目标的配置
                    target.Sources.AddRange(module.BuildTarget.Sources ?? new List<string>());
                    target.IncludeDirs.AddRange(module.BuildTarget.IncludeDirs ?? new List<string>());
                    target.Dependencies.AddRange(module.BuildTarget.Dependencies ?? new List<string>());
                    
                    if (!string.IsNullOrEmpty(module.BuildTarget.OutputName))
                    {
                        target.OutputName = module.BuildTarget.OutputName;
                    }
                }

                // 添加第三方库包含目录
                var projectRoot = FindProjectRoot();
                var thirdPartyPath = Path.Combine(projectRoot, "ThirdParty");
                if (Directory.Exists(thirdPartyPath))
                {
                    var spdlogInclude = Path.Combine(thirdPartyPath, "spdlog", "include");
                    var tcmallocInclude = Path.Combine(thirdPartyPath, "tcmalloc", "include");
                    
                    if (Directory.Exists(spdlogInclude))
                        target.IncludeDirs.Add(spdlogInclude);
                    if (Directory.Exists(tcmallocInclude))
                        target.IncludeDirs.Add(tcmallocInclude);
                }

                return target;
            }
            catch (Exception ex)
            {
                logger.Error($"创建构建目标失败: {ex.Message}");
                return null;
            }
        }


        /// <summary>
        /// 过滤要构建的目标模块
        /// </summary>
        private List<ModuleInfo> FilterTargetModules(List<ModuleInfo> modules, string? targetName)
        {
            if (string.IsNullOrEmpty(targetName))
            {
                // 如果没有指定目标，构建所有可执行模块
                return modules.Where(m => m.Type == "Executable" || m.Type == "CoreLib").ToList();
            }

            // 查找指定名称的模块
            var targetModule = modules.FirstOrDefault(m =>
                string.Equals(m.Name, targetName, StringComparison.OrdinalIgnoreCase));

            return targetModule != null ? new List<ModuleInfo> { targetModule } : new List<ModuleInfo>();
        }

        /// <summary>
        /// 构建单个模块
        /// </summary>
        private async Task<bool> BuildModuleAsync(ModuleInfo module, string platform, string configuration, bool clean)
        {
            try
            {
                logger.Info($"  类型: {module.Type}");
                logger.Info($"  路径: {Path.GetRelativePath(FindProjectRoot(), module.ModulePath)}");

                if (!string.IsNullOrEmpty(module.MetaFilePath))
                {
                    logger.Info($"  Meta文件: {Path.GetRelativePath(FindProjectRoot(), module.MetaFilePath)}");
                }

                // 检查源代码目录是否存在
                if (!Directory.Exists(module.SourcesPath))
                {
                    logger.Warning($"  源代码目录不存在: {module.SourcesPath}");
                    return false;
                }

                // 从 ModuleInfo 创建 NutTarget 并执行实际构建
                var nutTarget = CreateNutTargetFromModule(module, platform, configuration);
                if (nutTarget == null)
                {
                    logger.Error($"无法为模块 {module.Name} 创建构建目标");
                    return false;
                }

                // 使用 NutBuilder 执行实际构建
                var builder = new NutBuilder();
                
                if (clean)
                {
                    await builder.CleanAsync(nutTarget);
                }
                
                await builder.BuildAsync(nutTarget);
                logger.Info($"✅ 模块 {module.Name} 构建完成");
                return true;
            }
            catch (Exception ex)
            {
                logger.Error($"构建模块 {module.Name} 时发生错误: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 清理项目
        /// </summary>
        private async Task<bool> CleanProjectAsync(string projectRoot, string cleanOptions)
        {
            try
            {
                var dirsToClean = new List<string>();

                switch (cleanOptions.ToLower())
                {
                    case "all":
                        dirsToClean.AddRange(new[]
                        {
                            Path.Combine(projectRoot, "Binary"),
                            Path.Combine(projectRoot, "Intermediate"),
                            Path.Combine(projectRoot, "ProjectFiles")
                        });
                        break;

                    case "buildfiles":
                        dirsToClean.Add(Path.Combine(projectRoot, "Binary"));
                        break;

                    case "outputfiles":
                        dirsToClean.Add(Path.Combine(projectRoot, "Binary"));
                        break;

                    case "generatedfiles":
                        dirsToClean.Add(Path.Combine(projectRoot, "Intermediate", "Generated"));
                        break;

                    case "default":
                    default:
                        dirsToClean.AddRange([
                            Path.Combine(projectRoot, "Binary"),
                            Path.Combine(projectRoot, "Intermediate")
                        ]);
                        break;
                }

                foreach (var dir in dirsToClean)
                {
                    if (Directory.Exists(dir))
                    {
                        logger.Info($"清理目录: {Path.GetRelativePath(projectRoot, dir)}");
                        Directory.Delete(dir, true);
                    }
                }

                await Task.CompletedTask;
                return true;
            }
            catch (Exception ex)
            {
                logger.Error($"清理项目时发生错误: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// 获取默认平台
        /// </summary>
        private string GetDefaultPlatform()
        {
            if (OperatingSystem.IsWindows())
                return "Windows";
            else if (OperatingSystem.IsLinux())
                return "Linux";
            else if (OperatingSystem.IsMacOS())
                return "MacOS";
            else
                return "Unknown";
        }
    }
}
