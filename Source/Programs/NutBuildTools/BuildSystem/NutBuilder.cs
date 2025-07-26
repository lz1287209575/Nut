using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    public class NutBuilder
    {
        public async ValueTask BuildAsync(NutTarget target)
        {
            Logger.Info($"开始构建: Target={target.Name}, Platform={target.Platform}, Configuration={target.Configuration}");

            IEnumerable<string> sourceFileList = GetSourceFileList(target);

            Logger.Info("构建流程结束");
            await Task.CompletedTask;

        }

        public async ValueTask CleanAsync(NutTarget target)
        {
            Logger.Info($"开始清理: Target={target.Name}, Platform={target.Platform}, Configuration={target.Configuration}");
            // 清理逻辑
            // 例如删除构建输出目录、临时文件等
            // 模拟清理操作
            Logger.Info("清理流程结束");

            await Task.CompletedTask;
        }

        protected IEnumerable<string> GetSourceFileList(NutTarget target)
        {
            Logger.Info($"获取源文件列表: Target={target.Name}, Platform={target.Platform}, Configuration={target.Configuration}");
            var sourceFileExtensions = new string[] { ".cpp", ".hpp", ".c", ".cxx" };
            var sourceList = new List<string>();
            foreach (var source in target.Sources)
            {
                Logger.Info($"处理源目录: {source}");
                foreach (var filePath in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
                {
                    if (sourceFileExtensions.Contains(Path.GetExtension(filePath)))
                    {
                        Logger.Info($"发现源文件: {filePath}");
                        sourceList.Add(filePath);
                    }
                }
            }

            return sourceList;
        }

    }
}