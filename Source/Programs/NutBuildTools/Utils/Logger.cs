using System;
using System.IO;

namespace NutBuildTools.Utils
{
    public enum LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    }

    public static class Logger
    {
        private static readonly object lockObject = new object();
        private static string? logFilePath;
        private static bool enableFileLogging = false;
        private static LogLevel minLogLevel = LogLevel.Info;

        public static void Configure(string? logFilePath = null, bool enableFileLogging = false, LogLevel minLogLevel = LogLevel.Info)
        {
            Logger.logFilePath = logFilePath;
            Logger.enableFileLogging = enableFileLogging;
            Logger.minLogLevel = minLogLevel;
        }

        public static void Log(string message, LogLevel level = LogLevel.Info)
        {
            if (level < minLogLevel)
                return;

            var timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");
            var logMessage = $"[{timestamp}] [{level}] {message}";
            var coloredMessage = $"[NutBuildTools] {message}";

            lock (lockObject)
            {
                // 控制台输出（带颜色）
                WriteToConsole(coloredMessage, level);

                // 文件输出（无颜色）
                if (enableFileLogging && !string.IsNullOrEmpty(logFilePath))
                {
                    WriteToFile(logMessage);
                }
            }
        }

        public static void Debug(string message) => Log(message, LogLevel.Debug);
        public static void Info(string message) => Log(message, LogLevel.Info);
        public static void Warning(string message) => Log(message, LogLevel.Warning);
        public static void Error(string message) => Log(message, LogLevel.Error);
        public static void Fatal(string message) => Log(message, LogLevel.Fatal);

        private static void WriteToConsole(string message, LogLevel level)
        {
            var originalColor = Console.ForegroundColor;

            switch (level)
            {
                case LogLevel.Debug:
                    Console.ForegroundColor = ConsoleColor.Gray;
                    break;
                case LogLevel.Info:
                    Console.ForegroundColor = ConsoleColor.White;
                    break;
                case LogLevel.Warning:
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    break;
                case LogLevel.Error:
                    Console.ForegroundColor = ConsoleColor.Red;
                    break;
                case LogLevel.Fatal:
                    Console.ForegroundColor = ConsoleColor.DarkRed;
                    break;
            }

            Console.WriteLine(message);
            Console.ForegroundColor = originalColor;
        }

        private static void WriteToFile(string message)
        {
            try
            {
                var directory = Path.GetDirectoryName(logFilePath);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                File.AppendAllText(logFilePath, message + Environment.NewLine);
            }
            catch (Exception ex)
            {
                // 如果文件写入失败，输出到控制台但不影响主流程
                var originalColor = Console.ForegroundColor;
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"[NutBuildTools] 日志文件写入失败: {ex.Message}");
                Console.ForegroundColor = originalColor;
            }
        }
    }
}
