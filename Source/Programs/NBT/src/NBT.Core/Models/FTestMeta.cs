using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 测试元数据
/// </summary>
public class FTestMeta : FBuildMeta
{
    /// <summary>
    /// 测试类型
    /// </summary>
    [JsonPropertyName("test_type")]
    public string TestType { get; set; } = "unit";

    /// <summary>
    /// 测试框架
    /// </summary>
    [JsonPropertyName("framework")]
    public string Framework { get; set; } = "gtest";

    /// <summary>
    /// 测试超时时间（秒）
    /// </summary>
    [JsonPropertyName("timeout")]
    public int Timeout { get; set; } = 300;

    /// <summary>
    /// 是否启用并行测试
    /// </summary>
    [JsonPropertyName("parallel")]
    public bool bParallel { get; set; } = true;

    /// <summary>
    /// 并行测试线程数
    /// </summary>
    [JsonPropertyName("parallel_threads")]
    public int ParallelThreads { get; set; } = Environment.ProcessorCount;

    /// <summary>
    /// 测试过滤器
    /// </summary>
    [JsonPropertyName("filter")]
    public string Filter { get; set; } = string.Empty;

    /// <summary>
    /// 测试输出格式
    /// </summary>
    [JsonPropertyName("output_format")]
    public string OutputFormat { get; set; } = "xml";

    /// <summary>
    /// 测试覆盖率
    /// </summary>
    [JsonPropertyName("coverage")]
    public FCoverageConfig Coverage { get; set; } = new();

    /// <summary>
    /// 测试数据目录
    /// </summary>
    [JsonPropertyName("test_data_dir")]
    public string TestDataDir { get; set; } = "test_data";

    /// <summary>
    /// 测试结果目录
    /// </summary>
    [JsonPropertyName("test_results_dir")]
    public string TestResultsDir { get; set; } = "test_results";

    /// <summary>
    /// 模拟对象配置
    /// </summary>
    [JsonPropertyName("mocks")]
    public List<string> Mocks { get; set; } = new();

    /// <summary>
    /// 验证测试元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public override bool IsValid()
    {
        return base.IsValid() && !string.IsNullOrEmpty(TestType) && !string.IsNullOrEmpty(Framework);
    }

    /// <summary>
    /// 获取测试可执行文件路径
    /// </summary>
    /// <returns>测试可执行文件路径</returns>
    public virtual string GetTestExecutablePath()
    {
        return GetOutputPath();
    }

    /// <summary>
    /// 获取测试数据路径
    /// </summary>
    /// <returns>测试数据路径</returns>
    public virtual string GetTestDataPath()
    {
        return Path.Combine(GetFullPath(), TestDataDir);
    }

    /// <summary>
    /// 获取测试结果路径
    /// </summary>
    /// <returns>测试结果路径</returns>
    public virtual string GetTestResultsPath()
    {
        return Path.Combine(GetFullPath(), TestResultsDir);
    }

    /// <summary>
    /// 获取覆盖率报告路径
    /// </summary>
    /// <returns>覆盖率报告路径</returns>
    public virtual string GetCoverageReportPath()
    {
        return Path.Combine(GetTestResultsPath(), "coverage");
    }
}

/// <summary>
/// 覆盖率配置
/// </summary>
public class FCoverageConfig
{
    /// <summary>
    /// 是否启用覆盖率
    /// </summary>
    [JsonPropertyName("enabled")]
    public bool bEnabled { get; set; } = false;

    /// <summary>
    /// 覆盖率工具
    /// </summary>
    [JsonPropertyName("tool")]
    public string Tool { get; set; } = "gcov";

    /// <summary>
    /// 覆盖率报告格式
    /// </summary>
    [JsonPropertyName("report_format")]
    public string ReportFormat { get; set; } = "html";

    /// <summary>
    /// 覆盖率阈值
    /// </summary>
    [JsonPropertyName("threshold")]
    public double Threshold { get; set; } = 80.0;

    /// <summary>
    /// 排除的文件模式
    /// </summary>
    [JsonPropertyName("exclude_patterns")]
    public List<string> ExcludePatterns { get; set; } = new();

    /// <summary>
    /// 包含的文件模式
    /// </summary>
    [JsonPropertyName("include_patterns")]
    public List<string> IncludePatterns { get; set; } = new();

    /// <summary>
    /// 覆盖率输出目录
    /// </summary>
    [JsonPropertyName("output_dir")]
    public string OutputDir { get; set; } = "coverage";

    /// <summary>
    /// 是否生成详细报告
    /// </summary>
    [JsonPropertyName("detailed_report")]
    public bool bDetailedReport { get; set; } = true;

    /// <summary>
    /// 是否生成分支覆盖率
    /// </summary>
    [JsonPropertyName("branch_coverage")]
    public bool bBranchCoverage { get; set; } = false;

    /// <summary>
    /// 是否生成函数覆盖率
    /// </summary>
    [JsonPropertyName("function_coverage")]
    public bool bFunctionCoverage { get; set; } = true;

    /// <summary>
    /// 是否生成行覆盖率
    /// </summary>
    [JsonPropertyName("line_coverage")]
    public bool bLineCoverage { get; set; } = true;
} 