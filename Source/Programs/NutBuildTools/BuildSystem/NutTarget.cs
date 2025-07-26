using System.Collections.Generic;

namespace NutBuildTools.BuildSystem
{
    /// <summary>
    /// 基础构建目标类
    /// </summary>
    public abstract class NutTarget
    {
        public string Name { get; set; } = string.Empty;
        public string Platform { get; set; } = "Any";
        public string Configuration { get; set; } = "Debug";
        public List<string> Sources { get; set; } = new List<string>();
        public List<string> IncludeDirs { get; set; } = new List<string>();
        public List<string> Dependencies { get; set; } = new List<string>();
        public string OutputName { get; set; } = string.Empty;
        
        protected NutTarget()
        {
            // 基础构造函数
        }
    }

    /// <summary>
    /// 服务构建目标 - 用于微服务项目
    /// </summary>
    public class NutServiceTarget : NutTarget
    {
        public List<string> ProtoFiles { get; set; } = new List<string>();
        public List<string> ConfigFiles { get; set; } = new List<string>();
        public string ServiceType { get; set; } = "executable";
        
        public NutServiceTarget() : base()
        {
        }
    }

    /// <summary>
    /// Docker构建目标
    /// </summary>
    public class NutDockerTarget : NutTarget
    {
        public string BaseImage { get; set; } = "ubuntu:22.04";
        public List<string> CopyFiles { get; set; } = new List<string>();
        public List<int?> ExposePorts { get; set; } = new List<int?>();
        public List<string> Entrypoint { get; set; } = new List<string>();
        
        public NutDockerTarget() : base()
        {
        }
    }

    /// <summary>
    /// Kubernetes部署目标
    /// </summary>
    public class NutKubernetesTarget : NutTarget
    {
        public string DeploymentName { get; set; } = string.Empty;
        public int Replicas { get; set; } = 1;
        public int? ContainerPort { get; set; }
        public Dictionary<string, string> Environment { get; set; } = new Dictionary<string, string>();
        
        public NutKubernetesTarget() : base()
        {
        }
    }

    /// <summary>
    /// 静态库构建目标
    /// </summary>
    public class NutStaticLibraryTarget : NutTarget
    {
        public string LibraryType { get; set; } = "static_library";
        
        public NutStaticLibraryTarget() : base()
        {
        }
    }

    /// <summary>
    /// 可执行程序构建目标
    /// </summary>
    public class NutExecutableTarget : NutTarget
    {
        public string ExecutableType { get; set; } = "executable";
        
        public NutExecutableTarget() : base()
        {
        }
    }
} 