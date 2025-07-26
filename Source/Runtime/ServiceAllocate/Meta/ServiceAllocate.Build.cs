using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// ServiceAllocate 可执行程序构建目标
/// 服务分配管理器，负责管理服务实例的分配
/// </summary>
public class ServiceAllocateTarget : NutExecutableTarget
{
    public ServiceAllocateTarget() : base()
    {
        // 基本配置
        Name = "ServiceAllocate";
        ExecutableType = "executable";
        OutputName = "ServiceAllocate";
        
        // 源文件配置
        Sources = new List<string>
        {
            // 自动发现 Sources 目录下的所有 .cpp 文件
        };
        
        // 头文件包含目录
        IncludeDirs = new List<string>
        {
            "../Sources"
        };
        
        // 依赖库
        Dependencies = new List<string>
        {
            "LibNut"  // 依赖核心库
        };
    }
}
