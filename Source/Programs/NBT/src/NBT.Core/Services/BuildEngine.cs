using Microsoft.Extensions.Logging;
using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 构建引擎
/// </summary>
public class BuildEngine : IBuildEngine
{
    /// <summary>
    /// 元数据加载器
    /// </summary>
    private readonly IMetaLoader MetaLoader;

    /// <summary>
    /// 编译器
    /// </summary>
    private readonly ICompiler Compiler;

    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="MetaLoader">元数据加载器</param>
    /// <param name="Compiler">编译器</param>
    public BuildEngine(IMetaLoader MetaLoader, ICompiler Compiler)
    {
        this.MetaLoader = MetaLoader;
        this.Compiler = Compiler;
    }

    /// <summary>
    /// 构建项目
    /// </summary>
    /// <param name="Options">构建选项</param>
    /// <returns>构建结果</returns>
    public async Task<FBuildResult> BuildAsync(FBuildOptions Options)
    {
        try
        {
            Console.WriteLine($"开始构建项目: {Options.ProjectPath}");

            // 检查编译器是否可用
            if (!await Compiler.IsCompilerAvailableAsync())
            {
                return new FBuildResult
                {
                    bSuccess = false,
                    ErrorMessage = "编译器不可用"
                };
            }

            // 获取编译器版本
            var CompilerVersion = await Compiler.GetCompilerVersionAsync();
            Console.WriteLine($"使用编译器: {CompilerVersion}");

            // 模拟构建过程
            await Task.Delay(1000); // 模拟构建时间

            Console.WriteLine("构建完成");
            return new FBuildResult
            {
                bSuccess = true,
                OutputFile = Path.Combine(Options.ProjectPath, "output", "app"),
                BuildTimeMs = 1000
            };
        }
        catch (Exception Ex)
        {
            return new FBuildResult
            {
                bSuccess = false,
                ErrorMessage = Ex.Message
            };
        }
    }

    /// <summary>
    /// 构建所有模块
    /// </summary>
    /// <returns>是否成功</returns>
    public async Task<bool> BuildAllAsync()
    {
        try
        {
            Console.WriteLine("开始构建所有模块");
            await Task.Delay(1000); // 模拟构建时间
            Console.WriteLine("所有模块构建完成");
            return true;
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"构建失败: {Ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// 构建指定模块
    /// </summary>
    /// <param name="ModuleName">模块名称</param>
    /// <returns>是否成功</returns>
    public async Task<bool> BuildModuleAsync(string ModuleName)
    {
        try
        {
            Console.WriteLine($"开始构建模块: {ModuleName}");
            await Task.Delay(500); // 模拟构建时间
            Console.WriteLine($"模块 {ModuleName} 构建完成");
            return true;
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"构建模块 {ModuleName} 失败: {Ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// 清理所有模块
    /// </summary>
    /// <returns>是否成功</returns>
    public async Task<bool> CleanAllAsync()
    {
        try
        {
            Console.WriteLine("开始清理所有模块");
            await Task.Delay(500); // 模拟清理时间
            Console.WriteLine("所有模块清理完成");
            return true;
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"清理失败: {Ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// 清理指定模块
    /// </summary>
    /// <param name="ModuleName">模块名称</param>
    /// <returns>是否成功</returns>
    public async Task<bool> CleanModuleAsync(string ModuleName)
    {
        try
        {
            Console.WriteLine($"开始清理模块: {ModuleName}");
            await Task.Delay(200); // 模拟清理时间
            Console.WriteLine($"模块 {ModuleName} 清理完成");
            return true;
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"清理模块 {ModuleName} 失败: {Ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// 列出所有模块
    /// </summary>
    /// <returns>模块列表</returns>
    public async Task<List<FMeta>> ListModulesAsync()
    {
        try
        {
            Console.WriteLine("获取模块列表");
            await Task.Delay(100); // 模拟获取时间
            
            // 返回模拟的模块列表
            return new List<FMeta>
            {
                new FServiceMeta { Name = "ServiceAllocate", Type = "service" },
                new FLibraryMeta { Name = "LibNut", Type = "library" },
                new FTestMeta { Name = "TestSuite", Type = "test" }
            };
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"获取模块列表失败: {Ex.Message}");
            return new List<FMeta>();
        }
    }

    /// <summary>
    /// 刷新 IntelliSense
    /// </summary>
    /// <returns>是否成功</returns>
    public async Task<bool> RefreshIntelliSenseAsync()
    {
        try
        {
            Console.WriteLine("开始刷新 IntelliSense");
            await Task.Delay(2000); // 模拟刷新时间
            Console.WriteLine("IntelliSense 刷新完成");
            return true;
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"刷新 IntelliSense 失败: {Ex.Message}");
            return false;
        }
    }
}

/// <summary>
/// 构建结果
/// </summary>
public class FBuildResult
{
    /// <summary>
    /// 是否成功
    /// </summary>
    public bool bSuccess { get; set; } = false;

    /// <summary>
    /// 错误消息
    /// </summary>
    public string ErrorMessage { get; set; } = string.Empty;

    /// <summary>
    /// 输出文件路径
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// 构建时间（毫秒）
    /// </summary>
    public long BuildTimeMs { get; set; } = 0;

    /// <summary>
    /// 编译的文件数量
    /// </summary>
    public int CompiledFiles { get; set; } = 0;

    /// <summary>
    /// 警告数量
    /// </summary>
    public int Warnings { get; set; } = 0;

    /// <summary>
    /// 错误数量
    /// </summary>
    public int Errors { get; set; } = 0;
} 