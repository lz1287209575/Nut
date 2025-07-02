using System.CommandLine;
using NBT.Core.Services;

namespace NBT.Commands.Commands;

/// <summary>
/// 构建命令
/// </summary>
public class FBuildCommand : Command
{
    /// <summary>
    /// 构建引擎
    /// </summary>
    private readonly IBuildEngine BuildEngine;

    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="BuildEngine">构建引擎</param>
    public FBuildCommand(IBuildEngine BuildEngine) : base("build", "构建项目")
    {
        this.BuildEngine = BuildEngine;

        var ProjectPathOption = new Option<string>("--project")
        {
            Required = false,
            DefaultValueFactory = _ => ".",
            Description = "项目路径"
        };

        var ConfigOption = new Option<string>("--config")
        {
            Required = false,
            DefaultValueFactory = _ => "Debug",
            Description = "构建配置"
        };

        var CleanOption = new Option<bool>("--clean")
        {
            Required = false,
            DefaultValueFactory = _ => false,
            Description = "清理构建"
        };

        var ParallelOption = new Option<bool>("--parallel")
        {
            Required = false,
            DefaultValueFactory = _ => true,
            Description = "并行构建"
        };

        var VerboseOption = new Option<bool>("--verbose")
        {
            Required = false,
            DefaultValueFactory = k => false,
            Description = "详细输出"
        };
        
        Add(ProjectPathOption);
        Add(ConfigOption);
        Add(CleanOption);
        Add(ParallelOption);
        Add(VerboseOption);
        
        SetAction(async result =>
        {
            var ProjectPath = result.GetValue<string>("--project");
            var Config = result.GetValue<string>("--config");
            var Clean = result.GetValue<bool>("--clean");
            var Parallel = result.GetValue<bool>("--parallel");
            var Verbose = result.GetValue<bool>("--verbose");
            
            await HandleBuildAsync(ProjectPath, Config, Clean, Parallel, Verbose);
        });
    }

    /// <summary>
    /// 处理构建命令
    /// </summary>
    /// <param name="ProjectPath">项目路径</param>
    /// <param name="Config">构建配置</param>
    /// <param name="Clean">是否清理</param>
    /// <param name="Parallel">是否并行</param>
    /// <param name="Verbose">是否详细输出</param>
    private async Task HandleBuildAsync(string ProjectPath, string Config, bool Clean, bool Parallel, bool Verbose)
    {
        try
        {
            Console.WriteLine($"开始构建项目: {ProjectPath}");
            Console.WriteLine($"构建配置: {Config}");
            Console.WriteLine($"清理构建: {Clean}");
            Console.WriteLine($"并行构建: {Parallel}");
            Console.WriteLine($"详细输出: {Verbose}");

            var BuildOptions = new FBuildOptions
            {
                ProjectPath = ProjectPath,
                Configuration = Config,
                bClean = Clean,
                bParallel = Parallel,
                bVerbose = Verbose
            };

            var Result = await BuildEngine.BuildAsync(BuildOptions);

            if (Result.bSuccess)
            {
                Console.WriteLine("构建成功！");
            }
            else
            {
                Console.WriteLine($"构建失败: {Result.ErrorMessage}");
                Environment.Exit(1);
            }
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"构建过程中发生错误: {Ex.Message}");
            if (Verbose)
            {
                Console.WriteLine($"详细错误信息: {Ex}");
            }
            Environment.Exit(1);
        }
    }
}

/// <summary>
/// 构建选项
/// </summary>
public class FBuildOptions
{
    /// <summary>
    /// 项目路径
    /// </summary>
    public string ProjectPath { get; set; } = ".";

    /// <summary>
    /// 构建配置
    /// </summary>
    public string Configuration { get; set; } = "Debug";

    /// <summary>
    /// 是否清理构建
    /// </summary>
    public bool bClean { get; set; } = false;

    /// <summary>
    /// 是否并行构建
    /// </summary>
    public bool bParallel { get; set; } = true;

    /// <summary>
    /// 是否详细输出
    /// </summary>
    public bool bVerbose { get; set; } = false;

    /// <summary>
    /// 目标平台
    /// </summary>
    public string TargetPlatform { get; set; } = "linux-x64";

    /// <summary>
    /// 编译器选项
    /// </summary>
    public string CompilerOptions { get; set; } = string.Empty;

    /// <summary>
    /// 链接器选项
    /// </summary>
    public string LinkerOptions { get; set; } = string.Empty;
} 