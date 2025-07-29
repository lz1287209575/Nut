using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// TestNClass 可执行程序构建目标
/// 用于测试 NCLASS 反射系统功能
/// </summary>
public class TestNClassTarget : NutExecutableTarget
{
    public TestNClassTarget() : base()
    {
        // 基本配置
        Name = "TestNClass";
        ExecutableType = "executable";
        OutputName = "TestNClass";
        
        // 源文件配置 - 指定源文件目录
        Sources = new List<string>
        {
            "../"  // TestNClass 目录
        };
        
        // 头文件包含目录
        IncludeDirs = new List<string>
        {
            "../../Runtime/LibNut/Sources",
            "../../../ThirdParty/spdlog/include",
            "../../../ThirdParty/tcmalloc/src"
        };
        
        // 依赖库
        Dependencies = new List<string>
        {
            "LibNut"  // 依赖核心库
        };
    }
}