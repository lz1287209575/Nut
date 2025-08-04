using NutProjectFileGenerator.Models;

namespace NutProjectFileGenerator.Generators
{
    /// <summary>
    /// 项目文件生成器接口
    /// </summary>
    public interface IProjectFileGenerator
    {
        /// <summary>
        /// 生成器名称
        /// </summary>
        string Name { get; }

        /// <summary>
        /// 生成器描述
        /// </summary>
        string Description { get; }

        /// <summary>
        /// 支持的平台
        /// </summary>
        List<string> SupportedPlatforms { get; }

        /// <summary>
        /// 检查是否可以在当前环境运行
        /// </summary>
        /// <returns>是否可以运行</returns>
        bool CanGenerate();

        /// <summary>
        /// 生成项目文件
        /// </summary>
        /// <param name="solutionInfo">解决方案信息</param>
        /// <param name="outputDirectory">输出目录</param>
        /// <returns>是否生成成功</returns>
        Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory);

        /// <summary>
        /// 获取生成的文件列表
        /// </summary>
        /// <param name="solutionInfo">解决方案信息</param>
        /// <param name="outputDirectory">输出目录</param>
        /// <returns>生成的文件路径列表</returns>
        List<string> GetGeneratedFiles(SolutionInfo solutionInfo, string outputDirectory);
    }

    /// <summary>
    /// 项目文件生成器基类
    /// </summary>
    public abstract class ProjectFileGeneratorBase : IProjectFileGenerator
    {
        /// <summary>
        /// 生成器名称
        /// </summary>
        public abstract string Name { get; }

        /// <summary>
        /// 生成器描述
        /// </summary>
        public abstract string Description { get; }

        /// <summary>
        /// 支持的平台
        /// </summary>
        public abstract List<string> SupportedPlatforms { get; }

        /// <summary>
        /// 检查是否可以在当前环境运行
        /// </summary>
        /// <returns>是否可以运行</returns>
        public virtual bool CanGenerate()
        {
            return true;
        }

        /// <summary>
        /// 生成项目文件
        /// </summary>
        /// <param name="solutionInfo">解决方案信息</param>
        /// <param name="outputDirectory">输出目录</param>
        /// <returns>是否生成成功</returns>
        public abstract Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory);

        /// <summary>
        /// 获取生成的文件列表
        /// </summary>
        /// <param name="solutionInfo">解决方案信息</param>
        /// <param name="outputDirectory">输出目录</param>
        /// <returns>生成的文件路径列表</returns>
        public abstract List<string> GetGeneratedFiles(SolutionInfo solutionInfo, string outputDirectory);

        /// <summary>
        /// 确保目录存在
        /// </summary>
        /// <param name="directory">目录路径</param>
        protected static void EnsureDirectoryExists(string directory)
        {
            if (!Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }
        }

        /// <summary>
        /// 获取相对路径
        /// </summary>
        /// <param name="from">起始路径</param>
        /// <param name="to">目标路径</param>
        /// <returns>相对路径</returns>
        protected static string GetRelativePath(string from, string to)
        {
            return Path.GetRelativePath(from, to);
        }

        /// <summary>
        /// 规范化路径分隔符
        /// </summary>
        /// <param name="path">路径</param>
        /// <param name="separator">目标分隔符</param>
        /// <returns>规范化后的路径</returns>
        protected static string NormalizePath(string path, char separator = '/')
        {
            return path.Replace('\\', separator).Replace('/', separator);
        }
    }
}