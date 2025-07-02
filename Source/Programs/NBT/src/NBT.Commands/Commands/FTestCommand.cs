using System.CommandLine;
using NBT.Core.Services;

namespace NBT.Commands.Commands;

/// <summary>
/// 测试命令
/// </summary>
public class FTestCommand : Command
{
    /// <summary>
    /// 测试引擎
    /// </summary>
    private readonly ITestEngine TestEngine;

    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="TestEngine">测试引擎</param>
    public FTestCommand(ITestEngine TestEngine) : base("test", "运行测试")
    {
        this.TestEngine = TestEngine;

        var ProjectPathOption = new Option<string>("--project")
        {
            Required = false,
            DefaultValueFactory = k => ".",
            Description = "项目路径"
        };

        var FilterOption = new Option<string>("--filter")
        {
            Required = false,
            DefaultValueFactory = k => string.Empty,
            Description = "测试过滤器"
        };

        var CoverageOption = new Option<bool>("--coverage")
        {
            Required = false,
            DefaultValueFactory = k => false,
            Description = "生成覆盖率报告"
        };

        var ParallelOption = new Option<bool>("--parallel")
        {
            Required = false,
            DefaultValueFactory = k => true,
            Description = "并行测试"
        };

        var VerboseOption = new Option<bool>("--verbose")
        {
            Required = false,
            DefaultValueFactory = k => false,
            Description = "详细输出"
        };

        Add(ProjectPathOption);
        Add(FilterOption);
        Add(CoverageOption);
        Add(ParallelOption);
        Add(VerboseOption);
        
        SetAction(async result =>
        {
            var ProjectPath = result.GetRequiredValue<string>("--project");
            var Filter =  result.GetRequiredValue<string>("--filter");
            var Coverage = result.GetRequiredValue<bool>("--coverage");
            var Parallel = result.GetRequiredValue<bool>("--parallel");
            var Verbose = result.GetRequiredValue<bool>("--verbose");
            
            await HandleTestAsync(ProjectPath, Filter, Coverage, Parallel, Verbose);
        });
    }

    /// <summary>
    /// 处理测试命令
    /// </summary>
    /// <param name="ProjectPath">项目路径</param>
    /// <param name="Filter">测试过滤器</param>
    /// <param name="Coverage">是否生成覆盖率</param>
    /// <param name="Parallel">是否并行</param>
    /// <param name="Verbose">是否详细输出</param>
    private async Task HandleTestAsync(string ProjectPath, string Filter, bool Coverage, bool Parallel, bool Verbose)
    {
        try
        {
            Console.WriteLine($"开始运行测试: {ProjectPath}");
            Console.WriteLine($"测试过滤器: {Filter}");
            Console.WriteLine($"生成覆盖率: {Coverage}");
            Console.WriteLine($"并行测试: {Parallel}");
            Console.WriteLine($"详细输出: {Verbose}");

            var TestOptions = new FTestOptions
            {
                ProjectPath = ProjectPath,
                Filter = Filter,
                bCoverage = Coverage,
                bParallel = Parallel,
                bVerbose = Verbose
            };

            var Result = await TestEngine.RunTestsAsync(TestOptions);

            if (Result.bSuccess)
            {
                Console.WriteLine("测试运行成功！");
                Console.WriteLine($"通过测试: {Result.PassedTests}");
                Console.WriteLine($"失败测试: {Result.FailedTests}");
                Console.WriteLine($"跳过测试: {Result.SkippedTests}");
            }
            else
            {
                Console.WriteLine($"测试运行失败: {Result.ErrorMessage}");
                Environment.Exit(1);
            }
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"测试过程中发生错误: {Ex.Message}");
            if (Verbose)
            {
                Console.WriteLine($"详细错误信息: {Ex}");
            }
            Environment.Exit(1);
        }
    }
}

/// <summary>
/// 测试选项
/// </summary>
public class FTestOptions
{
    /// <summary>
    /// 项目路径
    /// </summary>
    public string ProjectPath { get; set; } = ".";

    /// <summary>
    /// 测试过滤器
    /// </summary>
    public string Filter { get; set; } = string.Empty;

    /// <summary>
    /// 是否生成覆盖率
    /// </summary>
    public bool bCoverage { get; set; } = false;

    /// <summary>
    /// 是否并行测试
    /// </summary>
    public bool bParallel { get; set; } = true;

    /// <summary>
    /// 是否详细输出
    /// </summary>
    public bool bVerbose { get; set; } = false;

    /// <summary>
    /// 超时时间（秒）
    /// </summary>
    public int Timeout { get; set; } = 300;
}

/// <summary>
/// 测试引擎接口
/// </summary>
public interface ITestEngine
{
    /// <summary>
    /// 运行测试
    /// </summary>
    /// <param name="Options">测试选项</param>
    /// <returns>测试结果</returns>
    Task<FTestResult> RunTestsAsync(FTestOptions Options);
}

/// <summary>
/// 测试结果
/// </summary>
public class FTestResult
{
    /// <summary>
    /// 是否成功
    /// </summary>
    public bool bSuccess { get; set; } = false;

    /// <summary>
    /// 错误消息
    /// </summary>
    public string ErrorMessage { get; set; } = string.Empty;

    /// <summary>
    /// 通过测试数量
    /// </summary>
    public int PassedTests { get; set; } = 0;

    /// <summary>
    /// 失败测试数量
    /// </summary>
    public int FailedTests { get; set; } = 0;

    /// <summary>
    /// 跳过测试数量
    /// </summary>
    public int SkippedTests { get; set; } = 0;

    /// <summary>
    /// 覆盖率百分比
    /// </summary>
    public double CoveragePercentage { get; set; } = 0.0;
} 