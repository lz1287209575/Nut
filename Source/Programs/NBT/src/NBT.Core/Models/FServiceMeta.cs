using System.Text.Json.Serialization;

namespace NBT.Core.Models;

/// <summary>
/// 服务元数据
/// </summary>
public class FServiceMeta : FBuildMeta
{
    /// <summary>
    /// 服务类型
    /// </summary>
    [JsonPropertyName("service_type")]
    public string ServiceType { get; set; } = "microservice";

    /// <summary>
    /// 监听端口
    /// </summary>
    [JsonPropertyName("port")]
    public int Port { get; set; } = 0;

    /// <summary>
    /// 协议类型
    /// </summary>
    [JsonPropertyName("protocol")]
    public string Protocol { get; set; } = "http";

    /// <summary>
    /// 健康检查路径
    /// </summary>
    [JsonPropertyName("health_check")]
    public string HealthCheck { get; set; } = "/health";

    /// <summary>
    /// 是否启用自动重启
    /// </summary>
    [JsonPropertyName("auto_restart")]
    public bool bAutoRestart { get; set; } = true;

    /// <summary>
    /// 重启策略
    /// </summary>
    [JsonPropertyName("restart_policy")]
    public string RestartPolicy { get; set; } = "always";

    /// <summary>
    /// 环境变量
    /// </summary>
    [JsonPropertyName("environment")]
    public Dictionary<string, string> Environment { get; set; } = new();

    /// <summary>
    /// 依赖服务
    /// </summary>
    [JsonPropertyName("service_dependencies")]
    public List<string> ServiceDependencies { get; set; } = new();

    /// <summary>
    /// 资源限制
    /// </summary>
    [JsonPropertyName("resources")]
    public FResourceLimits Resources { get; set; } = new();

    /// <summary>
    /// 日志配置
    /// </summary>
    [JsonPropertyName("logging")]
    public FLoggingConfig Logging { get; set; } = new();

    /// <summary>
    /// 验证服务元数据是否有效
    /// </summary>
    /// <returns>是否有效</returns>
    public override bool IsValid()
    {
        return base.IsValid() && !string.IsNullOrEmpty(ServiceType);
    }

    /// <summary>
    /// 获取服务端点
    /// </summary>
    /// <returns>服务端点</returns>
    public virtual string GetServiceEndpoint()
    {
        if (Port <= 0) return string.Empty;
        return $"{Protocol}://localhost:{Port}";
    }

    /// <summary>
    /// 获取健康检查端点
    /// </summary>
    /// <returns>健康检查端点</returns>
    public virtual string GetHealthCheckEndpoint()
    {
        var Endpoint = GetServiceEndpoint();
        if (string.IsNullOrEmpty(Endpoint)) return string.Empty;
        return $"{Endpoint}{HealthCheck}";
    }
}

/// <summary>
/// 资源限制
/// </summary>
public class FResourceLimits
{
    /// <summary>
    /// CPU 限制
    /// </summary>
    [JsonPropertyName("cpu")]
    public string Cpu { get; set; } = "1.0";

    /// <summary>
    /// 内存限制
    /// </summary>
    [JsonPropertyName("memory")]
    public string Memory { get; set; } = "512Mi";

    /// <summary>
    /// 磁盘限制
    /// </summary>
    [JsonPropertyName("disk")]
    public string Disk { get; set; } = "1Gi";
}

/// <summary>
/// 日志配置
/// </summary>
public class FLoggingConfig
{
    /// <summary>
    /// 日志级别
    /// </summary>
    [JsonPropertyName("level")]
    public string Level { get; set; } = "info";

    /// <summary>
    /// 日志格式
    /// </summary>
    [JsonPropertyName("format")]
    public string Format { get; set; } = "json";

    /// <summary>
    /// 日志文件路径
    /// </summary>
    [JsonPropertyName("file_path")]
    public string FilePath { get; set; } = "logs";

    /// <summary>
    /// 是否输出到控制台
    /// </summary>
    [JsonPropertyName("console_output")]
    public bool bConsoleOutput { get; set; } = true;

    /// <summary>
    /// 是否输出到文件
    /// </summary>
    [JsonPropertyName("file_output")]
    public bool bFileOutput { get; set; } = true;

    /// <summary>
    /// 日志轮转大小
    /// </summary>
    [JsonPropertyName("max_file_size")]
    public string MaxFileSize { get; set; } = "100MB";

    /// <summary>
    /// 保留日志文件数量
    /// </summary>
    [JsonPropertyName("max_files")]
    public int MaxFiles { get; set; } = 10;
} 