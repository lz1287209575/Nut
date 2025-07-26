using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    /// <summary>
    /// 依赖分析器 - 用于增量编译
    /// </summary>
    public class DependencyAnalyzer
    {
        private readonly Regex _dependencyRegex = new Regex(@"^(.+\.o):\s*(.+)$", RegexOptions.Multiline | RegexOptions.Compiled);

        /// <summary>
        /// 解析依赖文件 (.d 文件)
        /// </summary>
        public List<string> ParseDependencyFile(string dependencyFilePath)
        {
            var dependencies = new List<string>();
            
            if (!File.Exists(dependencyFilePath))
            {
                Logger.Debug($"依赖文件不存在: {dependencyFilePath}");
                return dependencies;
            }

            try
            {
                var content = File.ReadAllText(dependencyFilePath);
                var matches = _dependencyRegex.Matches(content);

                foreach (Match match in matches)
                {
                    if (match.Groups.Count >= 3)
                    {
                        var dependencyList = match.Groups[2].Value;
                        var depFiles = dependencyList.Split(new[] { ' ', '\t', '\\' }, StringSplitOptions.RemoveEmptyEntries)
                                                    .Where(s => !string.IsNullOrWhiteSpace(s) && s != "\n" && s != "\r\n")
                                                    .Select(s => s.Trim())
                                                    .Where(s => !string.IsNullOrEmpty(s))
                                                    .ToList();
                        
                        dependencies.AddRange(depFiles);
                    }
                }

                Logger.Debug($"从依赖文件解析出 {dependencies.Count} 个依赖项: {dependencyFilePath}");
            }
            catch (Exception ex)
            {
                Logger.Warning($"解析依赖文件失败: {dependencyFilePath}, 错误: {ex.Message}");
            }

            return dependencies.Distinct().ToList();
        }

        /// <summary>
        /// 检查源文件是否需要重新编译
        /// </summary>
        public bool IsRecompileNeeded(string sourceFile, string objectFile, string dependencyFile)
        {
            // 如果对象文件不存在，需要编译
            if (!File.Exists(objectFile))
            {
                Logger.Debug($"对象文件不存在，需要编译: {sourceFile}");
                return true;
            }

            var objectTime = File.GetLastWriteTime(objectFile);

            // 检查源文件是否更新
            if (File.Exists(sourceFile))
            {
                var sourceTime = File.GetLastWriteTime(sourceFile);
                if (sourceTime > objectTime)
                {
                    Logger.Debug($"源文件已更新，需要重新编译: {sourceFile}");
                    return true;
                }
            }

            // 检查依赖的头文件是否更新
            var dependencies = ParseDependencyFile(dependencyFile);
            foreach (var dep in dependencies)
            {
                if (File.Exists(dep))
                {
                    var depTime = File.GetLastWriteTime(dep);
                    if (depTime > objectTime)
                    {
                        Logger.Debug($"依赖文件已更新，需要重新编译: {dep} -> {sourceFile}");
                        return true;
                    }
                }
            }

            Logger.Debug($"无需重新编译: {sourceFile}");
            return false;
        }

        /// <summary>
        /// 获取所有需要编译的源文件
        /// </summary>
        public List<string> GetFilesToCompile(List<string> sourceFiles, BuildPaths buildPaths, string targetName, string objectExtension)
        {
            var filesToCompile = new List<string>();

            foreach (var sourceFile in sourceFiles)
            {
                var objectFile = buildPaths.GetObjectFilePath(sourceFile, targetName, objectExtension);
                var dependencyFile = buildPaths.GetDependencyFilePath(sourceFile, targetName);

                if (IsRecompileNeeded(sourceFile, objectFile, dependencyFile))
                {
                    filesToCompile.Add(sourceFile);
                }
            }

            Logger.Info($"需要编译的文件数量: {filesToCompile.Count}/{sourceFiles.Count}");
            
            if (filesToCompile.Count > 0)
            {
                Logger.Debug("需要编译的文件列表:");
                foreach (var file in filesToCompile)
                {
                    Logger.Debug($"  - {file}");
                }
            }

            return filesToCompile;
        }

        /// <summary>
        /// 生成依赖文件生成参数 (用于编译器)
        /// </summary>
        public string GetDependencyGenerationFlags(CompilerConfig compiler, string dependencyFile)
        {
            switch (compiler.Type)
            {
                case CompilerType.GCC:
                case CompilerType.Clang:
                    return $"-MMD -MP -MF \"{dependencyFile}\"";
                case CompilerType.MSVC:
                    // MSVC 在较旧版本中不直接支持依赖文件生成
                    // 我们暂时跳过依赖生成，依赖时间戳检查
                    return string.Empty;
                default:
                    return string.Empty;
            }
        }
    }
}