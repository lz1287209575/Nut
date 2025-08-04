using NutBuildSystem.Logging;

namespace NutBuildSystem.CommandLine
{
    /// <summary>
    /// 命令执行上下文
    /// </summary>
    public class CommandContext
    {
        /// <summary>
        /// 日志器
        /// </summary>
        public ILogger Logger { get; set; } = LoggerFactory.Default;

        /// <summary>
        /// 是否为详细模式
        /// </summary>
        public bool IsVerbose { get; set; }

        /// <summary>
        /// 是否为静默模式
        /// </summary>
        public bool IsQuiet { get; set; }

        /// <summary>
        /// 是否禁用颜色输出
        /// </summary>
        public bool NoColor { get; set; }

        /// <summary>
        /// 工作目录
        /// </summary>
        public string WorkingDirectory { get; set; } = Directory.GetCurrentDirectory();

        /// <summary>
        /// 取消令牌
        /// </summary>
        public CancellationToken CancellationToken { get; set; } = CancellationToken.None;

        /// <summary>
        /// 设置日志级别
        /// </summary>
        /// <param name="logLevel">日志级别字符串</param>
        public void SetLogLevel(string? logLevel)
        {
            var level = LoggerFactory.ParseLogLevel(logLevel ?? "Info");
            
            // 根据详细和静默模式调整日志级别
            if (IsVerbose && level > LogLevel.Debug)
            {
                level = LogLevel.Debug;
            }
            else if (IsQuiet && level < LogLevel.Error)
            {
                level = LogLevel.Error;
            }

            Logger.MinLevel = level;
        }

        /// <summary>
        /// 设置日志输出
        /// </summary>
        /// <param name="logFile">日志文件路径</param>
        /// <param name="logLevel">日志级别</param>
        public void SetupLogging(string? logFile, string? logLevel)
        {
            var parsedLevel = LoggerFactory.ParseLogLevel(logLevel ?? "Info");
            
            // 根据详细和静默模式调整控制台日志级别
            var consoleLevel = parsedLevel;
            if (IsVerbose && consoleLevel > LogLevel.Debug)
            {
                consoleLevel = LogLevel.Debug;
            }
            else if (IsQuiet && consoleLevel < LogLevel.Error)
            {
                consoleLevel = LogLevel.Error;
            }

            // 创建日志器
            if (!string.IsNullOrEmpty(logFile))
            {
                // 使用复合日志器（控制台 + 文件）
                Logger = LoggerFactory.CreateCompositeLogger(
                    filePath: logFile,
                    consoleLevel: consoleLevel,
                    fileLevel: LogLevel.Debug,
                    useColors: !NoColor
                );
            }
            else
            {
                // 只使用控制台日志器
                Logger = LoggerFactory.CreateConsoleLogger(
                    minLevel: consoleLevel,
                    useColors: !NoColor
                );
            }
        }

        /// <summary>
        /// 释放资源
        /// </summary>
        public void Dispose()
        {
            if (Logger is IDisposable disposable)
            {
                disposable.Dispose();
            }
        }
    }
}