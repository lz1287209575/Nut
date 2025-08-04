namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 日志接口
    /// </summary>
    public interface ILogger
    {
        /// <summary>
        /// 当前最小日志级别
        /// </summary>
        LogLevel MinLevel { get; set; }

        /// <summary>
        /// 记录调试信息
        /// </summary>
        void Debug(string message);

        /// <summary>
        /// 记录信息
        /// </summary>
        void Info(string message);

        /// <summary>
        /// 记录警告
        /// </summary>
        void Warning(string message);

        /// <summary>
        /// 记录错误
        /// </summary>
        void Error(string message);

        /// <summary>
        /// 记录致命错误
        /// </summary>
        void Fatal(string message);

        /// <summary>
        /// 记录异常
        /// </summary>
        void Error(string message, Exception exception);

        /// <summary>
        /// 记录指定级别的日志
        /// </summary>
        void Log(LogLevel level, string message);

        /// <summary>
        /// 判断指定级别是否启用
        /// </summary>
        bool IsEnabled(LogLevel level);
    }
}