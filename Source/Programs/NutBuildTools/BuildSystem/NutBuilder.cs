using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    public class NutBuilder
    {
        private readonly DependencyAnalyzer _dependencyAnalyzer = new DependencyAnalyzer();

        public async ValueTask BuildAsync(NutTarget target)
        {
            Logger.Info($"开始构建: Target={target.Name}, Platform={target.Platform}, Configuration={target.Configuration}");

            try
            {
                // 1. 检测编译器
                var compiler = CompilerConfig.DetectCompiler(target.Platform);
                
                // 2. 设置构建路径
                var projectRoot = FindProjectRoot();
                var buildPaths = new BuildPaths(projectRoot, target.Platform, target.Configuration);
                buildPaths.EnsureDirectoriesExist();

                // 3. 收集源文件
                var sourceFiles = GetSourceFileList(target);
                if (!sourceFiles.Any())
                {
                    Logger.Warning("未找到源文件，跳过构建");
                    return;
                }

                // 4. 增量编译检查
                var filesToCompile = _dependencyAnalyzer.GetFilesToCompile(
                    sourceFiles.ToList(), 
                    buildPaths, 
                    target.Name, 
                    compiler.ObjectExtension);

                // 5. 编译源文件
                var objectFiles = new List<string>();
                foreach (var sourceFile in filesToCompile)
                {
                    var objectFile = await CompileSourceFileAsync(sourceFile, target, compiler, buildPaths);
                    if (!string.IsNullOrEmpty(objectFile))
                    {
                        objectFiles.Add(objectFile);
                    }
                }

                // 6. 收集所有对象文件（包括未修改的）
                var allObjectFiles = sourceFiles.Select(sf => 
                    buildPaths.GetObjectFilePath(sf, target.Name, compiler.ObjectExtension)).ToList();

                // 7. 链接
                if (target is NutStaticLibraryTarget)
                {
                    await CreateStaticLibraryAsync(allObjectFiles, target, compiler, buildPaths);
                }
                else
                {
                    await LinkExecutableAsync(allObjectFiles, target, compiler, buildPaths);
                }

                Logger.Info("构建完成!");
            }
            catch (Exception ex)
            {
                Logger.Error($"构建失败: {ex.Message}");
                throw;
            }
        }

        public async ValueTask CleanAsync(NutTarget target, CleanOptions options = CleanOptions.Default)
        {
            Logger.Info($"开始清理: Target={target.Name}, Platform={target.Platform}, Configuration={target.Configuration}");
            
            try
            {
                var projectRoot = FindProjectRoot();
                var buildPaths = new BuildPaths(projectRoot, target.Platform, target.Configuration);
                buildPaths.Clean(options);
                Logger.Info("清理完成!");
            }
            catch (Exception ex)
            {
                Logger.Error($"清理失败: {ex.Message}");
                throw;
            }

            await Task.CompletedTask;
        }

        protected IEnumerable<string> GetSourceFileList(NutTarget target)
        {
            Logger.Info($"收集源文件: Target={target.Name}");
            var sourceFileExtensions = new string[] { ".cpp", ".c", ".cxx", ".cc" };
            var sourceList = new List<string>();
            
            foreach (var source in target.Sources)
            {
                Logger.Debug($"处理源目录: {source}");
                
                if (Directory.Exists(source))
                {
                    foreach (var filePath in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
                    {
                        if (sourceFileExtensions.Contains(Path.GetExtension(filePath).ToLower()))
                        {
                            Logger.Debug($"发现源文件: {filePath}");
                            sourceList.Add(filePath);
                        }
                    }
                }
                else if (File.Exists(source) && sourceFileExtensions.Contains(Path.GetExtension(source).ToLower()))
                {
                    Logger.Debug($"发现源文件: {source}");
                    sourceList.Add(source);
                }
            }

            Logger.Info($"找到 {sourceList.Count} 个源文件");
            return sourceList;
        }

        private async Task<string> CompileSourceFileAsync(string sourceFile, NutTarget target, 
            CompilerConfig compiler, BuildPaths buildPaths)
        {
            Logger.Info($"编译: {Path.GetFileName(sourceFile)}");

            var objectFile = buildPaths.GetObjectFilePath(sourceFile, target.Name, compiler.ObjectExtension);
            var dependencyFile = buildPaths.GetDependencyFilePath(sourceFile, target.Name);

            // 确保对象文件目录存在
            Directory.CreateDirectory(Path.GetDirectoryName(objectFile)!);

            // 构建编译命令
            var args = BuildCompileCommand(sourceFile, objectFile, dependencyFile, target, compiler);

            // 执行编译
            var result = await CommandExecutor.CompileAsync(compiler, args, buildPaths.ProjectRoot);

            if (!result.Success)
            {
                Logger.Error($"编译失败: {sourceFile}");
                Logger.Error($"退出码: {result.ExitCode}");
                Logger.Error($"编译命令: {compiler.CompilerPath} {args}");
                
                if (!string.IsNullOrEmpty(result.StandardOutput))
                {
                    Logger.Error($"标准输出: {result.StandardOutput}");
                }
                
                if (!string.IsNullOrEmpty(result.StandardError))
                {
                    Logger.Error($"错误输出: {result.StandardError}");
                }
                
                throw new InvalidOperationException($"编译失败: {sourceFile} (退出码: {result.ExitCode})");
            }

            Logger.Debug($"编译成功: {sourceFile} -> {objectFile}");
            return objectFile;
        }

        private string BuildCompileCommand(string sourceFile, string objectFile, string dependencyFile, 
            NutTarget target, CompilerConfig compiler)
        {
            var args = new List<string>();

            // 添加默认编译标志
            args.AddRange(compiler.DefaultFlags);

            // 添加配置相关标志
            if (target.Configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase))
            {
                args.AddRange(compiler.DebugFlags);
            }
            else
            {
                args.AddRange(compiler.ReleaseFlags);
            }

            // 添加包含目录
            foreach (var includeDir in target.IncludeDirs)
            {
                if (compiler.Type == CompilerType.MSVC)
                {
                    args.Add($"/I\"{includeDir}\"");
                }
                else
                {
                    args.Add($"-I\"{includeDir}\"");
                }
            }

            // 添加 ThirdParty 包含目录
            var projectRoot = FindProjectRoot();
            var thirdPartyIncludes = new[]
            {
                Path.Combine(projectRoot, "ThirdParty", "spdlog", "include"),
                Path.Combine(projectRoot, "ThirdParty", "tcmalloc", "src"),
                Path.Combine(projectRoot, "Source"),
                Path.Combine(projectRoot, "Intermediate", "Generated")
            };

            foreach (var include in thirdPartyIncludes)
            {
                if (Directory.Exists(include))
                {
                    if (compiler.Type == CompilerType.MSVC)
                    {
                        args.Add($"/I\"{include}\"");
                    }
                    else
                    {
                        args.Add($"-I\"{include}\"");
                    }
                }
            }

            // 添加依赖生成标志 (用于增量编译)
            var depFlags = _dependencyAnalyzer.GetDependencyGenerationFlags(compiler, dependencyFile);
            if (!string.IsNullOrEmpty(depFlags))
            {
                args.Add(depFlags);
            }

            // 只编译不链接
            if (compiler.Type == CompilerType.MSVC)
            {
                args.Add("/c");
            }
            else
            {
                args.Add("-c");
            }

            // 输入文件
            args.Add($"\"{sourceFile}\"");

            // 输出文件
            if (compiler.Type == CompilerType.MSVC)
            {
                args.Add($"/Fo\"{objectFile}\"");
            }
            else
            {
                args.Add($"-o \"{objectFile}\"");
            }

            return string.Join(" ", args);
        }

        private async Task LinkExecutableAsync(List<string> objectFiles, NutTarget target, 
            CompilerConfig compiler, BuildPaths buildPaths)
        {
            Logger.Info($"链接可执行文件: {target.Name}");

            var outputFile = buildPaths.GetOutputFilePath(target.Name, compiler.ExecutableExtension);
            
            var args = new List<string>();

            if (compiler.Type == CompilerType.MSVC)
            {
                // MSVC 链接器参数顺序
                args.Add($"/OUT:\"{outputFile}\"");
                args.AddRange(objectFiles.Select(obj => $"\"{obj}\""));
                
                // 添加必要的库
                args.Add("kernel32.lib");
                args.Add("user32.lib");
                
                // 添加 Windows 系统库
                if (target.Configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase))
                {
                    args.Add("/DEBUG");
                }
            }
            else
            {
                // GCC/Clang 链接器参数
                args.AddRange(objectFiles.Select(obj => $"\"{obj}\""));
                args.Add($"-o \"{outputFile}\"");
            }

            // 链接库
            AddLibraryFlags(args, target, compiler, buildPaths.ProjectRoot);

            var result = await CommandExecutor.LinkAsync(compiler, string.Join(" ", args), buildPaths.ProjectRoot);

            if (!result.Success)
            {
                Logger.Error($"链接失败: {target.Name}");
                Logger.Error($"退出码: {result.ExitCode}");
                Logger.Error($"链接命令: {compiler.LinkerPath} {string.Join(" ", args)}");
                
                if (!string.IsNullOrEmpty(result.StandardOutput))
                {
                    Logger.Error($"标准输出: {result.StandardOutput}");
                }
                
                if (!string.IsNullOrEmpty(result.StandardError))
                {
                    Logger.Error($"错误输出: {result.StandardError}");
                }
                
                throw new InvalidOperationException($"链接失败: {target.Name} (退出码: {result.ExitCode})");
            }

            Logger.Info($"链接成功: {outputFile}");
        }

        private async Task CreateStaticLibraryAsync(List<string> objectFiles, NutTarget target, 
            CompilerConfig compiler, BuildPaths buildPaths)
        {
            Logger.Info($"创建静态库: {target.Name}");

            var outputFile = buildPaths.GetOutputFilePath(target.Name, compiler.StaticLibExtension);
            
            var args = new List<string>();

            if (compiler.Type == CompilerType.MSVC)
            {
                args.Add($"/OUT:\"{outputFile}\"");
                args.AddRange(objectFiles.Select(obj => $"\"{obj}\""));
            }
            else
            {
                args.Add("rcs");
                args.Add($"\"{outputFile}\"");
                args.AddRange(objectFiles.Select(obj => $"\"{obj}\""));
            }

            var result = await CommandExecutor.ArchiveAsync(compiler, string.Join(" ", args), buildPaths.ProjectRoot);

            if (!result.Success)
            {
                Logger.Error($"创建静态库失败: {target.Name}");
                Logger.Error($"错误信息: {result.StandardError}");
                throw new InvalidOperationException($"创建静态库失败: {target.Name}");
            }

            Logger.Info($"静态库创建成功: {outputFile}");
        }

        private void AddLibraryFlags(List<string> args, NutTarget target, CompilerConfig compiler, string projectRoot)
        {
            // 添加依赖库
            foreach (var dependency in target.Dependencies)
            {
                switch (dependency.ToLower())
                {
                    case "spdlog":
                        // spdlog 是 header-only 库，无需链接
                        break;
                    case "tcmalloc":
                        if (compiler.Type == CompilerType.MSVC)
                        {
                            // TODO: 添加 Windows tcmalloc 支持
                        }
                        else
                        {
                            args.Add("-ltcmalloc");
                        }
                        break;
                    case "protobuf":
                        if (compiler.Type == CompilerType.MSVC)
                        {
                            args.Add("libprotobuf.lib");
                        }
                        else
                        {
                            args.Add("-lprotobuf");
                        }
                        break;
                    default:
                        // 其他依赖库
                        if (compiler.Type == CompilerType.MSVC)
                        {
                            args.Add($"{dependency}.lib");
                        }
                        else
                        {
                            args.Add($"-l{dependency}");
                        }
                        break;
                }
            }

            // 添加系统库
            if (target.Platform.Equals("Mac", StringComparison.OrdinalIgnoreCase) || 
                target.Platform.Equals("macOS", StringComparison.OrdinalIgnoreCase))
            {
                args.Add("-framework CoreFoundation");
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
                    return directory.FullName;
                }
                directory = directory.Parent;
            }

            throw new InvalidOperationException("未找到项目根目录 (CLAUDE.md)");
        }
    }
}