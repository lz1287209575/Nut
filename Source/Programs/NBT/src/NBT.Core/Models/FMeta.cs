using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// Meta 基类，所有模块的 Meta 类都继承此类
/// </summary>
public abstract class FMeta
{
    /// <summary>
    /// 模块名称
    /// </summary>
    [JsonPropertyName("name")]
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// 模块类型
    /// </summary>
    [JsonPropertyName("type")]
    public string Type { get; set; } = string.Empty;

    /// <summary>
    /// 模块描述
    /// </summary>
    [JsonPropertyName("description")]
    public string Description { get; set; } = string.Empty;

    /// <summary>
    /// 版本号
    /// </summary>
    [JsonPropertyName("version")]
    public string Version { get; set; } = "1.0.0";

    /// <summary>
    /// 作者
    /// </summary>
    [JsonPropertyName("author")]
    public string Author { get; set; } = string.Empty;

    /// <summary>
    /// 创建时间
    /// </summary>
    [JsonPropertyName("created_at")]
    public DateTime CreatedAt { get; set; } = DateTime.Now;

    /// <summary>
    /// 更新时间
    /// </summary>
    [JsonPropertyName("updated_at")]
    public DateTime UpdatedAt { get; set; } = DateTime.Now;

    /// <summary>
    /// 是否启用
    /// </summary>
    [JsonPropertyName("enabled")]
    public bool bEnabled { get; set; } = true;

    /// <summary>
    /// 模块标签
    /// </summary>
    [JsonPropertyName("tags")]
    public List<string> Tags { get; set; } = new();

    /// <summary>
    /// 元数据文件路径
    /// </summary>
    public string MetaFilePath { get; set; } = string.Empty;

    /// <summary>
    /// 项目根目录
    /// </summary>
    public string ProjectRoot { get; set; } = string.Empty;

    /// <summary>
    /// 模块根目录
    /// </summary>
    public string ModuleRoot { get; set; } = string.Empty;

    /// <summary>
    /// 验证元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public virtual bool IsValid()
    {
        return !string.IsNullOrEmpty(Name) && !string.IsNullOrEmpty(Type);
    }

    /// <summary>
    /// 获取模块的完整路径
    /// </summary>
    /// <returns>完整路径</returns>
    public virtual string GetFullPath()
    {
        return Path.Combine(ProjectRoot, ModuleRoot);
    }

    /// <summary>
    /// 获取模块的显示名称
    /// </summary>
    /// <returns>显示名称</returns>
    public virtual string GetDisplayName()
    {
        return string.IsNullOrEmpty(Description) ? Name : Description;
    }

    /// <summary>
    /// 更新元数据
    /// </summary>
    public virtual void Update()
    {
        UpdatedAt = DateTime.Now;
    }

    /// <summary>
    /// 克隆元数据
    /// </summary>
    /// <returns>克隆的元数据</returns>
    public virtual FMeta Clone()
    {
        return (FMeta)MemberwiseClone();
    }
} 