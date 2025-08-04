namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 文件日志实现
    /// </summary>
    public class FileLogger : ILogger, IDisposable
    {
        public LogLevel MinLevel { get; set; } = LogLevel.Debug;

        private readonly string filePath;
        private readonly StreamWriter writer;
        private readonly object lockObject = new object();
        private bool disposed = false;

        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="filePath">日志文件路径</param>
        public FileLogger(string filePath)
        {
            this.filePath = filePath;
            
            // 确保目录存在
            var directory = Path.GetDirectoryName(filePath);
            if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            writer = new StreamWriter(filePath, append: true)
            {
                AutoFlush = true
            };
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

            lock (lockObject)
            {
                if (disposed) return;

                var timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");
                var levelText = GetLevelText(level);
                var formattedMessage = $"[{timestamp}] {levelText} {message}";

                writer.WriteLine(formattedMessage);
            }
        }

        public bool IsEnabled(LogLevel level) => level >= MinLevel && !disposed;

        private string GetLevelText(LogLevel level)
        {
            return level switch
            {
                LogLevel.Debug => "[DEBUG]",
                LogLevel.Info => "[INFO ]",
                LogLevel.Warning => "[WARN ]",
                LogLevel.Error => "[ERROR]",
                LogLevel.Fatal => "[FATAL]",
                _ => "[UNKNW]"
            };
        }

        public void Dispose()
        {
            if (!disposed)
            {
                lock (lockObject)
                {
                    if (!disposed)
                    {
                        writer?.Dispose();
                        disposed = true;
                    }
                }
            }
        }
    }
}