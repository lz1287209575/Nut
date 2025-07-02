using System.Diagnostics;
using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 编译器
/// </summary>
public class FCompiler : ICompiler
{
    /// <summary>
    /// 编译选项
    /// </summary>
    private readonly FBuildConfig BuildConfig;

    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="BuildConfig">构建配置</param>
    public FCompiler(FBuildConfig BuildConfig)
    {
        this.BuildConfig = BuildConfig;
    }

    /// <summary>
    /// 编译单个文件
    /// </summary>
    /// <param name="SourceFile">源文件路径</param>
    /// <param name="OutputFile">输出文件路径</param>
    /// <param name="IncludeDirs">包含目录</param>
    /// <returns>编译结果</returns>
    public async Task<FCompileResult> CompileFileAsync(string SourceFile, string OutputFile, IEnumerable<string> IncludeDirs)
    {
        try
        {
            var CompilerCommand = BuildConfig.GetCompilerCommand();
            var CompileFlags = BuildConfig.GetCompileFlags();
            
            var IncludeFlags = string.Join(" ", IncludeDirs.Select(Dir => $"-I{Dir}"));
            var FullCommand = $"{CompilerCommand} {CompileFlags} {IncludeFlags} -c {SourceFile} -o {OutputFile}";

            var ProcessInfo = new ProcessStartInfo
            {
                FileName = "bash",
                Arguments = $"-c \"{FullCommand}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var Process = new Process();
            Process.StartInfo = ProcessInfo;
            Process.Start();

            var Output = await Process.StandardOutput.ReadToEndAsync();
            var Error = await Process.StandardError.ReadToEndAsync();
            
            await Process.WaitForExitAsync();

            return new FCompileResult
            {
                bSuccess = Process.ExitCode == 0,
                OutputFile = OutputFile,
                StandardOutput = Output,
                StandardError = Error,
                ExitCode = Process.ExitCode
            };
        }
        catch (Exception Ex)
        {
            return new FCompileResult
            {
                bSuccess = false,
                StandardError = Ex.Message,
                ExitCode = -1
            };
        }
    }

    /// <summary>
    /// 链接文件
    /// </summary>
    /// <param name="ObjectFiles">目标文件列表</param>
    /// <param name="OutputFile">输出文件路径</param>
    /// <param name="Libraries">库文件列表</param>
    /// <returns>链接结果</returns>
    public async Task<FCompileResult> LinkFilesAsync(IEnumerable<string> ObjectFiles, string OutputFile, IEnumerable<string> Libraries)
    {
        try
        {
            var CompilerCommand = BuildConfig.GetCompilerCommand();
            var LinkFlags = BuildConfig.GetLinkFlags();
            
            var ObjectFilesList = string.Join(" ", ObjectFiles);
            var LibraryFlags = string.Join(" ", Libraries.Select(Lib => $"-l{Lib}"));
            var FullCommand = $"{CompilerCommand} {LinkFlags} {ObjectFilesList} {LibraryFlags} -o {OutputFile}";

            var ProcessInfo = new ProcessStartInfo
            {
                FileName = "bash",
                Arguments = $"-c \"{FullCommand}\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var Process = new Process();
            Process.StartInfo = ProcessInfo;
            Process.Start();

            var Output = await Process.StandardOutput.ReadToEndAsync();
            var Error = await Process.StandardError.ReadToEndAsync();
            
            await Process.WaitForExitAsync();

            return new FCompileResult
            {
                bSuccess = Process.ExitCode == 0,
                OutputFile = OutputFile,
                StandardOutput = Output,
                StandardError = Error,
                ExitCode = Process.ExitCode
            };
        }
        catch (Exception Ex)
        {
            return new FCompileResult
            {
                bSuccess = false,
                StandardError = Ex.Message,
                ExitCode = -1
            };
        }
    }

    /// <summary>
    /// 检查编译器是否可用
    /// </summary>
    /// <returns>是否可用</returns>
    public async Task<bool> IsCompilerAvailableAsync()
    {
        try
        {
            var CompilerCommand = BuildConfig.GetCompilerCommand();
            var ProcessInfo = new ProcessStartInfo
            {
                FileName = CompilerCommand,
                Arguments = "--version",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var Process = new Process();
            Process.StartInfo = ProcessInfo;
            Process.Start();
            await Process.WaitForExitAsync();

            return Process.ExitCode == 0;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// 获取编译器版本
    /// </summary>
    /// <returns>编译器版本</returns>
    public async Task<string> GetCompilerVersionAsync()
    {
        try
        {
            var CompilerCommand = BuildConfig.GetCompilerCommand();
            var ProcessInfo = new ProcessStartInfo
            {
                FileName = CompilerCommand,
                Arguments = "--version",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var Process = new Process { StartInfo = ProcessInfo };
            Process.Start();
            var Output = await Process.StandardOutput.ReadToEndAsync();
            await Process.WaitForExitAsync();

            if (Process.ExitCode != 0) return "Unknown";
            var Lines = Output.Split('\n', StringSplitOptions.RemoveEmptyEntries);
            return Lines.Length > 0 ? Lines[0].Trim() : "Unknown";

        }
        catch
        {
            return "Unknown";
        }
    }

    public Task<bool> BuildModuleAsync(FBuildMeta InMeta)
    {
        throw new NotImplementedException();
    }

    public Task<bool> CleanModuleAsync(FBuildMeta InMeta)
    {
        throw new NotImplementedException();
    }

    public Task<bool> GenerateCompileCommandsAsync(FBuildMeta InMeta)
    {
        throw new NotImplementedException();
    }

    public Task<string> DetectCompilerAsync()
    {
        throw new NotImplementedException();
    }
}

/// <summary>
/// 编译结果
/// </summary>
public class FCompileResult
{
    /// <summary>
    /// 是否成功
    /// </summary>
    public bool bSuccess { get; set; } = false;

    /// <summary>
    /// 输出文件路径
    /// </summary>
    public string OutputFile { get; set; } = string.Empty;

    /// <summary>
    /// 标准输出
    /// </summary>
    public string StandardOutput { get; set; } = string.Empty;

    /// <summary>
    /// 标准错误
    /// </summary>
    public string StandardError { get; set; } = string.Empty;

    /// <summary>
    /// 退出代码
    /// </summary>
    public int ExitCode { get; set; } = 0;

    /// <summary>
    /// 编译时间（毫秒）
    /// </summary>
    public long CompileTimeMs { get; set; } = 0;
} 