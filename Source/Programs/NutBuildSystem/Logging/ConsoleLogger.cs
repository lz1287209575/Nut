namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 控制台日志实现
    /// </summary>
    public class ConsoleLogger : ILogger
    {
        public LogLevel MinLevel { get; set; } = LogLevel.Info;

        private readonly bool useColors;
        private readonly object lockObject = new object();

        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="useColors">是否使用颜色输出</param>
        public ConsoleLogger(bool useColors = true)
        {
            this.useColors = useColors;
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
            if (!IsEnabled(level))
                return;

            lock (lockObject)
            {
                var timestamp = DateTime.Now.ToString("HH:mm:ss.fff");
                var levelText = GetLevelText(level);
                var formattedMessage = $"[{timestamp}] {levelText} {message}";

                if (useColors)
                {
                    WriteColoredMessage(level, formattedMessage);
                }
                else
                {
                    Console.WriteLine(formattedMessage);
                }
            }
        }

        public bool IsEnabled(LogLevel level) => level >= MinLevel;

        private string GetLevelText(LogLevel level)
        {
            return level switch
            {
                LogLevel.Debug => "[DBG]",
                LogLevel.Info => "[INF]",
                LogLevel.Warning => "[WRN]",
                LogLevel.Error => "[ERR]",
                LogLevel.Fatal => "[FAT]",
                _ => "[???]"
            };
        }

        private void WriteColoredMessage(LogLevel level, string message)
        {
            var originalColor = Console.ForegroundColor;
            
            var color = level switch
            {
                LogLevel.Debug => ConsoleColor.Gray,
                LogLevel.Info => ConsoleColor.White,
                LogLevel.Warning => ConsoleColor.Yellow,
                LogLevel.Error => ConsoleColor.Red,
                LogLevel.Fatal => ConsoleColor.Magenta,
                _ => ConsoleColor.White
            };

            Console.ForegroundColor = color;
            Console.WriteLine(message);
            Console.ForegroundColor = originalColor;
        }
    }
}