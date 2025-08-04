namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 日志工厂 - 统一管理日志器的创建和配置
    /// </summary>
    public static class LoggerFactory
    {
        private static ILogger? defaultLogger;

        /// <summary>
        /// 默认日志器
        /// </summary>
        public static ILogger Default
        {
            get
            {
                if (defaultLogger == null)
                {
                    defaultLogger = new ConsoleLogger();
                }
                return defaultLogger;
            }
            set => defaultLogger = value;
        }

        /// <summary>
        /// 创建控制台日志器
        /// </summary>
        /// <param name="minLevel">最小日志级别</param>
        /// <param name="useColors">是否使用颜色</param>
        /// <returns>控制台日志器</returns>
        public static ConsoleLogger CreateConsoleLogger(LogLevel minLevel = LogLevel.Info, bool useColors = true)
        {
            return new ConsoleLogger(useColors)
            {
                MinLevel = minLevel
            };
        }

        /// <summary>
        /// 创建文件日志器
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <param name="minLevel">最小日志级别</param>
        /// <returns>文件日志器</returns>
        public static FileLogger CreateFileLogger(string filePath, LogLevel minLevel = LogLevel.Debug)
        {
            return new FileLogger(filePath)
            {
                MinLevel = minLevel
            };
        }

        /// <summary>
        /// 创建复合日志器（控制台 + 文件）
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <param name="consoleLevel">控制台日志级别</param>
        /// <param name="fileLevel">文件日志级别</param>
        /// <param name="useColors">控制台是否使用颜色</param>
        /// <returns>复合日志器</returns>
        public static CompositeLogger CreateCompositeLogger(
            string? filePath = null,
            LogLevel consoleLevel = LogLevel.Info,
            LogLevel fileLevel = LogLevel.Debug,
            bool useColors = true)
        {
            var composite = new CompositeLogger();
            
            // 添加控制台日志器
            composite.AddLogger(CreateConsoleLogger(consoleLevel, useColors));
            
            // 如果指定了文件路径，添加文件日志器
            if (!string.IsNullOrEmpty(filePath))
            {
                composite.AddLogger(CreateFileLogger(filePath, fileLevel));
            }

            return composite;
        }

        /// <summary>
        /// 解析日志级别字符串
        /// </summary>
        /// <param name="levelString">日志级别字符串</param>
        /// <param name="defaultLevel">默认级别</param>
        /// <returns>日志级别</returns>
        public static LogLevel ParseLogLevel(string levelString, LogLevel defaultLevel = LogLevel.Info)
        {
            if (string.IsNullOrEmpty(levelString))
                return defaultLevel;

            return levelString.ToUpperInvariant() switch
            {
                "DEBUG" or "DBG" => LogLevel.Debug,
                "INFO" or "INF" => LogLevel.Info,
                "WARNING" or "WARN" or "WRN" => LogLevel.Warning,
                "ERROR" or "ERR" => LogLevel.Error,
                "FATAL" or "FAT" => LogLevel.Fatal,
                _ => defaultLevel
            };
        }

        /// <summary>
        /// 释放默认日志器资源
        /// </summary>
        public static void DisposeDefault()
        {
            if (defaultLogger is IDisposable disposable)
            {
                disposable.Dispose();
            }
            defaultLogger = null;
        }
    }
}