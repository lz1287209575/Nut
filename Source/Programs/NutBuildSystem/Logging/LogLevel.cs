namespace NutBuildSystem.Logging
{
    /// <summary>
    /// 日志级别枚举
    /// </summary>
    public enum LogLevel
    {
        /// <summary>
        /// 调试信息 - 最详细的日志信息
        /// </summary>
        Debug = 0,

        /// <summary>
        /// 信息 - 一般的程序执行信息
        /// </summary>
        Info = 1,

        /// <summary>
        /// 警告 - 潜在的问题，但不影响程序继续执行
        /// </summary>
        Warning = 2,

        /// <summary>
        /// 错误 - 影响程序正常执行的问题
        /// </summary>
        Error = 3,

        /// <summary>
        /// 致命错误 - 导致程序无法继续执行的严重问题
        /// </summary>
        Fatal = 4
    }
}