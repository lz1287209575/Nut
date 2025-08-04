namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 复合日志器 - 可以同时向多个日志器输出
    /// </summary>
    public class CompositeLogger : ILogger, IDisposable
    {
        public LogLevel MinLevel { get; set; } = LogLevel.Info;

        private readonly List<ILogger> loggers;
        private bool disposed = false;

        /// <summary>
        /// 构造函数
        /// </summary>
        public CompositeLogger()
        {
            loggers = new List<ILogger>();
        }

        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="loggers">日志器列表</param>
        public CompositeLogger(params ILogger[] loggers)
        {
            this.loggers = new List<ILogger>(loggers);
        }

        /// <summary>
        /// 添加日志器
        /// </summary>
        public void AddLogger(ILogger logger)
        {
            if (logger != null && !disposed)
            {
                loggers.Add(logger);
            }
        }

        /// <summary>
        /// 移除日志器
        /// </summary>
        public void RemoveLogger(ILogger logger)
        {
            if (logger != null && !disposed)
            {
                loggers.Remove(logger);
            }
        }

        public void Debug(string message) => Log(LogLevel.Debug, message);

        public void Info(string message) => Log(LogLevel.Info, message);

        public void Warning(string message) => Log(LogLevel.Warning, message);

        public void Error(string message) => Log(LogLevel.Error, message);

        public void Fatal(string message) => Log(LogLevel.Fatal, message);

        public void Error(string message, Exception exception)
        {
            var fullMessage = $"{message}\n异常详情: {exception}";
            Log(LogLevel.Error, fullMessage);
        }

        public void Log(LogLevel level, string message)
        {
            if (!IsEnabled(level) || disposed)
                return;

            foreach (var logger in loggers.ToList()) // ToList() 避免并发修改
            {
                try
                {
                    logger.Log(level, message);
                }
                catch (Exception ex)
                {
                    // 避免单个日志器的异常影响其他日志器
                    Console.WriteLine($"日志器异常: {ex.Message}");
                }
            }
        }

        public bool IsEnabled(LogLevel level) => level >= MinLevel && !disposed;

        public void Dispose()
        {
            if (!disposed)
            {
                foreach (var logger in loggers)
                {
                    if (logger is IDisposable disposableLogger)
                    {
                        try
                        {
                            disposableLogger.Dispose();
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"释放日志器时发生异常: {ex.Message}");
                        }
                    }
                }
                loggers.Clear();
                disposed = true;
            }
        }
    }
}