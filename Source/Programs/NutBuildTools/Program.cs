using System;
using System.CommandLine;
using System.Threading.Tasks;
using NutBuildTools.BuildSystem;
using NutBuildTools.Utils;

namespace NutBuildTools
{
    class Program
    {
        static async Task<int> Main(string[] args)
        {
            var targetOption = new Option<string>("--target", "-t")
            {
                Description = "编译目标（如 Editor/Server/Client）"
            };
            var platformOption = new Option<string>("--platform", "-p")
            {
                Description = "平台（如 Windows/Linux/Mac）"
            };
            var configOption = new Option<string>("--configuration", "-c")
            {
                DefaultValueFactory = _ => "Debug",
                Description = "配置（如Debug, Release）"
            };
            var logLevelOption = new Option<string>("--log-level", "-l")
            {
                DefaultValueFactory = _ => "Info",
                Description = "日志级别（Debug/Info/Warning/Error/Fatal）"
            };
            var logFileOption = new Option<string>("--log-file")
            {
                Description = "日志文件路径（可选）"
            };

            var command = new RootCommand("NutBuildTools 命令行编译工具")
            {
                targetOption,
                platformOption,
                configOption,
                logLevelOption,
                logFileOption
            };

            var buildCommand = new Command("build")
            {

            };

            command.Add(buildCommand);

            command.SetAction(async result =>
            {
                // 解析日志配置
                var logLevelStr = result.GetValue<string>("--log-level");
                var logFile = result.GetValue<string>("--log-file");

                if (!Enum.TryParse<LogLevel>(logLevelStr, true, out var logLevel))
                {
                    logLevel = LogLevel.Info;
                }

                // 配置日志
                Logger.Configure(
                    logFilePath: logFile ?? "logs/nutbuildtools.log",
                    enableFileLogging: !string.IsNullOrEmpty(logFile),
                    minLogLevel: logLevel
                );

                Logger.Info("NutBuildTools 启动");
                string target = result.GetRequiredValue<string>("--target");
                string platform = result.GetRequiredValue<string>("--platform");
                string configuration = result.GetRequiredValue<string>("--configuration");
                Logger.Info($"参数: target={target}, platform={platform}, configuration={configuration}");
                var builder = new NutBuilder();
                // 创建一个默认的可执行目标
                var targetInstance = new NutExecutableTarget()
                {
                    Name = target,
                    Platform = platform,
                    Configuration = configuration
                };
                await builder.BuildAsync(targetInstance);
                Logger.Info("NutBuildTools 运行结束");
            });

            return await command.Parse(args).InvokeAsync();
        }
    }
}
