using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// NLib 静态库构建目标
/// Nut 框架的核心静态库
/// </summary>
public class NLibTarget : NutStaticLibraryTarget
{
    public NLibTarget() : base()
    {
        // 基本配置
        Name = "NLib";
        LibraryType = "static_library";
        OutputName = "NLib";
        
        // 源文件配置
        Sources = new List<string>
        {
            // 自动发现 Sources 目录下的所有 .cpp 文件
        };
        
        // 头文件包含目录
        IncludeDirs = new List<string>
        {
            "../Sources",
            "../../../ThirdParty/tcmalloc/src",
            "../../../ThirdParty/spdlog/include"
        };
        
        // 依赖库
        Dependencies = new List<string>
        {
            "tcmalloc",
            "spdlog"
        };
    }
}


