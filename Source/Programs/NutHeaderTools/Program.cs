using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace NutHeaderTools
{
    /// <summary>
    /// NutHeaderTools - Nut引擎的代码生成工具
    /// 类似UE的UnrealHeaderTool，用于解析NCLASS宏并生成.generate.h文件
    /// </summary>
    class Program
    {
        private static readonly Dictionary<string, HeaderInfo> ProcessedHeaders = new();
        private static readonly List<string> SourcePaths = new();
        private static int GeneratedFileCount = 0;

        static void Main(string[] args)
        {
            Console.WriteLine("NutHeaderTools v1.0 - Nut Engine Code Generator");
            Console.WriteLine("=====================================");

            if (args.Length == 0)
            {
                ShowUsage();
                return;
            }

            try
            {
                ParseArguments(args);
                ProcessAllHeaders();
                Console.WriteLine($"\n✓ 成功生成 {GeneratedFileCount} 个 .generate.h 文件");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 错误: {ex.Message}");
                Environment.Exit(1);
            }
        }

        private static void ShowUsage()
        {
            Console.WriteLine("用法:");
            Console.WriteLine("  NutHeaderTools <source_paths> [options]");
            Console.WriteLine();
            Console.WriteLine("参数:");
            Console.WriteLine("  source_paths    要处理的源代码目录路径（可以多个）");
            Console.WriteLine();
            Console.WriteLine("选项:");
            Console.WriteLine("  --help         显示此帮助信息");
            Console.WriteLine("  --verbose      显示详细输出");
            Console.WriteLine();
            Console.WriteLine("示例:");
            Console.WriteLine("  NutHeaderTools \"Source/Runtime/LibNut/Sources\"");
            Console.WriteLine("  NutHeaderTools \"Source/Runtime\" --verbose");
        }

        private static void ParseArguments(string[] args)
        {
            foreach (string arg in args)
            {
                if (arg == "--help")
                {
                    ShowUsage();
                    Environment.Exit(0);
                }
                else if (arg == "--verbose")
                {
                    // 设置详细输出标志
                    continue;
                }
                else if (Directory.Exists(arg))
                {
                    SourcePaths.Add(arg);
                }
                else
                {
                    throw new ArgumentException($"无效的源代码路径: {arg}");
                }
            }

            if (SourcePaths.Count == 0)
            {
                throw new ArgumentException("至少需要指定一个有效的源代码路径");
            }
        }

        private static void ProcessAllHeaders()
        {
            foreach (string sourcePath in SourcePaths)
            {
                Console.WriteLine($"🔍 扫描目录: {sourcePath}");
                ProcessDirectory(sourcePath);
            }
        }

        private static void ProcessDirectory(string directory)
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

        private static void ProcessHeaderFile(string headerPath)
        {
            try
            {
                string content = File.ReadAllText(headerPath);
                Console.WriteLine($"  🔍 处理文件: {Path.GetFileName(headerPath)}");
                
                HeaderInfo headerInfo = ParseHeader(headerPath, content);

                if (headerInfo.HasNClassMarkedClasses)
                {
                    string generateFilePath = GetGenerateFilePath(headerPath);
                    GenerateCodeFile(generateFilePath, headerInfo);
                    ProcessedHeaders[headerPath] = headerInfo;
                    GeneratedFileCount++;
                    
                    Console.WriteLine($"  ✓ 已生成: {Path.GetFileName(generateFilePath)}");
                }
                else
                {
                    Console.WriteLine($"  - 跳过: {Path.GetFileName(headerPath)} (无NCLASS标记)");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"  ❌ 处理文件失败 {Path.GetFileName(headerPath)}: {ex.Message}");
            }
        }

        private static HeaderInfo ParseHeader(string filePath, string content)
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
            
            Console.WriteLine($"    Debug: 文件包含NCLASS");
            
            // 尝试不同的正则表达式，处理换行符
            string[] patterns = {
                @"NCLASS\s*\([^)]*\)\s*\r?\n\s*class\s+(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)\s*class\s+(\w+)\s*(?::\s*public\s+(\w+))?",
                @"NCLASS\s*\([^)]*\)[\s\r\n]*class\s+(\w+)",
                @"NCLASS.*?class\s+(\w+)"
            };
            
            MatchCollection nclassMatches = null;
            foreach (string pattern in patterns)
            {
                nclassMatches = Regex.Matches(content, pattern, RegexOptions.Multiline | RegexOptions.Singleline);
                Console.WriteLine($"    Debug: 模式 '{pattern}' 找到 {nclassMatches.Count} 个匹配");
                if (nclassMatches.Count > 0) break;
            }
            
            Console.WriteLine($"    Debug: 最终找到 {nclassMatches?.Count ?? 0} 个NCLASS匹配");

            foreach (Match match in nclassMatches)
            {
                string className = match.Groups[1].Value;
                string baseClass = match.Groups[2].Success ? match.Groups[2].Value : "NObject";
                Console.WriteLine($"    Debug: 解析类 {className} : {baseClass}");

                ClassInfo classInfo = new ClassInfo
                {
                    Name = className,
                    BaseClass = baseClass
                };

                // 解析NPROPERTY标记的属性
                ParseProperties(content, classInfo);
                
                // 解析NFUNCTION标记的函数
                ParseFunctions(content, classInfo);

                info.Classes.Add(classInfo);
            }

            return info;
        }

        private static void ParseProperties(string content, ClassInfo classInfo)
        {
            // 使用更精确的单一正则表达式，避免重复匹配
            string propertyPattern = @"NPROPERTY\s*\([^)]*\)[\s\r\n]*(\w+(?:\s*<[^>]+>)?(?:\s*::\w+)*)\s+(\w+)(?:\s*=\s*[^;]+)?;";
            
            MatchCollection propertyMatches = Regex.Matches(content, propertyPattern, RegexOptions.Multiline);
            
            HashSet<string> addedProperties = new HashSet<string>();
            
            foreach (Match match in propertyMatches)
            {
                string type = match.Groups[1].Value.Trim();
                string name = match.Groups[2].Value;
                
                // 避免重复添加
                if (!addedProperties.Contains(name))
                {
                    PropertyInfo property = new PropertyInfo
                    {
                        Type = type,
                        Name = name
                    };
                    classInfo.Properties.Add(property);
                    addedProperties.Add(name);
                    Console.WriteLine($"    Debug: 找到属性 {property.Type} {property.Name}");
                }
            }
        }

        private static void ParseFunctions(string content, ClassInfo classInfo)
        {
            // 简化的函数解析 - 查找NFUNCTION()标记的函数
            MatchCollection functionMatches = Regex.Matches(content,
                @"NFUNCTION\s*\([^)]*\)\s*(\w+(?:\s*<[^>]+>)?)\s+(\w+)\s*\([^)]*\);",
                RegexOptions.Multiline);

            foreach (Match match in functionMatches)
            {
                FunctionInfo function = new FunctionInfo
                {
                    ReturnType = match.Groups[1].Value.Trim(),
                    Name = match.Groups[2].Value
                };
                classInfo.Functions.Add(function);
            }
        }

        private static string GetGenerateFilePath(string headerPath)
        {
            string directory = Path.GetDirectoryName(headerPath);
            string fileName = Path.GetFileNameWithoutExtension(headerPath);
            return Path.Combine(directory, $"{fileName}.generate.h");
        }

        private static void GenerateCodeFile(string outputPath, HeaderInfo headerInfo)
        {
            using StreamWriter writer = new StreamWriter(outputPath);
            
            writer.WriteLine("// 此文件由 NutHeaderTools 自动生成 - 请勿手动修改");
            writer.WriteLine($"// 生成时间: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            writer.WriteLine($"// 源文件: {Path.GetFileName(headerInfo.FilePath)}");
            writer.WriteLine();
            writer.WriteLine("#pragma once");
            writer.WriteLine();
            writer.WriteLine("// 包含反射系统头文件");
            writer.WriteLine("#include \"NObjectReflection.h\"");
            writer.WriteLine();
            writer.WriteLine("namespace Nut");
            writer.WriteLine("{");
            writer.WriteLine();

            foreach (ClassInfo classInfo in headerInfo.Classes)
            {
                GenerateClassReflection(writer, classInfo);
            }

            writer.WriteLine("} // namespace Nut");
            writer.WriteLine();
            writer.WriteLine("// NutHeaderTools 生成结束");
        }

        private static void GenerateClassReflection(StreamWriter writer, ClassInfo classInfo)
        {
            writer.WriteLine($"// === {classInfo.Name} 反射信息 ===");
            writer.WriteLine();

            // 生成类型信息结构
            writer.WriteLine($"struct {classInfo.Name}TypeInfo");
            writer.WriteLine("{");
            writer.WriteLine($"    static constexpr const char* ClassName = \"{classInfo.Name}\";");
            writer.WriteLine($"    static constexpr const char* BaseClassName = \"{classInfo.BaseClass}\";");
            writer.WriteLine($"    static constexpr size_t PropertyCount = {classInfo.Properties.Count};");
            writer.WriteLine($"    static constexpr size_t FunctionCount = {classInfo.Functions.Count};");
            writer.WriteLine("};");
            writer.WriteLine();

            // 生成属性访问器
            if (classInfo.Properties.Count > 0)
            {
                writer.WriteLine($"// {classInfo.Name} 属性访问器");
                foreach (PropertyInfo property in classInfo.Properties)
                {
                    writer.WriteLine($"// Property: {property.Type} {property.Name}");
                }
                writer.WriteLine();
            }

            // 生成函数调用器
            if (classInfo.Functions.Count > 0)
            {
                writer.WriteLine($"// {classInfo.Name} 函数调用器");
                foreach (FunctionInfo function in classInfo.Functions)
                {
                    writer.WriteLine($"// Function: {function.ReturnType} {function.Name}()");
                }
                writer.WriteLine();
            }

            // 注册反射信息
            writer.WriteLine($"// 注册 {classInfo.Name} 到反射系统");
            writer.WriteLine($"REGISTER_NCLASS_REFLECTION({classInfo.Name});");
            writer.WriteLine();
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