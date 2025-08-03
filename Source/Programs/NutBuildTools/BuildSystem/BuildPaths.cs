using System;
using System.IO;
using System.Linq;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    /// <summary>
    /// 清理选项
    /// </summary>
    [Flags]
    public enum CleanOptions
    {
        /// <summary>
        /// 仅清理构建中间文件
        /// </summary>
        BuildFiles = 1 << 0,
        
        /// <summary>
        /// 清理输出文件
        /// </summary>
        OutputFiles = 1 << 1,
        
        /// <summary>
        /// 清理生成的文件
        /// </summary>
        GeneratedFiles = 1 << 2,
        
        /// <summary>
        /// 默认清理选项（构建文件 + 输出文件）
        /// </summary>
        Default = BuildFiles | OutputFiles,
        
        /// <summary>
        /// 完全清理（所有文件）
        /// </summary>
        All = BuildFiles | OutputFiles | GeneratedFiles
    }

    /// <summary>
    /// 构建路径管理
    /// </summary>
    public class BuildPaths
    {
        public string ProjectRoot { get; private set; }
        public string BuildRoot { get; private set; }
        public string PlatformDir { get; private set; }
        public string ConfigurationDir { get; private set; }
        public string ObjectDir { get; private set; }
        public string OutputDir { get; private set; }
        public string IntermediateDir { get; private set; }

        public BuildPaths(string projectRoot, string platform, string configuration)
        {
            ProjectRoot = projectRoot;
            
            // 构建目录结构: Intermediate/Platform/Configuration/
            BuildRoot = Path.Combine(projectRoot, "Intermediate");
            PlatformDir = Path.Combine(BuildRoot, platform);
            ConfigurationDir = Path.Combine(PlatformDir, configuration);
            
            // 中间文件目录 (编译产生的 .o/.obj 文件)
            IntermediateDir = Path.Combine(ConfigurationDir, "Intermediate");
            ObjectDir = Path.Combine(IntermediateDir, "Objects");
            
            // 输出目录 (最终的可执行文件/库文件)
            OutputDir = Path.Combine(projectRoot, "Binary");

            Logger.Info($"构建路径配置:");
            Logger.Info($"  项目根目录: {ProjectRoot}");
            Logger.Info($"  构建根目录: {BuildRoot}");
            Logger.Info($"  输出目录: {OutputDir}");
            Logger.Info($"  中间目录: {IntermediateDir}");
            Logger.Info($"  对象文件目录: {ObjectDir}");
        }

        /// <summary>
        /// 确保所有必要的目录存在
        /// </summary>
        public void EnsureDirectoriesExist()
        {
            Logger.Info("创建构建目录结构...");
            
            Directory.CreateDirectory(BuildRoot);
            Directory.CreateDirectory(PlatformDir);
            Directory.CreateDirectory(ConfigurationDir);
            Directory.CreateDirectory(IntermediateDir);
            Directory.CreateDirectory(ObjectDir);
            Directory.CreateDirectory(OutputDir);
            
            Logger.Info("构建目录结构创建完成");
        }

        /// <summary>
        /// 获取目标的对象文件目录
        /// </summary>
        public string GetTargetObjectDir(string targetName)
        {
            var targetObjDir = Path.Combine(ObjectDir, targetName);
            Directory.CreateDirectory(targetObjDir);
            return targetObjDir;
        }

        /// <summary>
        /// 获取源文件对应的对象文件路径
        /// </summary>
        public string GetObjectFilePath(string sourceFile, string targetName, string objectExtension)
        {
            var sourceFileName = Path.GetFileNameWithoutExtension(sourceFile);
            var objectFileName = sourceFileName + objectExtension;
            var targetObjDir = GetTargetObjectDir(targetName);
            return Path.Combine(targetObjDir, objectFileName);
        }

        /// <summary>
        /// 获取输出文件路径
        /// </summary>
        public string GetOutputFilePath(string targetName, string extension)
        {
            var outputFileName = targetName + extension;
            return Path.Combine(OutputDir, outputFileName);
        }

        /// <summary>
        /// 清理构建目录
        /// </summary>
        public void Clean(CleanOptions options = CleanOptions.Default)
        {
            Logger.Info($"开始清理构建目录，选项: {options}...");
            
            // 清理特定配置的构建目录
            if (options.HasFlag(CleanOptions.BuildFiles))
            {
                if (Directory.Exists(ConfigurationDir))
                {
                    try
                    {
                        Directory.Delete(ConfigurationDir, true);
                        Logger.Info($"已清理构建目录: {ConfigurationDir}");
                    }
                    catch (Exception ex)
                    {
                        Logger.Warning($"清理构建目录失败: {ex.Message}");
                    }
                }
                else
                {
                    Logger.Info("构建目录不存在，无需清理");
                }
            }

            // 清理输出目录中的相关文件
            if (options.HasFlag(CleanOptions.OutputFiles))
            {
                CleanOutputFiles();
            }
            
            // 清理生成的中间文件
            if (options.HasFlag(CleanOptions.GeneratedFiles))
            {
                CleanGeneratedFiles();
            }
        }

        /// <summary>
        /// 清理输出文件
        /// </summary>
        private void CleanOutputFiles()
        {
            if (!Directory.Exists(OutputDir))
            {
                Logger.Info("输出目录不存在，跳过清理");
                return;
            }

            try
            {
                // 清理可执行文件和库文件
                var executableExtensions = new[] { ".exe", ".app", "" }; // 空字符串用于 Unix 可执行文件
                var libraryExtensions = new[] { ".dll", ".so", ".dylib", ".a", ".lib" };
                
                var allExtensions = executableExtensions.Concat(libraryExtensions);
                
                foreach (var file in Directory.GetFiles(OutputDir, "*.*", SearchOption.TopDirectoryOnly))
                {
                    var extension = Path.GetExtension(file).ToLowerInvariant();
                    if (allExtensions.Contains(extension) || string.IsNullOrEmpty(extension))
                    {
                        try
                        {
                            File.Delete(file);
                            Logger.Info($"已删除输出文件: {Path.GetFileName(file)}");
                        }
                        catch (Exception ex)
                        {
                            Logger.Warning($"删除输出文件失败 {file}: {ex.Message}");
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Warning($"清理输出目录失败: {ex.Message}");
            }
        }

        /// <summary>
        /// 清理生成的文件
        /// </summary>
        private void CleanGeneratedFiles()
        {
            var generatedDir = Path.Combine(ProjectRoot, "Intermediate", "Generated");
            if (Directory.Exists(generatedDir))
            {
                try
                {
                    Directory.Delete(generatedDir, true);
                    Logger.Info($"已清理生成文件目录: {generatedDir}");
                }
                catch (Exception ex)
                {
                    Logger.Warning($"清理生成文件目录失败: {ex.Message}");
                }
            }
            else
            {
                Logger.Info("生成文件目录不存在，跳过清理");
            }
        }

        /// <summary>
        /// 检查源文件是否需要重新编译
        /// </summary>
        public bool IsSourceFileNeedRecompile(string sourceFile, string objectFile)
        {
            if (!File.Exists(objectFile))
            {
                Logger.Debug($"对象文件不存在，需要编译: {sourceFile}");
                return true;
            }

            var sourceTime = File.GetLastWriteTime(sourceFile);
            var objectTime = File.GetLastWriteTime(objectFile);

            if (sourceTime > objectTime)
            {
                Logger.Debug($"源文件较新，需要重新编译: {sourceFile}");
                return true;
            }

            return false;
        }

        /// <summary>
        /// 获取依赖文件路径 (.d 文件，用于增量编译)
        /// </summary>
        public string GetDependencyFilePath(string sourceFile, string targetName)
        {
            var sourceFileName = Path.GetFileNameWithoutExtension(sourceFile);
            var depFileName = sourceFileName + ".d";
            var targetObjDir = GetTargetObjectDir(targetName);
            return Path.Combine(targetObjDir, depFileName);
        }
    }
}