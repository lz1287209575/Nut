using NutBuildSystem.BuildTargets;
using NutBuildSystem.Compilation;
using NutBuildSystem.IO;
using NutBuildSystem.Logging;
using NutBuildSystem.Models;

namespace NutBuildSystem.Discovery;

/// <summary>
/// 模块发现器 - 根据nprx文件发现模块并定位Meta文件
/// </summary>
public class ModuleDiscovery
{
    /// <summary>
    /// 根据nprx项目文件发现所有构建目标
    /// </summary>
    /// <param name="projectRoot">项目根目录</param>
    /// <param name="logger">日志记录器</param>
    /// <returns>构建目标信息列表</returns>
    public static async Task<List<ModuleInfo>> DiscoverModulesAsync(string projectRoot, ILogger? logger = null)
    {
        logger ??= new ConsoleLogger();
        var modules = new List<ModuleInfo>();
        
        // 1. 定位并读取nprx文件
        var nutProject = NutProjectReader.FindAndReadProject(projectRoot, logger);
        if (nutProject == null)
        {
            logger.Warning("No .nprx project file found, cannot discover modules");
            return modules;
        }
        
        logger.Info($"Found project: {nutProject.EngineAssociation}");
        
        // 2. 遍历所有模块，寻找对应的Meta文件
        foreach (var module in nutProject.Modules)
        {
            var moduleInfo = await DiscoverModuleAsync(module, nutProject, projectRoot, logger);
            if (moduleInfo != null)
            {
                modules.Add(moduleInfo);
            }
        }
        
        logger.Info($"Discovered {modules.Count} modules");
        return modules;
    }
    
    /// <summary>
    /// 发现单个模块的信息
    /// </summary>
    /// <param name="module">nprx中的模块配置</param>
    /// <param name="nutProject">项目配置</param>
    /// <param name="projectRoot">项目根目录</param>
    /// <param name="logger">日志记录器</param>
    /// <returns>模块信息</returns>
    private static async Task<ModuleInfo?> DiscoverModuleAsync(NutModule module, NutProject nutProject, string projectRoot, ILogger logger)
    {
        try
        {
            // 根据模块名称搜索可能的路径
            var possiblePaths = GetPossibleModulePaths(module, nutProject, projectRoot);
            
            string? modulePath = null;
            string? metaFilePath = null;
            
            // 寻找模块目录和Meta文件
            foreach (var path in possiblePaths)
            {
                if (Directory.Exists(path))
                {
                    modulePath = path;
                    
                    // 在模块目录下寻找Meta文件
                    metaFilePath = FindMetaFile(path, module.Name);
                    
                    if (metaFilePath != null)
                    {
                        break;
                    }
                }
            }
            
            if (modulePath == null)
            {
                logger.Warning($"Module directory not found for: {module.Name}");
                return null;
            }
            
            var moduleInfo = new ModuleInfo
            {
                Name = module.Name,
                Type = module.Type,
                LoadingPhase = module.LoadingPhase,
                ModulePath = modulePath,
                MetaFilePath = metaFilePath,
                SourcesPath = Path.Combine(modulePath, "Sources")
            };
            
            // 如果找到Meta文件，加载构建目标信息
            if (!string.IsNullOrEmpty(metaFilePath) && File.Exists(metaFilePath))
            {
                moduleInfo.BuildTarget = await BuildTargetCompiler.CompileAndCreateTargetAsync(metaFilePath);
                logger.Debug($"Loaded Meta file for {module.Name}: {metaFilePath}");
            }
            else
            {
                logger.Warning($"No Meta file found for module: {module.Name}");
            }
            
            return moduleInfo;
        }
        catch (Exception ex)
        {
            logger.Error($"Error discovering module {module.Name}: {ex.Message}");
            return null;
        }
    }
    
    /// <summary>
    /// 获取模块可能的路径列表
    /// </summary>
    /// <param name="module">模块配置</param>
    /// <param name="nutProject">项目配置</param>
    /// <param name="projectRoot">项目根目录</param>
    /// <returns>可能的路径列表</returns>
    private static List<string> GetPossibleModulePaths(NutModule module, NutProject nutProject, string projectRoot)
    {
        var paths = new List<string>();
        
        // 根据模块类型确定搜索路径
        var searchDirs = module.Type.ToLower() switch
        {
            "corelib" => new[] { "Source/Runtime", "Source/Runtime/NLib" },
            "executable" => new[] { "Source/Runtime/MicroServices", "Source/Runtime" },
            _ => new[] { "Source/Runtime", "Source/Programs" }
        };
        
        // 添加AdditionalRootDirectories中的路径
        searchDirs = searchDirs.Concat(nutProject.AdditionalRootDirectories).ToArray();
        
        foreach (var searchDir in searchDirs)
        {
            var baseDir = Path.Combine(projectRoot, searchDir);
            
            // 直接路径
            paths.Add(Path.Combine(baseDir, module.Name));
            
            // 递归搜索子目录中匹配的模块名
            if (Directory.Exists(baseDir))
            {
                var subDirs = Directory.GetDirectories(baseDir, module.Name, SearchOption.AllDirectories);
                paths.AddRange(subDirs);
            }
        }
        
        return paths.Distinct().ToList();
    }
    
    /// <summary>
    /// 在指定目录中寻找Meta文件
    /// </summary>
    /// <param name="moduleDirectory">模块目录</param>
    /// <param name="moduleName">模块名称</param>
    /// <returns>Meta文件路径，如果未找到则返回null</returns>
    private static string? FindMetaFile(string moduleDirectory, string moduleName)
    {
        // 常见的Meta文件命名模式
        var metaFileNames = new[]
        {
            $"{moduleName}.Build.cs",
            "Build.cs",
            $"Meta/{moduleName}.Build.cs",
            $"Meta/Build.cs"
        };
        
        foreach (var fileName in metaFileNames)
        {
            var fullPath = Path.Combine(moduleDirectory, fileName);
            if (File.Exists(fullPath))
            {
                return fullPath;
            }
        }
        
        // 递归搜索Meta目录
        var metaDir = Path.Combine(moduleDirectory, "Meta");
        if (Directory.Exists(metaDir))
        {
            var buildFiles = Directory.GetFiles(metaDir, "*.Build.cs", SearchOption.TopDirectoryOnly);
            if (buildFiles.Length > 0)
            {
                return buildFiles[0];
            }
        }
        
        return null;
    }
}

/// <summary>
/// 模块信息
/// </summary>
public class ModuleInfo
{
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty;
    public string LoadingPhase { get; set; } = string.Empty;
    public string ModulePath { get; set; } = string.Empty;
    public string? MetaFilePath { get; set; }
    public string SourcesPath { get; set; } = string.Empty;
    public INutBuildTarget? BuildTarget { get; set; }
}