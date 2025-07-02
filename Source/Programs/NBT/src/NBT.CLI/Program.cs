using System.CommandLine;
using NBT.Core.Services;

namespace NBT.CLI;

/// <summary>
/// 主程序类
/// </summary>
class Program
{
    /// <summary>
    /// 主入口点
    /// </summary>
    /// <param name="Args">命令行参数</param>
    /// <returns>退出代码</returns>
    static async Task<int> Main(string[] Args)
    {
        var RootCommand = new RootCommand("NBT - Nut Build Tools");

        // 添加构建命令
        var BuildCommand = new FBuildCommand(new BuildEngine(new FMetaLoader(), new FCompiler()));

        // 添加测试命令
        var TestCommand = new Command("test", "运行测试")
        {
            new Option<string>("--project")
            {
                Required = false,
                DefaultValueFactory = _ => ".",
                Description = "项目路径"
            }
        };
        TestCommand.SetAction(async Result =>
        {
            await HandleTestAsync(Result.GetRequiredValue<string>("--project"));
        });


        // 添加清理命令
        var CleanCommand = new Command("clean", "清理构建文件")
        {
            new Option<string>("--project")
            {
                Required = false,
                DefaultValueFactory = _ => ".",
                Description = "项目路径"
            }
        };

        CleanCommand.SetAction(async Result =>
        {
            await HandleCleanAsync(Result.GetRequiredValue<string>("--project"));
        });

        // 添加帮助命令
        var HelpCommand = new Command("help", "显示帮助信息");
        HelpCommand.SetAction(Result =>
        {
            Console.WriteLine("NBT - Nut Build Tools");
            Console.WriteLine("用法: nbt <command> [options]");
            Console.WriteLine();
            Console.WriteLine("命令:");
            Console.WriteLine("  build    构建项目");
            Console.WriteLine("  test     运行测试");
            Console.WriteLine("  clean    清理构建文件");
            Console.WriteLine("  help     显示帮助信息");
        });

        RootCommand.Add(BuildCommand);
        RootCommand.Add(TestCommand);
        RootCommand.Add(CleanCommand);
        RootCommand.Add(HelpCommand);

        return await RootCommand.Parse(Args).InvokeAsync();
    }

    /// <summary>
    /// 处理构建命令
    /// </summary>
    /// <param name="ProjectPath">项目路径</param>
    static async Task HandleBuildAsync(string ProjectPath)
    {
        try
        {
            Console.WriteLine($"开始构建项目: {ProjectPath}");

            // 创建服务实例
            var MetaLoader = new FMetaLoader();
            var Compiler = new FCompiler(new FBuildConfig());
            var BuildEngine = new BuildEngine(MetaLoader, Compiler);

            var BuildOptions = new FBuildOptions
            {
                ProjectPath = ProjectPath,
                Configuration = "Debug",
                bClean = false,
                bParallel = true,
                bVerbose = true
            };

            var Result = await BuildEngine.BuildAsync(BuildOptions);

            if (Result.bSuccess)
            {
                Console.WriteLine("构建成功！");
            }
            else
            {
                Console.WriteLine($"构建失败: {Result.ErrorMessage}");
                Environment.Exit(1);
            }
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"构建过程中发生错误: {Ex.Message}");
            Environment.Exit(1);
        }
    }

    /// <summary>
    /// 处理测试命令
    /// </summary>
    /// <param name="ProjectPath">项目路径</param>
    static async Task HandleTestAsync(string ProjectPath)
    {
        try
        {
            Console.WriteLine($"开始运行测试: {ProjectPath}");
            Console.WriteLine("测试功能尚未实现");
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"测试过程中发生错误: {Ex.Message}");
            Environment.Exit(1);
        }
    }

    /// <summary>
    /// 处理清理命令
    /// </summary>
    /// <param name="ProjectPath">项目路径</param>
    static async Task HandleCleanAsync(string ProjectPath)
    {
        try
        {
            Console.WriteLine($"开始清理项目: {ProjectPath}");
            Console.WriteLine("清理功能尚未实现");
        }
        catch (Exception Ex)
        {
            Console.WriteLine($"清理过程中发生错误: {Ex.Message}");
            Environment.Exit(1);
        }
    }
}