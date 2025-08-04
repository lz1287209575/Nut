using NutBuildSystem.BuildTargets;
using NutBuildSystem.Compilation;

namespace NutBuildSystem.Discovery
{
    /// <summary>
    /// 构建目标发现器
    /// </summary>
    public class BuildTargetDiscovery
    {
        /// <summary>
        /// 扫描并加载所有Build.cs文件
        /// </summary>
        /// <param name="sourceRoot">源代码根目录</param>
        /// <returns>构建目标列表</returns>
        public static async Task<List<BuildTargetInfo>> DiscoverBuildTargetsAsync(string sourceRoot = "Source")
        {
            var buildTargets = new List<BuildTargetInfo>();
            
            if (!Directory.Exists(sourceRoot))
            {
                throw new DirectoryNotFoundException($"源代码根目录不存在: {sourceRoot}");
            }

            // 查找所有Build.cs文件
            var buildFiles = Directory.GetFiles(sourceRoot, "*.Build.cs", SearchOption.AllDirectories);
            
            foreach (var buildFile in buildFiles)
            {
                try
                {
                    var target = await BuildTargetCompiler.CompileAndCreateTargetAsync(buildFile);
                    if (target != null)
                    {
                        var buildInfo = CreateBuildTargetInfo(buildFile, target);
                        buildTargets.Add(buildInfo);
                    }
                }
                catch (Exception ex)
                {
                    // 记录编译失败的文件，但继续处理其他文件
                    Console.WriteLine($"警告: 无法编译 {buildFile}: {ex.Message}");
                }
            }
            
            return buildTargets;
        }

        /// <summary>
        /// 创建构建目标信息
        /// </summary>
        private static BuildTargetInfo CreateBuildTargetInfo(string buildFilePath, INutBuildTarget target)
        {
            var directory = Path.GetDirectoryName(buildFilePath) ?? "";
            var parentDir = Path.GetDirectoryName(directory) ?? ""; // Meta的父目录
            
            return new BuildTargetInfo
            {
                Name = target.Name,
                TargetType = target.GetType().Name,
                BuildFilePath = buildFilePath,
                BaseDirectory = parentDir,
                SourcesDirectory = Path.Combine(parentDir, "Sources"),
                Sources = target.Sources?.ToList() ?? new List<string>(),
                IncludeDirs = target.IncludeDirs?.ToList() ?? new List<string>(),
                Dependencies = target.Dependencies?.ToList() ?? new List<string>(),
                OutputName = target.OutputName
            };
        }
    }

    /// <summary>
    /// 构建目标信息
    /// </summary>
    public class BuildTargetInfo
    {
        public string Name { get; set; } = "";
        public string TargetType { get; set; } = "";
        public string BuildFilePath { get; set; } = "";
        public string BaseDirectory { get; set; } = "";
        public string SourcesDirectory { get; set; } = "";
        public string OutputName { get; set; } = "";
        public List<string> Sources { get; set; } = new();
        public List<string> IncludeDirs { get; set; } = new();
        public List<string> Dependencies { get; set; } = new();
        public List<string> ProtoFiles { get; set; } = new();
        public List<string> ConfigFiles { get; set; } = new();
    }
}