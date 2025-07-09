using System.Threading.Tasks;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    public class NutBuilder
    {
        public async ValueTask BuildAsync(NutTarget target)
        {
            Logger.Info($"开始构建: Target={target.Target}, Platform={target.Platform}, Configuration={target.Configuration}");
            Logger.Info("构建流程结束");


            await Task.CompletedTask;
        }
    }
}
