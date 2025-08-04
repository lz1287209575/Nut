using System.CommandLine;
using NutBuildSystem.Logging;

namespace NutBuildSystem.CommandLine
{
    /// <summary>
    /// 通用命令行选项
    /// </summary>
    public static class CommonOptions
    {
        /// <summary>
        /// 日志级别选项
        /// </summary>
        public static Option<string> LogLevel { get; } = new("--log-level", "-l")
        {
            Description = "日志级别 (Debug/Info/Warning/Error/Fatal)",
            ArgumentHelpName = "level"
        };

        /// <summary>
        /// 日志文件选项
        /// </summary>
        public static Option<string?> LogFile { get; } = new("--log-file")
        {
            Description = "日志文件路径（可选）",
            ArgumentHelpName = "path"
        };

        /// <summary>
        /// 详细输出选项
        /// </summary>
        public static Option<bool> Verbose { get; } = new("--verbose", "-v")
        {
            Description = "显示详细输出"
        };

        /// <summary>
        /// 静默模式选项
        /// </summary>
        public static Option<bool> Quiet { get; } = new("--quiet", "-q")
        {
            Description = "静默模式，只显示错误信息"
        };

        /// <summary>
        /// 禁用颜色输出选项
        /// </summary>
        public static Option<bool> NoColor { get; } = new("--no-color")
        {
            Description = "禁用彩色输出"
        };

        /// <summary>
        /// 帮助选项
        /// </summary>
        public static Option<bool> Help { get; } = new("--help", "-h")
        {
            Description = "显示帮助信息"
        };

        /// <summary>
        /// 版本选项
        /// </summary>
        public static Option<bool> Version { get; } = new("--version")
        {
            Description = "显示版本信息"
        };
    }

    /// <summary>
    /// 构建相关的命令行选项
    /// </summary>
    public static class BuildOptions
    {
        /// <summary>
        /// 构建目标选项
        /// </summary>
        public static Option<string?> Target { get; } = new("--target", "-t")
        {
            Description = "构建目标名称",
            ArgumentHelpName = "target"
        };

        /// <summary>
        /// 平台选项
        /// </summary>
        public static Option<string?> Platform { get; } = new("--platform", "-p")
        {
            Description = "目标平台 (Windows/Linux/MacOS)",
            ArgumentHelpName = "platform"
        };

        /// <summary>
        /// 配置选项
        /// </summary>
        public static Option<string> Configuration { get; } = new("--configuration", "-c")
        {
            Description = "构建配置 (Debug/Release)",
            ArgumentHelpName = "config"
        };

        /// <summary>
        /// 并行构建选项
        /// </summary>
        public static Option<int> Jobs { get; } = new("--jobs", "-j")
        {
            Description = "并行构建任务数量",
            ArgumentHelpName = "count"
        };

        /// <summary>
        /// 清理构建选项
        /// </summary>
        public static Option<bool> Clean { get; } = new("--clean")
        {
            Description = "清理之前的构建输出"
        };

        /// <summary>
        /// 强制重建选项
        /// </summary>
        public static Option<bool> Rebuild { get; } = new("--rebuild")
        {
            Description = "强制重建所有目标"
        };
    }

    /// <summary>
    /// 头文件工具相关的命令行选项
    /// </summary>
    public static class HeaderToolOptions
    {
        /// <summary>
        /// 源路径选项
        /// </summary>
        public static Option<string[]> SourcePaths { get; } = new("--source", "-s")
        {
            Description = "要处理的源代码目录路径",
            ArgumentHelpName = "paths",
            AllowMultipleArgumentsPerToken = true
        };

        /// <summary>
        /// 使用Meta构建文件选项
        /// </summary>
        public static Option<bool> UseMeta { get; } = new("--meta", "-m")
        {
            Description = "使用Meta目录下的Build.cs文件来确定要处理的源文件"
        };

        /// <summary>
        /// 输出目录选项
        /// </summary>
        public static Option<string?> OutputDirectory { get; } = new("--output", "-o")
        {
            Description = "生成文件的输出目录",
            ArgumentHelpName = "directory"
        };

        /// <summary>
        /// 强制重新生成选项
        /// </summary>
        public static Option<bool> Force { get; } = new("--force", "-f")
        {
            Description = "强制重新生成所有文件"
        };
    }
}