using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 构建元数据基类
/// </summary>
public abstract class FBuildMeta : FMeta
{
    [JsonPropertyName("sources")]
    public List<string> Sources { get; set; } = new();

    [JsonPropertyName("include_dirs")]
    public List<string> IncludeDirs { get; set; } = new();

    [JsonPropertyName("dependencies")]
    public List<string> Dependencies { get; set; } = new();

    [JsonPropertyName("output")]
    public string Output { get; set; } = string.Empty;

    /// <summary>
    /// 源码目录
    /// </summary>
    public string SourcesDir { get; set; } = string.Empty;

    /// <summary>
    /// 输出目录
    /// </summary>
    public string OutputDir { get; set; } = string.Empty;

    /// <summary>
    /// 中间文件目录
    /// </summary>
    public string IntermediateDir { get; set; } = string.Empty;

    /// <summary>
    /// 配置文件目录
    /// </summary>
    public string ConfigsDir { get; set; } = string.Empty;

    /// <summary>
    /// 构建配置
    /// </summary>
    [JsonPropertyName("build_config")]
    public FBuildConfig BuildConfig { get; set; } = new();

    /// <summary>
    /// 验证构建元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public override bool IsValid()
    {
        return base.IsValid() && !string.IsNullOrEmpty(Output);
    }

    /// <summary>
    /// 获取构建输出路径
    /// </summary>
    /// <returns>输出路径</returns>
    public virtual string GetOutputPath()
    {
        return Path.Combine(GetFullPath(), OutputDir, Output);
    }

    /// <summary>
    /// 获取源码路径
    /// </summary>
    /// <returns>源码路径</returns>
    public virtual string GetSourcesPath()
    {
        return Path.Combine(GetFullPath(), SourcesDir);
    }

    /// <summary>
    /// 获取中间文件路径
    /// </summary>
    /// <returns>中间文件路径</returns>
    public virtual string GetIntermediatePath()
    {
        return Path.Combine(GetFullPath(), IntermediateDir);
    }
}

/// <summary>
/// 服务构建元数据
/// </summary>
public class FServiceBuildMeta : FBuildMeta
{
    [JsonPropertyName("protos_dir")]
    public string ProtosDir { get; set; } = string.Empty;

    [JsonPropertyName("config_files")]
    public List<string> ConfigFiles { get; set; } = new();
}

/// <summary>
/// 库构建元数据
/// </summary>
public class FLibraryBuildMeta : FBuildMeta
{
    [JsonPropertyName("library_type")]
    public string LibraryType { get; set; } = "static"; // static, shared
} 