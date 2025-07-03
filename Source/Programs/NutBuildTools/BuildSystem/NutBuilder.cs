using System.Threading.Tasks;

namespace NutBuildTools.BuildSystem
{
    public class NutBuilder
    {
        public async ValueTask BuildAsync(NutTarget target)
        {
            await Task.CompletedTask;
        }
    }
} 