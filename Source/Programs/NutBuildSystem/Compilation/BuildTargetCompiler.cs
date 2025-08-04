using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using NutBuildSystem.BuildTargets;
using System.Reflection;

namespace NutBuildSystem.Compilation
{
    /// <summary>
    /// Build.cs文件编译器 - 使用Roslyn动态编译
    /// </summary>
    public class BuildTargetCompiler
    {
        /// <summary>
        /// 编译Build.cs文件并创建目标实例
        /// </summary>
        /// <param name="buildFilePath">Build.cs文件路径</param>
        /// <returns>构建目标实例</returns>
        public static async Task<INutBuildTarget?> CompileAndCreateTargetAsync(string buildFilePath)
        {
            if (!File.Exists(buildFilePath))
            {
                throw new FileNotFoundException($"Build文件不存在: {buildFilePath}");
            }

            try
            {
                string code = await File.ReadAllTextAsync(buildFilePath);
                
                // 创建语法树
                SyntaxTree syntaxTree = CSharpSyntaxTree.ParseText(code);
                
                // 获取必要的程序集引用
                var references = GetRequiredReferences();
                
                // 创建编译选项
                var compilation = CSharpCompilation.Create(
                    assemblyName: $"BuildTarget_{Path.GetFileNameWithoutExtension(buildFilePath)}",
                    syntaxTrees: new[] { syntaxTree },
                    references: references,
                    options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
                );

                // 编译到内存
                using var memoryStream = new MemoryStream();
                var result = compilation.Emit(memoryStream);
                
                if (!result.Success)
                {
                    var errors = result.Diagnostics
                        .Where(diagnostic => diagnostic.IsWarningAsError || diagnostic.Severity == DiagnosticSeverity.Error)
                        .Select(diagnostic => diagnostic.GetMessage())
                        .ToList();
                    
                    throw new InvalidOperationException($"编译Build.cs失败: {string.Join(", ", errors)}");
                }

                // 加载程序集
                memoryStream.Seek(0, SeekOrigin.Begin);
                var assembly = Assembly.Load(memoryStream.ToArray());
                
                // 查找实现INutBuildTarget的类
                var targetType = assembly.GetTypes()
                    .FirstOrDefault(t => typeof(INutBuildTarget).IsAssignableFrom(t) && 
                                       !t.IsAbstract && 
                                       !t.IsInterface);
                
                if (targetType == null)
                {
                    throw new InvalidOperationException($"在{buildFilePath}中找不到实现INutBuildTarget的类");
                }
                
                // 创建实例
                var instance = Activator.CreateInstance(targetType) as INutBuildTarget;
                return instance;
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"编译Build.cs文件失败: {buildFilePath}", ex);
            }
        }

        /// <summary>
        /// 获取编译所需的程序集引用
        /// </summary>
        private static List<MetadataReference> GetRequiredReferences()
        {
            var references = new List<MetadataReference>();
            
            // 添加核心.NET引用
            references.Add(MetadataReference.CreateFromFile(typeof(object).Assembly.Location));
            references.Add(MetadataReference.CreateFromFile(typeof(List<>).Assembly.Location));
            references.Add(MetadataReference.CreateFromFile(typeof(Console).Assembly.Location));
            
            // 添加当前程序集引用（包含INutBuildTarget等接口）
            references.Add(MetadataReference.CreateFromFile(typeof(INutBuildTarget).Assembly.Location));
            
            // 添加System.Runtime引用
            var systemRuntime = Assembly.Load("System.Runtime");
            references.Add(MetadataReference.CreateFromFile(systemRuntime.Location));
            
            // 添加System.Collections引用
            var systemCollections = Assembly.Load("System.Collections");
            references.Add(MetadataReference.CreateFromFile(systemCollections.Location));

            return references;
        }
    }
}