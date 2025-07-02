using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 元数据加载器接口
/// </summary>
public interface IMetaLoader
{
    /// <summary>
    /// 加载所有元数据
    /// </summary>
    Task<List<FBuildMeta>> LoadAllMetasAsync();

    /// <summary>
    /// 加载指定模块的元数据
    /// </summary>
    /// <param name="InModuleName">模块名称</param>
    Task<FBuildMeta?> LoadMetaAsync(string InModuleName);

    /// <summary>
    /// 扫描所有可用的模块
    /// </summary>
    Task<List<string>> ScanModulesAsync();
} 