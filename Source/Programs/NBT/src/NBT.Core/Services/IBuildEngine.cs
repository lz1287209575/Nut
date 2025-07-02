using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 构建引擎接口
/// </summary>
public interface IBuildEngine
{
    /// <summary>
    /// 构建项目
    /// </summary>
    /// <param name="Options">构建选项</param>
    /// <returns>构建结果</returns>
    Task<FBuildResult> BuildAsync(Op Options);

    /// <summary>
    /// 构建所有模块
    /// </summary>
    /// <returns>是否成功</returns>
    Task<bool> BuildAllAsync();

    /// <summary>
    /// 构建指定模块
    /// </summary>
    /// <param name="ModuleName">模块名称</param>
    /// <returns>是否成功</returns>
    Task<bool> BuildModuleAsync(string ModuleName);

    /// <summary>
    /// 清理所有模块
    /// </summary>
    /// <returns>是否成功</returns>
    Task<bool> CleanAllAsync();

    /// <summary>
    /// 清理指定模块
    /// </summary>
    /// <param name="ModuleName">模块名称</param>
    /// <returns>是否成功</returns>
    Task<bool> CleanModuleAsync(string ModuleName);

    /// <summary>
    /// 列出所有模块
    /// </summary>
    /// <returns>模块列表</returns>
    Task<List<FMeta>> ListModulesAsync();

    /// <summary>
    /// 刷新 IntelliSense
    /// </summary>
    /// <returns>是否成功</returns>
    Task<bool> RefreshIntelliSenseAsync();
} 