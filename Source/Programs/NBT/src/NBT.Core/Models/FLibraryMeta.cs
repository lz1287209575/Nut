using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 库元数据
/// </summary>
public class FLibraryMeta : FBuildMeta
{
    /// <summary>
    /// 库类型
    /// </summary>
    [JsonPropertyName("library_type")]
    public string LibraryType { get; set; } = "static";

    /// <summary>
    /// 库文件扩展名
    /// </summary>
    [JsonPropertyName("extension")]
    public string Extension { get; set; } = ".a";

    /// <summary>
    /// 是否生成动态库
    /// </summary>
    [JsonPropertyName("shared")]
    public bool bShared { get; set; } = false;

    /// <summary>
    /// 是否生成静态库
    /// </summary>
    [JsonPropertyName("static")]
    public bool bStatic { get; set; } = true;

    /// <summary>
    /// 是否生成头文件
    /// </summary>
    [JsonPropertyName("headers")]
    public bool bHeaders { get; set; } = true;

    /// <summary>
    /// 头文件目录
    /// </summary>
    [JsonPropertyName("header_dirs")]
    public List<string> HeaderDirs { get; set; } = new();

    /// <summary>
    /// 导出符号
    /// </summary>
    [JsonPropertyName("exports")]
    public List<string> Exports { get; set; } = new();

    /// <summary>
    /// 版本信息
    /// </summary>
    [JsonPropertyName("version_info")]
    public FVersionInfo VersionInfo { get; set; } = new();

    /// <summary>
    /// 许可证信息
    /// </summary>
    [JsonPropertyName("license")]
    public string License { get; set; } = string.Empty;

    /// <summary>
    /// 验证库元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public override bool IsValid()
    {
        return base.IsValid() && !string.IsNullOrEmpty(LibraryType);
    }

    /// <summary>
    /// 获取库文件路径
    /// </summary>
    /// <returns>库文件路径</returns>
    public virtual string GetLibraryPath()
    {
        var LibraryName = Path.GetFileNameWithoutExtension(Output);
        var Extension = bShared ? ".so" : ".a";
        return Path.Combine(GetOutputPath(), $"lib{LibraryName}{Extension}");
    }

    /// <summary>
    /// 获取头文件路径
    /// </summary>
    /// <returns>头文件路径</returns>
    public virtual string GetHeadersPath()
    {
        return Path.Combine(GetOutputPath(), "include");
    }

    /// <summary>
    /// 获取 pkg-config 文件路径
    /// </summary>
    /// <returns>pkg-config 文件路径</returns>
    public virtual string GetPkgConfigPath()
    {
        return Path.Combine(GetOutputPath(), "pkgconfig");
    }
}

/// <summary>
/// 版本信息
/// </summary>
public class FVersionInfo
{
    /// <summary>
    /// 主版本号
    /// </summary>
    [JsonPropertyName("major")]
    public int Major { get; set; } = 1;

    /// <summary>
    /// 次版本号
    /// </summary>
    [JsonPropertyName("minor")]
    public int Minor { get; set; } = 0;

    /// <summary>
    /// 修订版本号
    /// </summary>
    [JsonPropertyName("patch")]
    public int Patch { get; set; } = 0;

    /// <summary>
    /// 构建版本号
    /// </summary>
    [JsonPropertyName("build")]
    public int Build { get; set; } = 0;

    /// <summary>
    /// 版本字符串
    /// </summary>
    [JsonPropertyName("version_string")]
    public string VersionString { get; set; } = string.Empty;

    /// <summary>
    /// 获取完整版本号
    /// </summary>
    /// <returns>完整版本号</returns>
    public virtual string GetFullVersion()
    {
        if (!string.IsNullOrEmpty(VersionString))
        {
            return VersionString;
        }
        return $"{Major}.{Minor}.{Patch}.{Build}";
    }

    /// <summary>
    /// 获取兼容版本号
    /// </summary>
    /// <returns>兼容版本号</returns>
    public virtual string GetCompatibleVersion()
    {
        return $"{Major}.{Minor}";
    }

    /// <summary>
    /// 获取当前版本号
    /// </summary>
    /// <returns>当前版本号</returns>
    public virtual string GetCurrentVersion()
    {
        return $"{Major}.{Minor}.{Patch}";
    }
} 