using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Emit;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    public class NutProjectParser
    {
        public async Task<INutBuildTarget> ParseAsync(string target)
        {
            if (!File.Exists($"{target}.Build.cs"))
            {
                Logger.Error("找不到Build文件");
                return null;
            }

            string code = await File.ReadAllTextAsync($"{target}.Build.cs");

            SyntaxTree st = CSharpSyntaxTree.ParseText(code);
            CSharpCompilation compilation = CSharpCompilation.Create(
                $"Nut{target}Build",
                [st],
                [MetadataReference.CreateFromFile(Assembly.GetExecutingAssembly().Location)]
            );

            using var ms = new MemoryStream();
            EmitResult result = compilation.Emit(ms);
            if (!result.Success)
            {
                // 处理编译错误
                foreach (var diagnostic in result.Diagnostics)
                {
                    Logger.Error(diagnostic.GetMessage());
                }
                return null;
            }
            ms.Seek(0, SeekOrigin.Begin);
            Assembly assembly = Assembly.Load(ms.ToArray());
            var targetClass = assembly.GetTypes().Single(
                t =>
                    typeof(INutBuildTarget).IsAssignableFrom(t) && !t.IsAbstract && !t.IsInterface
            );
            return Activator.CreateInstance(targetClass) as INutBuildTarget;
        }
    }
}
