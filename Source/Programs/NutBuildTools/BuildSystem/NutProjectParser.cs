using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis.CSharp.Scripting;
using Microsoft.CodeAnalysis.Scripting;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    public class NutProjectParser
    {
        public async Task<INutBuildTarget> Parse(string target)
        {
            if (!File.Exists($"{target}.Build.cs"))
            {
                Logger.Log("找不到Build文件");              
            }

            string buildCode = await File.ReadAllTextAsync($"{target}.Build.cs");

            Script sc = CSharpScript.Create<INutBuildTarget>(buildCode);
            ScriptOptions options = ScriptOptions.Default
                .AddReferences(Assembly.GetExecutingAssembly())
                .AddImports("NutBuildTools.BuildSystem", "System.Collections.Generic");
            sc.WithOptions(options);

            var result = await sc.RunAsync();
            var compilation = result.Script.GetCompilation();

            return await Task.FromResult<INutBuildTarget>(null);
        }
    }
} 