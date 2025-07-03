using System.CommandLine;
using System.CommandLine.Invocation;
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

            var command = new RootCommand("NutBuildTools 命令行编译工具")
            {
                targetOption,
                platformOption,
                configOption
            };

            command.SetAction(async result =>
            {
                string target = result.GetRequiredValue<string>("--target");
                string platform = result.GetRequiredValue<string>("--platform");
                string configuration = result.GetRequiredValue<string>("--configuration");

                var builder = new NutBuilder();
                await builder.BuildAsync(new NutTarget()
                {
                    Target = target,
                    Platform = platform,
                    Configuration = configuration
                });
            });

            return await command.Parse(args).InvokeAsync();
        }
    }
}