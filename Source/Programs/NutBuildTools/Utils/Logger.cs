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
        private static bool? isXcodeEnvironment = null;

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

        private static bool IsXcodeEnvironment()
        {
            if (isXcodeEnvironment.HasValue)
                return isXcodeEnvironment.Value;

            // 检测 Xcode 环境变量 - 更严格的检测
            var srcRoot = Environment.GetEnvironmentVariable("SRCROOT");
            var xcodeApp = Environment.GetEnvironmentVariable("XCODE_VERSION_ACTUAL");
            var platformName = Environment.GetEnvironmentVariable("PLATFORM_NAME");
            var configuration = Environment.GetEnvironmentVariable("CONFIGURATION");
            var buildDir = Environment.GetEnvironmentVariable("BUILD_DIR");
            
            // 更准确的 Xcode 环境检测
            isXcodeEnvironment = (!string.IsNullOrEmpty(srcRoot) && 
                                 !string.IsNullOrEmpty(configuration)) ||
                                !string.IsNullOrEmpty(xcodeApp) ||
                                (!string.IsNullOrEmpty(platformName) && 
                                 !string.IsNullOrEmpty(buildDir));
            
            return isXcodeEnvironment.Value;
        }

        private static void WriteToConsole(string message, LogLevel level)
        {
            if (IsXcodeEnvironment())
            {
                // Xcode 环境：使用标准格式，避免颜色代码影响输出
                var prefix = level switch
                {
                    LogLevel.Debug => "🔍 DEBUG",
                    LogLevel.Info => "ℹ️  INFO ",
                    LogLevel.Warning => "⚠️  WARN ",
                    LogLevel.Error => "❌ ERROR",
                    LogLevel.Fatal => "💀 FATAL",
                    _ => "     "
                };
                
                var cleanMessage = message.Replace("[NutBuildTools] ", "");
                var outputLine = $"{prefix}: {cleanMessage}";
                
                // 直接写入标准输出流并立即刷新
                Console.Out.WriteLine(outputLine);
                Console.Out.Flush();
                
                // 对于错误和警告，同时输出到 stderr 以便 Xcode 正确识别
                if (level >= LogLevel.Warning)
                {
                    Console.Error.WriteLine(outputLine);
                    Console.Error.Flush();
                }
            }
            else
            {
                // 普通控制台环境：使用颜色
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
