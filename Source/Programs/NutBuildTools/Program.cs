using System;
using System.Collections.Generic;
using System.CommandLine;
using System.IO;
using System.Threading.Tasks;
using NutBuildTools.BuildSystem;
using NutBuildSystem.Logging;
using NutBuildSystem.CommandLine;

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

            // 设置默认处理程序（显示帮助）
            builder.SetDefaultHandler(async (context) =>
            {
                logger = context.Logger;
                logger.Info("NutBuildTools v1.0 - Nut Engine Build System");
                logger.Info("使用 --help 查看可用命令");
                return 0;
            });
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

        private string FindProjectRoot()
        {
            var currentDir = Directory.GetCurrentDirectory();
            var directory = new DirectoryInfo(currentDir);

            while (directory != null)
            {
                var claudeMdPath = Path.Combine(directory.FullName, "CLAUDE.md");
                if (File.Exists(claudeMdPath))
                {
                    logger.Debug($"找到项目根目录: {directory.FullName}");
                    return directory.FullName;
                }
                directory = directory.Parent;
            }

            throw new InvalidOperationException("未找到项目根目录 (CLAUDE.md)");
        }
    }
}
