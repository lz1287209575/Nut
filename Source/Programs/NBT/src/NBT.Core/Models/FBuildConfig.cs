using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 构建配置
/// </summary>
public class FBuildConfig
{
    /// <summary>
    /// 编译器类型
    /// </summary>
    [JsonPropertyName("compiler")]
    public string Compiler { get; set; } = "g++";

    /// <summary>
    /// C++ 标准
    /// </summary>
    [JsonPropertyName("cpp_standard")]
    public string CppStandard { get; set; } = "c++20";

    /// <summary>
    /// 优化级别
    /// </summary>
    [JsonPropertyName("optimization")]
    public string Optimization { get; set; } = "O2";

    /// <summary>
    /// 调试信息
    /// </summary>
    [JsonPropertyName("debug_info")]
    public bool bDebugInfo { get; set; } = false;

    /// <summary>
    /// 警告级别
    /// </summary>
    [JsonPropertyName("warning_level")]
    public string WarningLevel { get; set; } = "Wall";

    /// <summary>
    /// 是否启用并行编译
    /// </summary>
    [JsonPropertyName("parallel_build")]
    public bool bParallelBuild { get; set; } = true;

    /// <summary>
    /// 并行编译线程数
    /// </summary>
    [JsonPropertyName("parallel_threads")]
    public int ParallelThreads { get; set; } = Environment.ProcessorCount;

    /// <summary>
    /// 是否启用增量编译
    /// </summary>
    [JsonPropertyName("incremental_build")]
    public bool bIncrementalBuild { get; set; } = true;

    /// <summary>
    /// 是否启用预编译头文件
    /// </summary>
    [JsonPropertyName("precompiled_headers")]
    public bool bPrecompiledHeaders { get; set; } = false;

    /// <summary>
    /// 预编译头文件路径
    /// </summary>
    [JsonPropertyName("pch_file")]
    public string PchFile { get; set; } = string.Empty;

    /// <summary>
    /// 额外的编译选项
    /// </summary>
    [JsonPropertyName("extra_flags")]
    public List<string> ExtraFlags { get; set; } = new();

    /// <summary>
    /// 额外的链接选项
    /// </summary>
    [JsonPropertyName("link_flags")]
    public List<string> LinkFlags { get; set; } = new();

    /// <summary>
    /// 定义宏
    /// </summary>
    [JsonPropertyName("defines")]
    public List<string> Defines { get; set; } = new();

    /// <summary>
    /// 获取编译器命令
    /// </summary>
    /// <returns>编译器命令</returns>
    public virtual string GetCompilerCommand()
    {
        return Compiler;
    }

    /// <summary>
    /// 获取编译选项字符串
    /// </summary>
    /// <returns>编译选项字符串</returns>
    public virtual string GetCompileFlags()
    {
        var Flags = new List<string>
        {
            $"-std={CppStandard}",
            $"-{Optimization}"
        };

        if (bDebugInfo)
        {
            Flags.Add("-g");
        }

        if (!string.IsNullOrEmpty(WarningLevel))
        {
            Flags.Add($"-{WarningLevel}");
        }

        foreach (var Define in Defines)
        {
            Flags.Add($"-D{Define}");
        }

        Flags.AddRange(ExtraFlags);

        return string.Join(" ", Flags);
    }

    /// <summary>
    /// 获取链接选项字符串
    /// </summary>
    /// <returns>链接选项字符串</returns>
    public virtual string GetLinkFlags()
    {
        return string.Join(" ", LinkFlags);
    }

    /// <summary>
    /// 验证构建配置是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public virtual bool IsValid()
    {
        return !string.IsNullOrEmpty(Compiler) && 
               !string.IsNullOrEmpty(CppStandard) && 
               !string.IsNullOrEmpty(Optimization);
    }
} 