using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 插件元数据
/// </summary>
public class FPluginMeta : FBuildMeta
{
    /// <summary>
    /// 插件类型
    /// </summary>
    [JsonPropertyName("plugin_type")]
    public string PluginType { get; set; } = "dynamic";

    /// <summary>
    /// 插件接口版本
    /// </summary>
    [JsonPropertyName("interface_version")]
    public string InterfaceVersion { get; set; } = "1.0";

    /// <summary>
    /// 插件入口点
    /// </summary>
    [JsonPropertyName("entry_point")]
    public string EntryPoint { get; set; } = string.Empty;

    /// <summary>
    /// 插件依赖
    /// </summary>
    [JsonPropertyName("plugin_dependencies")]
    public List<string> PluginDependencies { get; set; } = new();

    /// <summary>
    /// 插件配置
    /// </summary>
    [JsonPropertyName("config")]
    public Dictionary<string, object> Config { get; set; } = new();

    /// <summary>
    /// 插件权限
    /// </summary>
    [JsonPropertyName("permissions")]
    public List<string> Permissions { get; set; } = new();

    /// <summary>
    /// 插件类别
    /// </summary>
    [JsonPropertyName("category")]
    public string Category { get; set; } = "general";

    /// <summary>
    /// 插件优先级
    /// </summary>
    [JsonPropertyName("priority")]
    public int Priority { get; set; } = 0;

    /// <summary>
    /// 是否启用热重载
    /// </summary>
    [JsonPropertyName("hot_reload")]
    public bool bHotReload { get; set; } = false;

    /// <summary>
    /// 插件元数据
    /// </summary>
    [JsonPropertyName("metadata")]
    public Dictionary<string, string> Metadata { get; set; } = new();

    /// <summary>
    /// 验证插件元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public override bool IsValid()
    {
        return base.IsValid() && !string.IsNullOrEmpty(PluginType) && !string.IsNullOrEmpty(InterfaceVersion);
    }

    /// <summary>
    /// 获取插件文件路径
    /// </summary>
    /// <returns>插件文件路径</returns>
    public virtual string GetPluginPath()
    {
        var PluginName = Path.GetFileNameWithoutExtension(Output);
        var Extension = PluginType == "dynamic" ? ".so" : ".a";
        return Path.Combine(GetOutputPath(), $"lib{PluginName}{Extension}");
    }

    /// <summary>
    /// 获取插件配置路径
    /// </summary>
    /// <returns>插件配置路径</returns>
    public virtual string GetConfigPath()
    {
        return Path.Combine(GetFullPath(), "config");
    }

    /// <summary>
    /// 获取插件数据路径
    /// </summary>
    /// <returns>插件数据路径</returns>
    public virtual string GetDataPath()
    {
        return Path.Combine(GetFullPath(), "data");
    }

    /// <summary>
    /// 获取插件文档路径
    /// </summary>
    /// <returns>插件文档路径</returns>
    public virtual string GetDocsPath()
    {
        return Path.Combine(GetFullPath(), "docs");
    }

    /// <summary>
    /// 检查插件权限
    /// </summary>
    /// <param name="Permission">权限名称</param>
    /// <returns>是否有权限</returns>
    public virtual bool HasPermission(string Permission)
    {
        return Permissions.Contains(Permission) || Permissions.Contains("*");
    }

    /// <summary>
    /// 获取插件配置值
    /// </summary>
    /// <typeparam name="T">配置值类型</typeparam>
    /// <param name="Key">配置键</param>
    /// <param name="DefaultValue">默认值</param>
    /// <returns>配置值</returns>
    public virtual T GetConfigValue<T>(string Key, T DefaultValue = default!)
    {
        if (Config.TryGetValue(Key, out var Value))
        {
            if (Value is T TypedValue)
            {
                return TypedValue;
            }
        }
        return DefaultValue;
    }

    /// <summary>
    /// 设置插件配置值
    /// </summary>
    /// <typeparam name="T">配置值类型</typeparam>
    /// <param name="Key">配置键</param>
    /// <param name="Value">配置值</param>
    public virtual void SetConfigValue<T>(string Key, T Value)
    {
        Config[Key] = Value!;
    }

    /// <summary>
    /// 获取插件元数据值
    /// </summary>
    /// <param name="Key">元数据键</param>
    /// <param name="DefaultValue">默认值</param>
    /// <returns>元数据值</returns>
    public virtual string GetMetadataValue(string Key, string DefaultValue = "")
    {
        return Metadata.TryGetValue(Key, out var Value) ? Value : DefaultValue;
    }

    /// <summary>
    /// 设置插件元数据值
    /// </summary>
    /// <param name="Key">元数据键</param>
    /// <param name="Value">元数据值</param>
    public virtual void SetMetadataValue(string Key, string Value)
    {
        Metadata[Key] = Value;
    }
} 