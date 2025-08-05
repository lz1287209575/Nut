using System.CommandLine;
using System.CommandLine.Builder;
using System.CommandLine.Parsing;
using System.CommandLine.Invocation;
using System.Reflection;
using System.Linq;

namespace NutBuildSystem.CommandLine
{
    /// <summary>
    /// 命令行构建器 - 提供统一的命令行应用构建功能
    /// </summary>
    public class CommandLineBuilder
    {
        private readonly RootCommand rootCommand;
        private readonly List<Command> commands;
        private readonly List<Option> globalOptions;

        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="description">应用程序描述</param>
        public CommandLineBuilder(string description)
        {
            rootCommand = new RootCommand(description);
            commands = new List<Command>();
            globalOptions = new List<Option>();
        }

        /// <summary>
        /// 添加全局选项
        /// </summary>
        public CommandLineBuilder AddGlobalOption(Option option)
        {
            globalOptions.Add(option);
            rootCommand.AddGlobalOption(option);
            return this;
        }

        /// <summary>
        /// 添加常用的全局选项
        /// </summary>
        public CommandLineBuilder AddCommonGlobalOptions()
        {
            return AddGlobalOption(CommonOptions.LogLevel)
                .AddGlobalOption(CommonOptions.LogFile)
                .AddGlobalOption(CommonOptions.Verbose)
                .AddGlobalOption(CommonOptions.Quiet)
                .AddGlobalOption(CommonOptions.NoColor);
        }

        /// <summary>
        /// 添加命令
        /// </summary>
        public CommandLineBuilder AddCommand(Command command)
        {
            commands.Add(command);
            rootCommand.AddCommand(command);
            return this;
        }

        /// <summary>
        /// 添加子命令
        /// </summary>
        public CommandLineBuilder AddSubCommand(string name, string description, Action<Command> configure)
        {
            var command = new Command(name, description);
            configure(command);
            return AddCommand(command);
        }

        /// <summary>
        /// 设置默认处理程序
        /// </summary>
        public CommandLineBuilder SetDefaultHandler(Func<CommandContext, Task<int>> handler)
        {
            rootCommand.SetHandler(async (InvocationContext context) =>
            {
                var commandContext = CreateCommandContext(context);
                try
                {
                    var result = await handler(commandContext);
                    context.ExitCode = result;
                }
                finally
                {
                    commandContext.Dispose();
                }
            });
            return this;
        }

        /// <summary>
        /// 为指定的子命令设置处理程序
        /// </summary>
        /// <param name="commandName">子命令名称</param>
        /// <param name="handler">处理程序</param>
        public CommandLineBuilder SetHandlerForSubCommand(string commandName, Func<CommandContext, Task<int>> handler)
        {
            var command = commands.FirstOrDefault(c => c.Name == commandName);
            if (command == null)
            {
                throw new InvalidOperationException($"Command '{commandName}' not found. Make sure to add the command before setting its handler.");
            }

            command.SetHandler(async (InvocationContext context) =>
            {
                var commandContext = CreateCommandContext(context);
                try
                {
                    var result = await handler(commandContext);
                    context.ExitCode = result;
                }
                finally
                {
                    commandContext.Dispose();
                }
            });

            return this;
        }

        /// <summary>
        /// 构建命令行解析器
        /// </summary>
        public Parser Build()
        {
            return new System.CommandLine.Builder.CommandLineBuilder(rootCommand)
                .UseDefaults()
                .UseExceptionHandler((exception, context) =>
                {
                    context.Console.WriteLine($"❌ 发生未处理的异常: {exception.Message}");
                    if (exception.InnerException != null)
                    {
                        context.Console.WriteLine($"内部异常: {exception.InnerException.Message}");
                    }
                    context.ExitCode = -1;
                })
                .Build();
        }

        /// <summary>
        /// 从解析结果创建命令上下文
        /// </summary>
        private static CommandContext CreateCommandContext(InvocationContext context)
        {
            var parseResult = context.ParseResult;
            var commandContext = new CommandContext();

            // 解析通用选项
            var logLevel = parseResult.GetValueForOption(CommonOptions.LogLevel);
            var logFile = parseResult.GetValueForOption(CommonOptions.LogFile);
            var verbose = parseResult.GetValueForOption(CommonOptions.Verbose);
            var quiet = parseResult.GetValueForOption(CommonOptions.Quiet);
            var noColor = parseResult.GetValueForOption(CommonOptions.NoColor);

            // 设置命令上下文
            commandContext.ParseResult = parseResult;
            commandContext.IsVerbose = verbose;
            commandContext.IsQuiet = quiet;
            commandContext.NoColor = noColor;
            commandContext.CancellationToken = context.GetCancellationToken();

            // 设置日志
            commandContext.SetupLogging(logFile, logLevel);

            return commandContext;
        }
    }

    /// <summary>
    /// 命令行应用程序基类
    /// </summary>
    public abstract class CommandLineApplication
    {
        protected readonly CommandLineBuilder builder;

        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="description">应用程序描述</param>
        protected CommandLineApplication(string description)
        {
            builder = new CommandLineBuilder(description);
        }

        /// <summary>
        /// 配置命令行
        /// </summary>
        protected abstract void ConfigureCommands();

        /// <summary>
        /// 运行应用程序
        /// </summary>
        /// <param name="args">命令行参数</param>
        /// <returns>退出代码</returns>
        public async Task<int> RunAsync(string[] args)
        {
            try
            {
                ConfigureCommands();
                var parser = builder.Build();
                return await parser.InvokeAsync(args);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"❌ 应用程序启动失败: {ex.Message}");
                return -1;
            }
        }

        /// <summary>
        /// 显示版本信息
        /// </summary>
        protected virtual void ShowVersion()
        {
            var assembly = System.Reflection.Assembly.GetEntryAssembly();
            var version = assembly?.GetName().Version?.ToString() ?? "未知版本";
            var titleAttribute = assembly?.GetCustomAttributes<AssemblyTitleAttribute>().FirstOrDefault();
            var title = titleAttribute?.Title ?? "Nut Tool";
            
            Console.WriteLine($"{title} v{version}");
        }
    }
}