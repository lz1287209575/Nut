using NutBuildSystem.BuildTargets;

namespace NutProjectFileGenerator.Models
{
    /// <summary>
    /// 项目信息模型
    /// </summary>
    public class ProjectInfo
    {
        /// <summary>
        /// 项目名称
        /// </summary>
        public string Name { get; set; } = "";

        /// <summary>
        /// 项目类型
        /// </summary>
        public ProjectType Type { get; set; } = ProjectType.Executable;

        /// <summary>
        /// 源文件路径列表
        /// </summary>
        public List<string> SourceFiles { get; set; } = new();

        /// <summary>
        /// 头文件路径列表
        /// </summary>
        public List<string> HeaderFiles { get; set; } = new();

        /// <summary>
        /// 包含目录列表
        /// </summary>
        public List<string> IncludeDirectories { get; set; } = new();

        /// <summary>
        /// 依赖库列表
        /// </summary>
        public List<string> Dependencies { get; set; } = new();

        /// <summary>
        /// 预处理器定义
        /// </summary>
        public List<string> PreprocessorDefinitions { get; set; } = new();

        /// <summary>
        /// 编译器标志
        /// </summary>
        public List<string> CompilerFlags { get; set; } = new();

        /// <summary>
        /// 链接器标志
        /// </summary>
        public List<string> LinkerFlags { get; set; } = new();

        /// <summary>
        /// 输出目录
        /// </summary>
        public string OutputDirectory { get; set; } = "";

        /// <summary>
        /// 输出文件名
        /// </summary>
        public string OutputName { get; set; } = "";

        /// <summary>
        /// 项目根目录
        /// </summary>
        public string ProjectRoot { get; set; } = "";

        /// <summary>
        /// 构建目标信息
        /// </summary>
        public INutBuildTarget? BuildTarget { get; set; }

        /// <summary>
        /// 支持的平台
        /// </summary>
        public List<string> SupportedPlatforms { get; set; } = new() { "Windows", "Linux", "MacOS" };

        /// <summary>
        /// 支持的配置
        /// </summary>
        public List<string> SupportedConfigurations { get; set; } = new() { "Debug", "Release" };
    }

    /// <summary>
    /// 项目类型枚举
    /// </summary>
    public enum ProjectType
    {
        /// <summary>
        /// 可执行文件
        /// </summary>
        Executable,

        /// <summary>
        /// 静态库
        /// </summary>
        StaticLibrary,

        /// <summary>
        /// 动态库
        /// </summary>
        DynamicLibrary,

        /// <summary>
        /// 头文件库
        /// </summary>
        HeaderOnly,

        /// <summary>
        /// C# 类库
        /// </summary>
        CSharpLibrary,

        /// <summary>
        /// C# 可执行文件
        /// </summary>
        CSharpExecutable
    }

    /// <summary>
    /// 解决方案信息模型
    /// </summary>
    public class SolutionInfo
    {
        /// <summary>
        /// 解决方案名称
        /// </summary>
        public string Name { get; set; } = "NutEngine";

        /// <summary>
        /// 项目列表
        /// </summary>
        public List<ProjectInfo> Projects { get; set; } = new();

        /// <summary>
        /// 解决方案根目录
        /// </summary>
        public string SolutionRoot { get; set; } = "";

        /// <summary>
        /// 第三方库目录
        /// </summary>
        public string ThirdPartyDirectory { get; set; } = "";

        /// <summary>
        /// 中间文件目录
        /// </summary>
        public string IntermediateDirectory { get; set; } = "";

        /// <summary>
        /// 输出目录
        /// </summary>
        public string BinaryDirectory { get; set; } = "";

        /// <summary>
        /// 全局包含目录
        /// </summary>
        public List<string> GlobalIncludeDirectories { get; set; } = new();

        /// <summary>
        /// 全局预处理器定义
        /// </summary>
        public List<string> GlobalPreprocessorDefinitions { get; set; } = new();
    }
}