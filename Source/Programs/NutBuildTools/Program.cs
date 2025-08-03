using System;
using System.Collections.Generic;
using System.CommandLine;
using System.IO;
using System.Threading.Tasks;
using NutBuildTools.BuildSystem;
using NutBuildTools.Utils;

namespace NutBuildTools
{
    class Program
    {
        static async Task<int> Main(string[] args)
        {
            var targetOption = new Option<string>("--target", "-t")
            {
                Description = "编译目标（如 Editor/Server/Client）"
            };
            var platformOption = new Option<string>("--platform", "-p")
            {
                Description = "平台（如 Windows/Linux/Mac）"
            };
            var configOption = new Option<string>("--configuration", "-c")
            {
                DefaultValueFactory = _ => "Debug",
                Description = "配置（如Debug, Release）"
            };
            var logLevelOption = new Option<string>("--log-level", "-l")
            {
                DefaultValueFactory = _ => "Info",
                Description = "日志级别（Debug/Info/Warning/Error/Fatal）"
            };
            var logFileOption = new Option<string>("--log-file")
            {
                Description = "日志文件路径（可选）"
            };

            var command = new RootCommand("NutBuildTools 命令行编译工具")
            {
                targetOption,
                platformOption,
                configOption,
                logLevelOption,
                logFileOption
            };

            var buildCommand = new Command("build")
            {
                Description = "构建项目"
            };

            var cleanOptionsOption = new Option<string>("--clean-options", "-co")
            {
                DefaultValueFactory = _ => "Default",
                Description = "清理选项 (Default/All/BuildFiles/OutputFiles/GeneratedFiles)"
            };


            var cleanCommand = new Command("clean")
            {
                Description = "清理构建产物"
            };
            cleanCommand.Add(cleanOptionsOption);

            command.Add(buildCommand);
            command.Add(cleanCommand);

            // 配置日志的通用函数
            void ConfigureLogging(ParseResult result)
            {
                var logLevelStr = result.GetValue<string>("--log-level");
                var logFile = result.GetValue<string>("--log-file");

                if (!Enum.TryParse<LogLevel>(logLevelStr, true, out var logLevel))
                {
                    logLevel = LogLevel.Info;
                }

                Logger.Configure(
                    logFilePath: logFile ?? "logs/nutbuildtools.log",
                    enableFileLogging: !string.IsNullOrEmpty(logFile),
                    minLogLevel: logLevel
                );
            }

            // 获取构建参数的通用函数
            (string target, string platform, string configuration) GetBuildParameters(ParseResult result)
            {
                string target = result.GetRequiredValue<string>("--target");
                string platform = result.GetRequiredValue<string>("--platform");
                string configuration = result.GetRequiredValue<string>("--configuration");
                return (target, platform, configuration);
            }

            buildCommand.SetAction(async result =>
            {
                ConfigureLogging(result);
                Logger.Info("NutBuildTools 启动 - 构建模式");

                // 调试：显示环境信息
                Logger.Info($"环境检测: SRCROOT={Environment.GetEnvironmentVariable("SRCROOT")}");
                Logger.Info($"环境检测: CONFIGURATION={Environment.GetEnvironmentVariable("CONFIGURATION")}");
                Logger.Info($"环境检测: PLATFORM_NAME={Environment.GetEnvironmentVariable("PLATFORM_NAME")}");

                var (target, platform, configuration) = GetBuildParameters(result);
                Logger.Info($"参数: target={target}, platform={platform}, configuration={configuration}");

                var builder = new NutBuilder();
                var targetInstance = CreateTargetFromMetadata(target, platform, configuration);

                await builder.BuildAsync(targetInstance);
                Logger.Info("NutBuildTools 构建完成");
            });

            cleanCommand.SetAction(async result =>
            {
                ConfigureLogging(result);
                Logger.Info("NutBuildTools 启动 - 清理模式");

                var (target, platform, configuration) = GetBuildParameters(result);
                var cleanOptionsStr = result.GetValue<string>("--clean-options");
                Logger.Info($"参数: target={target}, platform={platform}, configuration={configuration}, clean-options={cleanOptionsStr}");

                // 解析清理选项
                CleanOptions cleanOptions = CleanOptions.Default;
                if (Enum.TryParse<CleanOptions>(cleanOptionsStr, true, out var parsedOptions))
                {
                    cleanOptions = parsedOptions;
                }
                else
                {
                    Logger.Warning($"无效的清理选项: {cleanOptionsStr}，使用默认选项");
                }

                var builder = new NutBuilder();
                var targetInstance = CreateTargetFromMetadata(target, platform, configuration);

                await builder.CleanAsync(targetInstance, cleanOptions);
                Logger.Info("NutBuildTools 清理完成");
            });

            return await command.Parse(args).InvokeAsync();
        }

        private static NutTarget CreateTargetFromMetadata(string targetName, string platform, string configuration)
        {
            Logger.Info($"创建构建目标: {targetName}");

            try
            {
                var projectRoot = FindProjectRoot();
                var targetInstance = FindAndCreateTarget(projectRoot, targetName, platform, configuration);

                if (targetInstance == null)
                {
                    // 如果找不到元数据，创建默认目标
                    Logger.Warning($"未找到目标 {targetName} 的元数据，使用默认配置");
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
                Logger.Error($"创建构建目标失败: {ex.Message}");
                throw;
            }
        }

        private static NutTarget FindAndCreateTarget(string projectRoot, string targetName, string platform, string configuration)
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
                    Logger.Info($"找到元数据文件: {metaPath}");
                    return ParseBuildMetadata(metaPath, targetName, platform, configuration, projectRoot);
                }
            }

            return null;
        }

        private static NutTarget ParseBuildMetadata(string metaPath, string targetName, string platform, string configuration, string projectRoot)
        {
            try
            {
                // 读取元数据文件内容
                var content = File.ReadAllText(metaPath);
                Logger.Debug($"解析元数据文件: {metaPath}");

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

                Logger.Info($"成功解析构建目标: {target.Name} ({target.GetType().Name})");
                Logger.Info($"  源目录: {string.Join(", ", target.Sources)}");
                Logger.Info($"  包含目录: {string.Join(", ", target.IncludeDirs)}");
                Logger.Info($"  依赖库: {string.Join(", ", target.Dependencies)}");

                return target;
            }
            catch (Exception ex)
            {
                Logger.Warning($"解析元数据文件失败: {ex.Message}");
                return null;
            }
        }

        private static string FindProjectRoot()
        {
            var currentDir = Directory.GetCurrentDirectory();
            var directory = new DirectoryInfo(currentDir);

            while (directory != null)
            {
                var claudeMdPath = Path.Combine(directory.FullName, "CLAUDE.md");
                if (File.Exists(claudeMdPath))
                {
                    Logger.Debug($"找到项目根目录: {directory.FullName}");
                    return directory.FullName;
                }
                directory = directory.Parent;
            }

            throw new InvalidOperationException("未找到项目根目录 (CLAUDE.md)");
        }
    }
}
