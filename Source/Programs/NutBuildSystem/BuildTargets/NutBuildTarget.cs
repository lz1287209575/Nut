namespace NutBuildSystem.BuildTargets
{
    /// <summary>
    /// 构建目标基类
    /// </summary>
    public abstract class NutBuildTarget : INutBuildTarget
    {
        public string Name { get; set; } = "";
        public List<string> Sources { get; set; } = new();
        public List<string> IncludeDirs { get; set; } = new();
        public List<string> Dependencies { get; set; } = new();
        public string OutputName { get; set; } = "";

        protected NutBuildTarget()
        {
        }
    }

    /// <summary>
    /// 静态库构建目标
    /// </summary>
    public abstract class NutStaticLibraryTarget : NutBuildTarget
    {
        public string LibraryType { get; set; } = "static_library";
    }

    /// <summary>
    /// 可执行文件构建目标
    /// </summary>
    public abstract class NutExecutableTarget : NutBuildTarget
    {
        public string ExecutableType { get; set; } = "executable";
    }

    /// <summary>
    /// 微服务构建目标
    /// </summary>
    public abstract class NutServiceTarget : NutBuildTarget
    {
        public string ServiceType { get; set; } = "executable";
        public List<string> ProtoFiles { get; set; } = new();
        public List<string> ConfigFiles { get; set; } = new();
    }

    /// <summary>
    /// Docker构建目标
    /// </summary>
    public abstract class NutDockerTarget : NutBuildTarget
    {
        public string BaseImage { get; set; } = "";
        public List<string> DockerFiles { get; set; } = new();
    }

    /// <summary>
    /// Kubernetes构建目标
    /// </summary>
    public abstract class NutKubernetesTarget : NutBuildTarget
    {
        public List<string> KubernetesFiles { get; set; } = new();
    }
}