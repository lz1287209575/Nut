using System.CommandLine;
using NutBuildSystem.Logging;
using NutBuildSystem.CommandLine;
using NutBuildSystem.IO;
using NutProjectFileGenerator.Generators;
using NutProjectFileGenerator.Utils;

namespace NutProjectFileGenerator
{
    /// <summary>
    /// NutProjectFileGenerator - Nut引擎项目文件生成工具
    /// 通过读取Meta文件自动生成各种IDE的项目文件
    /// </summary>
    class NutProjectFileGeneratorApp : CommandLineApplication
    {
        private ILogger logger = LoggerFactory.Default;
        private readonly List<IProjectFileGenerator> generators = new();

        public NutProjectFileGeneratorApp() : base("NutProjectFileGenerator - Nut Engine Project File Generator")
        {
            InitializeGenerators();
        }

        static async Task<int> Main(string[] args)
        {
            var app = new NutProjectFileGeneratorApp();
            return await app.RunAsync(args);
        }

        protected override void ConfigureCommands()
        {
            builder.AddCommonGlobalOptions();

            // 添加项目文件生成器特定选项
            var generatorOption = new Option<string[]>("--generator", "-g")
            {
                Description = "要使用的生成器 (VSCode/Xcode/VisualStudio)",
                ArgumentHelpName = "generators",
                AllowMultipleArgumentsPerToken = true
            };

            var outputDirOption = new Option<string?>("--output", "-o")
            {
                Description = "输出目录（默认为项目根目录）",
                ArgumentHelpName = "directory"
            };

            var forceOption = new Option<bool>("--force", "-f")
            {
                Description = "强制覆盖现有项目文件"
            };

            var listGeneratorsOption = new Option<bool>("--list-generators", "-l")
            {
                Description = "列出所有可用的生成器"
            };

            builder.AddGlobalOption(generatorOption);
            builder.AddGlobalOption(outputDirOption);
            builder.AddGlobalOption(forceOption);
            builder.AddGlobalOption(listGeneratorsOption);

            // 设置默认处理程序
            builder.SetDefaultHandler(async (context) =>
            {
                logger = context.Logger;

                try
                {
                    // 从 context 中获取实际的参数值
                    var listGenerators = context.ParseResult.GetValueForOption(listGeneratorsOption);
                    var selectedGenerators = context.ParseResult.GetValueForOption(generatorOption) ?? Array.Empty<string>();
                    var outputDir = context.ParseResult.GetValueForOption(outputDirOption);
                    var force = context.ParseResult.GetValueForOption(forceOption);

                    if (listGenerators)
                    {
                        return await ListGeneratorsAsync();
                    }

                    return await GenerateProjectFilesAsync(selectedGenerators, outputDir, force);
                }
                catch (Exception ex)
                {
                    logger.Error($"项目文件生成失败: {ex.Message}", ex);
                    return 1;
                }
            });
        }

        /// <summary>
        /// 初始化生成器
        /// </summary>
        private void InitializeGenerators()
        {
            generators.Add(new VSCodeGenerator());
            generators.Add(new XcodeGenerator());
            generators.Add(new VisualStudioGenerator());
        }

        /// <summary>
        /// 列出所有可用的生成器
        /// </summary>
        private async Task<int> ListGeneratorsAsync()
        {
            logger.Info("可用的项目文件生成器:");
            logger.Info("========================");

            foreach (var generator in generators)
            {
                var status = generator.CanGenerate() ? "✅ 可用" : "❌ 不可用";
                var platforms = string.Join(", ", generator.SupportedPlatforms);
                
                logger.Info($"🔧 {generator.Name}");
                logger.Info($"   描述: {generator.Description}");
                logger.Info($"   平台: {platforms}");
                logger.Info($"   状态: {status}");
                logger.Info("");
            }

            return 0;
        }

        /// <summary>
        /// 生成项目文件
        /// </summary>
        private async Task<int> GenerateProjectFilesAsync(string[] selectedGenerators, string? outputDir, bool force)
        {
            logger.Info("NutProjectFileGenerator v1.0 - Nut Engine Project File Generator");
            logger.Info("==============================================================");

            // 查找项目根目录
            var projectRoot = FindProjectRoot();
            logger.Info($"📁 项目根目录: {projectRoot}");

            // 设置输出目录
            outputDir ??= Path.Combine(projectRoot, "ProjectFiles");

            if (!Directory.Exists(outputDir))
            {
                Directory.CreateDirectory(outputDir);
            }

            logger.Info($"📁 输出目录: {outputDir}");

            // 构建解决方案信息
            logger.Info("🔍 分析项目结构...");
            var solutionInfo = await ProjectInfoBuilder.BuildSolutionInfoAsync(projectRoot);
            
            logger.Info($"✅ 发现 {solutionInfo.Projects.Count} 个项目:");
            foreach (var project in solutionInfo.Projects)
            {
                logger.Info($"   • {project.Name} ({project.Type})");
            }

            // 过滤生成器
            var activeGenerators = new List<IProjectFileGenerator>();
            
            if (selectedGenerators.Length == 0)
            {
                // 如果没有指定生成器，使用所有可用的生成器
                activeGenerators.AddRange(generators.Where(g => g.CanGenerate()));
            }
            else
            {
                foreach (var generatorName in selectedGenerators)
                {
                    var generator = generators.FirstOrDefault(g => 
                        string.Equals(g.Name, generatorName, StringComparison.OrdinalIgnoreCase));
                    
                    if (generator == null)
                    {
                        logger.Warning($"未知的生成器: {generatorName}");
                        continue;
                    }
                    
                    if (!generator.CanGenerate())
                    {
                        logger.Warning($"生成器 {generator.Name} 在当前环境下不可用");
                        continue;
                    }
                    
                    activeGenerators.Add(generator);
                }
            }

            if (activeGenerators.Count == 0)
            {
                logger.Error("没有可用的生成器");
                return 1;
            }

            // 执行生成
            logger.Info($"🚀 使用 {activeGenerators.Count} 个生成器生成项目文件...");
            
            var successCount = 0;
            foreach (var generator in activeGenerators)
            {
                logger.Info($"🔧 运行 {generator.Name} 生成器...");
                
                try
                {
                    // 检查现有文件
                    if (!force)
                    {
                        var existingFiles = generator.GetGeneratedFiles(solutionInfo, outputDir)
                            .Where(File.Exists)
                            .ToList();
                        
                        if (existingFiles.Count > 0)
                        {
                            logger.Warning($"发现 {existingFiles.Count} 个现有文件，使用 --force 覆盖");
                            foreach (var file in existingFiles.Take(3))
                            {
                                logger.Warning($"   • {Path.GetRelativePath(outputDir, file)}");
                            }
                            if (existingFiles.Count > 3)
                            {
                                logger.Warning($"   ... 以及其他 {existingFiles.Count - 3} 个文件");
                            }
                            continue;
                        }
                    }

                    var success = await generator.GenerateAsync(solutionInfo, outputDir);
                    if (success)
                    {
                        logger.Info($"   ✅ {generator.Name} 生成成功");
                        successCount++;

                        // 显示生成的文件
                        var generatedFiles = generator.GetGeneratedFiles(solutionInfo, outputDir);
                        foreach (var file in generatedFiles.Take(3))
                        {
                            if (File.Exists(file))
                            {
                                logger.Debug($"      📄 {Path.GetRelativePath(outputDir, file)}");
                            }
                        }
                        if (generatedFiles.Count > 3)
                        {
                            logger.Debug($"      ... 以及其他 {generatedFiles.Count - 3} 个文件");
                        }
                    }
                    else
                    {
                        logger.Error($"   ❌ {generator.Name} 生成失败");
                    }
                }
                catch (Exception ex)
                {
                    logger.Error($"   ❌ {generator.Name} 生成器出错: {ex.Message}", ex);
                }
            }

            // 总结
            logger.Info("");
            logger.Info("==============================================================");
            if (successCount > 0)
            {
                logger.Info($"🎉 项目文件生成完成！成功: {successCount}/{activeGenerators.Count}");
                
                if (successCount < activeGenerators.Count)
                {
                    logger.Warning("部分生成器未能成功运行，请检查上述错误信息");
                }
            }
            else
            {
                logger.Error("❌ 所有生成器都未能成功运行");
                return 1;
            }

            return 0;
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
    }
}