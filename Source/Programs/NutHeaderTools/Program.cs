using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.CommandLine;
using NutBuildSystem.Discovery;
using NutBuildSystem.BuildTargets;
using NutBuildSystem.IO;
using NutBuildSystem.Logging;
using NutBuildSystem.CommandLine;
using NutHeaderTools.Parsers;
using NutHeaderTools.Generators;
using NutHeaderTools.Validators;

namespace NutHeaderTools
{
    /// <summary>
    /// NutHeaderTools - Nut引擎的代码生成工具
    /// 类似UE的UnrealHeaderTool，用于解析NCLASS宏并生成.generate.h文件
    /// 使用nprx项目文件和Meta文件来确定要处理的源文件
    /// </summary>
    class NutHeaderToolsApp : CommandLineApplication
    {
        private static readonly Dictionary<string, HeaderInfo> ProcessedHeaders = new();
        private readonly List<ModuleInfo> modules = new();
        private int generatedFileCount = 0;
        private ILogger logger = LoggerFactory.Default;
        private string projectRoot = string.Empty;
        private readonly AdvancedMacroParser macroParser;
        private readonly ReflectionCodeGenerator codeGenerator;
        private readonly ReflectionValidator validator;

        public NutHeaderToolsApp() : base("NutHeaderTools - Nut Engine Code Generator")
        {
            macroParser = new AdvancedMacroParser(logger);
            codeGenerator = new ReflectionCodeGenerator(logger);
            validator = new ReflectionValidator(logger);
        }

        static async Task<int> Main(string[] args)
        {
            var app = new NutHeaderToolsApp();
            return await app.RunAsync(args);
        }

        protected override void ConfigureCommands()
        {
            builder.AddCommonGlobalOptions();
            
            // 使用与 NutBuildTools 相同的参数结构
            builder.AddGlobalOption(BuildOptions.Target);
            builder.AddGlobalOption(HeaderToolOptions.OutputDirectory);
            builder.AddGlobalOption(HeaderToolOptions.Force);

            builder.SetDefaultHandler(ExecuteAsync);
        }

        private async Task<int> ExecuteAsync(CommandContext context)
        {
            logger = context.Logger;
            
            logger.Info("NutHeaderTools v1.0 - Nut Engine Code Generator");
            logger.Info("=====================================");

            try
            {
                // 从 context 中获取命令行参数
                var target = context.ParseResult.GetValueForOption(BuildOptions.Target);
                var outputDirectory = context.ParseResult.GetValueForOption(HeaderToolOptions.OutputDirectory);
                var force = context.ParseResult.GetValueForOption(HeaderToolOptions.Force);

                logger.Info($"配置选项:");
                logger.Info($"  目标模块: {target ?? "全部"}");
                logger.Info($"  强制重新生成: {force}");
                if (!string.IsNullOrEmpty(outputDirectory))
                {
                    logger.Info($"  输出目录: {outputDirectory}");
                }
                
                await ProcessAllHeadersAsync(target, outputDirectory, force);
                logger.Info($"✓ 成功生成 {generatedFileCount} 个 .generate.h 文件");
                return 0;
            }
            catch (Exception ex)
            {
                logger.Error($"错误: {ex.Message}", ex);
                return 1;
            }
        }

        private async Task ProcessAllHeadersAsync(string? targetName, string? outputDirectory, bool force)
        {
            // 定位项目根目录
            projectRoot = FindProjectRootFromNprx();
            if (string.IsNullOrEmpty(projectRoot))
            {
                logger.Error("无法找到.nprx项目文件，请在项目根目录运行此工具");
                throw new InvalidOperationException("No .nprx project file found");
            }

            logger.Info($"📁 项目根目录: {projectRoot}");
            logger.Info("🔍 发现模块和Meta文件...");
            
            // 使用新的模块发现逻辑
            await DiscoverModulesAsync();
            
            // 根据 target 参数过滤模块
            var targetModules = FilterTargetModules(targetName);
            
            if (targetModules.Count == 0)
            {
                if (!string.IsNullOrEmpty(targetName))
                {
                    logger.Error($"未找到目标模块: {targetName}");
                    throw new InvalidOperationException($"Target module not found: {targetName}");
                }
                else
                {
                    logger.Warning("没有找到任何模块需要处理");
                    return;
                }
            }
            
            logger.Info($"找到 {targetModules.Count} 个模块需要处理:");
            foreach (var module in targetModules)
            {
                logger.Info($"  • {module.Name} ({module.Type})");
            }
            
            ProcessModules(targetModules, outputDirectory, force);
        }

        private void ProcessDirectory(string directory)
        {
            // 处理.h和.hpp文件
            string[] headerFiles = Directory.GetFiles(directory, "*.h", SearchOption.AllDirectories)
                .Concat(Directory.GetFiles(directory, "*.hpp", SearchOption.AllDirectories))
                .ToArray();

            foreach (string headerFile in headerFiles)
            {
                ProcessHeaderFile(headerFile);
            }
        }

        private void ProcessHeaderFile(string headerPath)
        {
            try
            {
                string content = File.ReadAllText(headerPath);
                logger.Debug($"  🔍 处理文件: {Path.GetFileName(headerPath)}");

                HeaderInfo headerInfo = ParseHeader(headerPath, content);

                if (headerInfo.HasNClassMarkedClasses)
                {
                    string generateFilePath = GetGenerateFilePath(headerPath);
                    GenerateCodeFile(generateFilePath, headerInfo);
                    ProcessedHeaders[headerPath] = headerInfo;
                    generatedFileCount++;

                    logger.Info($"  ✓ 已生成: {Path.GetFileName(generateFilePath)}");
                }
                else
                {
                    logger.Debug($"  - 跳过: {Path.GetFileName(headerPath)} (无NCLASS标记)");
                }
            }
            catch (Exception ex)
            {
                logger.Error($"  处理文件失败 {Path.GetFileName(headerPath)}: {ex.Message}", ex);
            }
        }

        private HeaderInfo ParseHeader(string filePath, string content)
        {
            HeaderInfo info = new HeaderInfo
            {
                FilePath = filePath,
                FileName = Path.GetFileNameWithoutExtension(filePath)
            };

            // 先检查是否包含NCLASS
            if (!content.Contains("NCLASS"))
            {
                return info;
            }

            logger.Debug($"    文件包含NCLASS");

            // 使用高级解析器解析类
            var classMatches = ParseClassesWithAdvancedParser(content);
            
            foreach (var classMatch in classMatches)
            {
                logger.Debug($"    解析类 {classMatch.Name} : {classMatch.BaseClass}");

                // 验证类信息
                var validationResult = validator.ValidateClass(classMatch);
                if (!validationResult.IsValid)
                {
                    logger.Warning($"    类 {classMatch.Name} 验证失败:");
                    logger.Warning(validationResult.ToString());
                }

                // 解析NPROPERTY标记的属性
                ParsePropertiesWithAdvancedParser(content, classMatch);

                // 解析NFUNCTION标记的函数
                ParseFunctionsWithAdvancedParser(content, classMatch);

                info.Classes.Add(classMatch);
            }

            return info;
        }

        private List<ClassInfo> ParseClassesWithAdvancedParser(string content)
        {
            var classes = new List<ClassInfo>();
            
            // 使用改进的正则表达式
            string[] patterns = {
                @"NCLASS\s*\([^)]*\)\s*\r?\n\s*class\s+(?:\w+\s+)?(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)\s*class\s+(?:\w+\s+)?(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)[\s\r\n]*class\s+(?:\w+\s+)?(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)[\s\r\n]*class\s+(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)[\s\r\n]+class\s+(\w+)\s*:\s*public\s+(\w+)",
                @"NCLASS.*?class\s+(\w+)\s*:\s*public\s+(\w+)",
                @"NCLASS.*?class\s+(\w+)"
            };

            foreach (string pattern in patterns)
            {
                var matches = Regex.Matches(content, pattern, RegexOptions.Multiline | RegexOptions.Singleline);
                logger.Debug($"    模式 '{pattern}' 找到 {matches.Count} 个匹配");
                
                foreach (Match match in matches)
                {
                    string className = match.Groups[1].Value;
                    string baseClass = match.Groups[2].Success ? match.Groups[2].Value : "NObject";
                    
                    // 验证类名
                    if (macroParser.IsValidFunctionName(className))
                    {
                        classes.Add(new ClassInfo
                        {
                            Name = className,
                            BaseClass = baseClass
                        });
                    }
                }
                
                if (classes.Count > 0) break;
            }

            return classes;
        }

        private void ParsePropertiesWithAdvancedParser(string content, ClassInfo classInfo)
        {
            // 使用更精确的正则表达式，处理各种类型声明格式
            string[] propertyPatterns = {
                // 标准格式：NPROPERTY(...) Type Name;
                @"NPROPERTY\s*\([^)]*\)[\s\r\n]*([A-Za-z_]\w*(?:<[^<>]*>)?(?:::\w+)*)\s+([A-Za-z_]\w*)\s*(?:=\s*[^;]*)?\s*;",
                // 带模板的格式：NPROPERTY(...) TTemplate<Type> Name;
                @"NPROPERTY\s*\([^)]*\)[\s\r\n]*([A-Za-z_]\w*<[^<>]*>)\s+([A-Za-z_]\w*)\s*(?:=\s*[^;]*)?\s*;",
                // 命名空间类型：NPROPERTY(...) Namespace::Type Name;
                @"NPROPERTY\s*\([^)]*\)[\s\r\n]*([A-Za-z_]\w*::[A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*(?:=\s*[^;]*)?\s*;"
            };

            HashSet<string> addedProperties = new HashSet<string>();

            foreach (string pattern in propertyPatterns)
            {
                MatchCollection propertyMatches = Regex.Matches(content, pattern, RegexOptions.Multiline | RegexOptions.IgnoreCase);

                foreach (Match match in propertyMatches)
                {
                    string type = match.Groups[1].Value.Trim();
                    string name = match.Groups[2].Value.Trim();

                    // 使用高级解析器验证类型和名称
                    if (!addedProperties.Contains(name) && 
                        !string.IsNullOrEmpty(type) && 
                        !string.IsNullOrEmpty(name) &&
                        macroParser.IsValidFunctionName(name) &&
                        macroParser.IsValidTypeName(type))
                    {
                        // 标准化类型名称
                        var normalizedType = macroParser.NormalizeTypeName(type);
                        
                        PropertyInfo property = new PropertyInfo
                        {
                            Type = normalizedType,
                            Name = name
                        };
                        classInfo.Properties.Add(property);
                        addedProperties.Add(name);
                        logger.Debug($"    找到属性 {property.Type} {property.Name}");
                    }
                }
            }
        }

        private void ParseFunctionsWithAdvancedParser(string content, ClassInfo classInfo)
        {
            // 更灵活的函数解析 - 查找NFUNCTION()或NMETHOD()标记的函数
            string[] functionPatterns = {
                // 标准函数：NFUNCTION(...) ReturnType FunctionName(...);
                @"(?:NFUNCTION|NMETHOD)\s*\([^)]*\)[\s\r\n]*([A-Za-z_]\w*(?:<[^<>]*>)?(?:::\w+)*)\s+([A-Za-z_]\w*)\s*\([^)]*\)\s*(?:const)?\s*(?:override)?\s*;",
                // 虚函数：NFUNCTION(...) virtual ReturnType FunctionName(...) = 0;
                @"(?:NFUNCTION|NMETHOD)\s*\([^)]*\)[\s\r\n]*virtual\s+([A-Za-z_]\w*(?:<[^<>]*>)?(?:::\w+)*)\s+([A-Za-z_]\w*)\s*\([^)]*\)\s*(?:const)?\s*(?:override)?\s*(?:=\s*0)?\s*;",
                // 静态函数：NFUNCTION(...) static ReturnType FunctionName(...);
                @"(?:NFUNCTION|NMETHOD)\s*\([^)]*\)[\s\r\n]*static\s+([A-Za-z_]\w*(?:<[^<>]*>)?(?:::\w+)*)\s+([A-Za-z_]\w*)\s*\([^)]*\)\s*;"
            };

            HashSet<string> addedFunctions = new HashSet<string>();

            foreach (string pattern in functionPatterns)
            {
                MatchCollection functionMatches = Regex.Matches(content, pattern, RegexOptions.Multiline | RegexOptions.IgnoreCase);

                foreach (Match match in functionMatches)
                {
                    string returnType = match.Groups[1].Value.Trim();
                    string name = match.Groups[2].Value.Trim();

                    if (!addedFunctions.Contains(name) && 
                        !string.IsNullOrEmpty(returnType) && 
                        !string.IsNullOrEmpty(name) &&
                        macroParser.IsValidFunctionName(name) &&
                        macroParser.IsValidTypeName(returnType))
                    {
                        // 标准化返回类型
                        var normalizedReturnType = macroParser.NormalizeTypeName(returnType);
                        
                        FunctionInfo function = new FunctionInfo
                        {
                            ReturnType = normalizedReturnType,
                            Name = name
                        };
                        classInfo.Functions.Add(function);
                        addedFunctions.Add(name);
                        logger.Debug($"    找到函数 {function.ReturnType} {function.Name}()");
                    }
                }
            }
        }

        /// <summary>
        /// 通过nprx文件定位项目根目录
        /// </summary>
        private string FindProjectRootFromNprx()
        {
            return NutProjectReader.FindProjectFile(Environment.CurrentDirectory) != null 
                ? Path.GetDirectoryName(NutProjectReader.FindProjectFile(Environment.CurrentDirectory)) ?? Environment.CurrentDirectory
                : string.Empty;
        }

        /// <summary>
        /// 发现项目中的所有模块
        /// </summary>
        private async Task DiscoverModulesAsync()
        {
            try
            {
                var discoveredModules = await ModuleDiscovery.DiscoverModulesAsync(projectRoot, logger);
                modules.AddRange(discoveredModules);
                
                logger.Info($"找到 {modules.Count} 个模块:");
                foreach (var module in modules)
                {
                    string metaStatus = !string.IsNullOrEmpty(module.MetaFilePath) ? "✓" : "✗";
                    logger.Info($"  {metaStatus} {module.Name} ({module.Type}) - {Path.GetRelativePath(projectRoot, module.ModulePath)}");
                    if (!string.IsNullOrEmpty(module.MetaFilePath))
                    {
                        logger.Debug($"    Meta文件: {Path.GetRelativePath(projectRoot, module.MetaFilePath)}");
                    }
                }
            }
            catch (Exception ex)
            {
                logger.Error($"发现模块失败: {ex.Message}", ex);
                throw;
            }
        }

        /// <summary>
        /// 过滤要处理的目标模块
        /// </summary>
        private List<ModuleInfo> FilterTargetModules(string? targetName)
        {
            if (string.IsNullOrEmpty(targetName))
            {
                // 如果没有指定目标，处理所有有Meta文件的模块
                return modules.Where(m => !string.IsNullOrEmpty(m.MetaFilePath)).ToList();
            }

            // 查找指定名称的模块
            var targetModule = modules.FirstOrDefault(m =>
                string.Equals(m.Name, targetName, StringComparison.OrdinalIgnoreCase));

            if (targetModule != null)
            {
                return new List<ModuleInfo> { targetModule };
            }

            return new List<ModuleInfo>();
        }

        /// <summary>
        /// 处理指定的模块
        /// </summary>
        private void ProcessModules(List<ModuleInfo> targetModules, string? outputDirectory, bool force)
        {
            foreach (var module in targetModules)
            {
                logger.Info($"🔍 处理模块: {module.Name}");

                // 检查是否有Meta文件
                if (string.IsNullOrEmpty(module.MetaFilePath))
                {
                    logger.Warning($"    模块 {module.Name} 没有Meta文件，跳过处理");
                    continue;
                }

                logger.Info($"    Meta文件: {Path.GetRelativePath(projectRoot, module.MetaFilePath)}");

                // 根据Meta文件中的配置处理源文件
                ProcessModuleWithMeta(module, outputDirectory, force);
            }
        }

        /// <summary>
        /// 根据Meta文件配置处理模块
        /// </summary>
        private void ProcessModuleWithMeta(ModuleInfo module, string? outputDirectory, bool force)
        {
            try
            {
                if (Directory.Exists(module.SourcesPath))
                {
                    logger.Debug($"    扫描源目录: {module.SourcesPath}");
                    ProcessDirectory(module.SourcesPath);
                }
                else
                {
                    logger.Warning($"    源目录不存在: {module.SourcesPath}");
                }

                // 如果模块有BuildTarget，可以进一步根据其Sources配置处理
                if (module.BuildTarget != null && module.BuildTarget.Sources != null)
                {
                    logger.Debug($"    根据Meta文件中的Sources配置处理文件");
                    foreach (var source in module.BuildTarget.Sources)
                    {
                        string fullPath = Path.IsPathRooted(source) ? source : Path.Combine(module.ModulePath, source);
                        if (Directory.Exists(fullPath))
                        {
                            ProcessDirectory(fullPath);
                        }
                        else if (File.Exists(fullPath) && (fullPath.EndsWith(".h") || fullPath.EndsWith(".hpp")))
                        {
                            ProcessHeaderFile(fullPath);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                logger.Error($"    处理模块 {module.Name} 失败: {ex.Message}", ex);
            }
        }

        private string GetGenerateFilePath(string headerPath)
        {
            // 生成文件直接放在 Intermediate/Generated 目录下，使用平坦结构
            string intermediateRoot = Path.Combine(projectRoot, "Intermediate", "Generated");
            string fileName = Path.GetFileNameWithoutExtension(headerPath);
            
            Directory.CreateDirectory(intermediateRoot); // 确保目录存在
            
            return Path.Combine(intermediateRoot, $"{fileName}.generate.h");
        }

        private void GenerateCodeFile(string outputPath, HeaderInfo headerInfo)
        {
            using StreamWriter writer = new StreamWriter(outputPath);

            writer.WriteLine("// 此文件由 NutHeaderTools 自动生成 - 请勿手动修改");
            writer.WriteLine($"// 生成时间: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            writer.WriteLine($"// 源文件: {Path.GetFileName(headerInfo.FilePath)}");
            writer.WriteLine();
            writer.WriteLine("#pragma once");
            writer.WriteLine();

            // 检查是否需要包含反射系统头文件
            bool needsReflectionInclude = headerInfo.Classes.Any(c => 
                !IsTemplateClass(c.Name) && !IsAbstractClass(c.Name));

            if (needsReflectionInclude)
            {
                writer.WriteLine("// 包含反射系统头文件");
                writer.WriteLine("#include \"Reflection/ReflectionStructures.h\"");
                writer.WriteLine("#include \"Reflection/ReflectionRegistry.h\"");
                writer.WriteLine();
            }

            writer.WriteLine("namespace NLib");
            writer.WriteLine("{");
            writer.WriteLine();

            foreach (ClassInfo classInfo in headerInfo.Classes)
            {
                // 生成类的反射数据和函数实现
                GenerateClassImplementation(writer, classInfo);
            }

            // 重定义GENERATED_BODY宏为第一个类的实现
            if (headerInfo.Classes.Count > 0)
            {
                var firstClass = headerInfo.Classes[0];
                writer.WriteLine("} // namespace NLib");
                writer.WriteLine();
                writer.WriteLine("// 重定义GENERATED_BODY宏为当前文件的类实现");
                writer.WriteLine("#undef GENERATED_BODY");
                writer.WriteLine($"#define GENERATED_BODY() {firstClass.Name}_GENERATED_BODY");
                writer.WriteLine();
                
                // 生成静态成员变量定义宏
                writer.WriteLine("// 静态成员变量定义宏 - 在类定义之后使用");
                foreach (ClassInfo classInfo in headerInfo.Classes)
                {
                    writer.WriteLine($"#define IMPLEMENT_{classInfo.Name}_STATICS() \\");
                    writer.WriteLine($"    namespace NLib {{ bool {classInfo.Name}::bReflectionRegistered = false; }}");
                    writer.WriteLine();
                }
            }
            else
            {
                writer.WriteLine("} // namespace NLib");
            }

            writer.WriteLine();
            writer.WriteLine("// NutHeaderTools 生成结束");
        }

        private void GenerateClassImplementation(StreamWriter writer, ClassInfo classInfo)
        {
            writer.WriteLine($"// {classInfo.Name} 反射实现");
            
            // 生成类特定的GENERATED_BODY宏
            writer.WriteLine($"#define {classInfo.Name}_GENERATED_BODY \\");
            writer.WriteLine($"private: \\");
            writer.WriteLine($"    friend class NLib::CReflectionRegistry; \\");
            writer.WriteLine($"    static bool bReflectionRegistered; \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    static void RegisterReflection() \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        if (!bReflectionRegistered) \\");
            writer.WriteLine($"        {{ \\");
            writer.WriteLine($"            auto& Registry = NLib::CReflectionRegistry::GetInstance(); \\");
            writer.WriteLine($"            Registry.RegisterClass(GetStaticClassReflection()); \\");
            writer.WriteLine($"            bReflectionRegistered = true; \\");
            writer.WriteLine($"        }} \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"public: \\");
            writer.WriteLine($"    using Super = NObject; \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    virtual const std::type_info& GetTypeInfo() const override \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        return typeid(*this); \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    virtual const char* GetTypeName() const override \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        return GetStaticTypeName(); \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    virtual const NLib::SClassReflection* GetClassReflection() const override \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        return GetStaticClassReflection(); \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    static const char* GetStaticTypeName() \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        return \"{classInfo.Name}\"; \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    static const NLib::SClassReflection* GetStaticClassReflection() \\");
            writer.WriteLine($"    {{ \\");
            writer.WriteLine($"        static const SClassReflection reflection = {{ \\");
            writer.WriteLine($"            \"{classInfo.Name}\", \\");
            writer.WriteLine($"            \"{classInfo.BaseClass}\", \\");
            writer.WriteLine($"            sizeof({classInfo.Name}), \\");
            writer.WriteLine($"            &typeid({classInfo.Name}), \\");
            writer.WriteLine($"            EClassFlags::None, \\");
            writer.WriteLine($"            nullptr, nullptr, nullptr, \\");
            writer.WriteLine($"            nullptr, 0, nullptr, 0, \\");
            writer.WriteLine($"            nullptr \\");
            writer.WriteLine($"        }}; \\");
            writer.WriteLine($"        return &reflection; \\");
            writer.WriteLine($"    }} \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"    static NLib::NObject* CreateDefaultObject(); \\");
            writer.WriteLine($" \\");
            writer.WriteLine($"private:");
            writer.WriteLine();
        }



        private static bool IsTemplateClass(string className)
        {
            // 检查是否为已知的模板类
            var templateClasses = new[] { "NArray", "NHashMap" };
            return templateClasses.Contains(className);
        }

        private static bool IsAbstractClass(string className)
        {
            // 检查是否为已知的抽象类
            var abstractClasses = new[] { "NContainer" };
            return abstractClasses.Contains(className);
        }



    }

    // === 数据结构定义 ===

    public class HeaderInfo
    {
        public string FilePath { get; set; } = "";
        public string FileName { get; set; } = "";
        public List<ClassInfo> Classes { get; set; } = new();

        public bool HasNClassMarkedClasses => Classes.Count > 0;
    }

    public class ClassInfo
    {
        public string Name { get; set; } = "";
        public string BaseClass { get; set; } = "NObject";
        public List<PropertyInfo> Properties { get; set; } = new();
        public List<FunctionInfo> Functions { get; set; } = new();
    }

    public class PropertyInfo
    {
        public string Type { get; set; } = "";
        public string Name { get; set; } = "";
        public string Attributes { get; set; } = "";
    }

    public class FunctionInfo
    {
        public string ReturnType { get; set; } = "";
        public string Name { get; set; } = "";
        public string Parameters { get; set; } = "";
        public string Attributes { get; set; } = "";
    }

}
