namespace NutBuildSystem.BuildTargets
{
    /// <summary>
    /// 构建目标接口 - 用于Roslyn动态编译的Build.cs文件
    /// </summary>
    public interface INutBuildTarget
    {
        /// <summary>
        /// 目标名称
        /// </summary>
        string Name { get; set; }

        /// <summary>
        /// 源文件列表
        /// </summary>
        List<string> Sources { get; set; }

        /// <summary>
        /// 包含目录列表
        /// </summary>
        List<string> IncludeDirs { get; set; }

        /// <summary>
        /// 依赖库列表
        /// </summary>
        List<string> Dependencies { get; set; }

        /// <summary>
        /// 输出名称
        /// </summary>
        string OutputName { get; set; }
    }
}