using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    /// <summary>
    /// 命令执行器
    /// </summary>
    public class CommandExecutor
    {
        /// <summary>
        /// 执行命令并等待完成
        /// </summary>
        public static async Task<CommandResult> ExecuteAsync(string executable, string arguments, string workingDirectory = "")
        {
            Logger.Debug($"执行命令: {executable} {arguments}");
            
            if (!string.IsNullOrEmpty(workingDirectory))
            {
                Logger.Debug($"工作目录: {workingDirectory}");
            }

            var startInfo = new ProcessStartInfo
            {
                FileName = executable,
                Arguments = arguments,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true,
                WorkingDirectory = workingDirectory
            };

            var output = new StringBuilder();
            var error = new StringBuilder();

            try
            {
                using (var process = new Process { StartInfo = startInfo })
                {
                    process.OutputDataReceived += (sender, args) =>
                    {
                        if (args.Data != null)
                        {
                            output.AppendLine(args.Data);
                            Logger.Debug($"[STDOUT] {args.Data}");
                        }
                    };

                    process.ErrorDataReceived += (sender, args) =>
                    {
                        if (args.Data != null)
                        {
                            error.AppendLine(args.Data);
                            Logger.Debug($"[STDERR] {args.Data}");
                        }
                    };

                    var stopwatch = Stopwatch.StartNew();
                    process.Start();

                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await process.WaitForExitAsync();
                    stopwatch.Stop();

                    var result = new CommandResult
                    {
                        ExitCode = process.ExitCode,
                        StandardOutput = output.ToString(),
                        StandardError = error.ToString(),
                        ExecutionTime = stopwatch.Elapsed,
                        Success = process.ExitCode == 0
                    };

                    if (result.Success)
                    {
                        Logger.Debug($"命令执行成功，耗时: {result.ExecutionTime.TotalMilliseconds:F2}ms");
                    }
                    else
                    {
                        Logger.Error($"命令执行失败，退出码: {result.ExitCode}，耗时: {result.ExecutionTime.TotalMilliseconds:F2}ms");
                        if (!string.IsNullOrEmpty(result.StandardError))
                        {
                            Logger.Error($"错误信息: {result.StandardError.Trim()}");
                        }
                    }

                    return result;
                }
            }
            catch (Exception ex)
            {
                Logger.Error($"执行命令时发生异常: {ex.Message}");
                return new CommandResult
                {
                    ExitCode = -1,
                    StandardOutput = string.Empty,
                    StandardError = ex.Message,
                    ExecutionTime = TimeSpan.Zero,
                    Success = false
                };
            }
        }

        /// <summary>
        /// 执行编译命令
        /// </summary>
        public static async Task<CommandResult> CompileAsync(CompilerConfig compiler, string arguments, string workingDirectory = "")
        {
            Logger.Info($"编译: {Path.GetFileName(compiler.CompilerPath)} {arguments}");
            return await ExecuteAsync(compiler.CompilerPath, arguments, workingDirectory);
        }

        /// <summary>
        /// 执行链接命令
        /// </summary>
        public static async Task<CommandResult> LinkAsync(CompilerConfig compiler, string arguments, string workingDirectory = "")
        {
            Logger.Info($"链接: {Path.GetFileName(compiler.LinkerPath)} {arguments}");
            return await ExecuteAsync(compiler.LinkerPath, arguments, workingDirectory);
        }

        /// <summary>
        /// 执行归档命令 (创建静态库)
        /// </summary>
        public static async Task<CommandResult> ArchiveAsync(CompilerConfig compiler, string arguments, string workingDirectory = "")
        {
            Logger.Info($"归档: {Path.GetFileName(compiler.ArchiverPath)} {arguments}");
            return await ExecuteAsync(compiler.ArchiverPath, arguments, workingDirectory);
        }
    }

    /// <summary>
    /// 命令执行结果
    /// </summary>
    public class CommandResult
    {
        public int ExitCode { get; set; }
        public string StandardOutput { get; set; } = string.Empty;
        public string StandardError { get; set; } = string.Empty;
        public TimeSpan ExecutionTime { get; set; }
        public bool Success { get; set; }

        public override string ToString()
        {
            return $"ExitCode: {ExitCode}, Success: {Success}, ExecutionTime: {ExecutionTime.TotalMilliseconds:F2}ms";
        }
    }
}