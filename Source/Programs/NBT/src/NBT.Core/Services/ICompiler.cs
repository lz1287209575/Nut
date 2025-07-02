using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 编译器接口
/// </summary>
public interface ICompiler
{
    /// <summary>
    /// 构建模块
    /// </summary>
    /// <param name="InMeta">构建元数据</param>
    Task<bool> BuildModuleAsync(FBuildMeta InMeta);

    /// <summary>
    /// 清理模块
    /// </summary>
    /// <param name="InMeta">构建元数据</param>
    Task<bool> CleanModuleAsync(FBuildMeta InMeta);

    /// <summary>
    /// 生成 compile_commands.json
    /// </summary>
    /// <param name="InMeta">构建元数据</param>
    Task<bool> GenerateCompileCommandsAsync(FBuildMeta InMeta);

    /// <summary>
    /// 检测编译器
    /// </summary>
    Task<string> DetectCompilerAsync();
} 