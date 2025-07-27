using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// PlayerService 微服务构建目标
/// 基于 gRPC 的玩家服务，负责处理玩家相关操作
/// </summary>
public class PlayerServiceTarget : NutServiceTarget
{
    public PlayerServiceTarget() : base()
    {
        // 基本配置
        Name = "PlayerService";
        ServiceType = "executable";
        OutputName = "PlayerServiceMain";
        
        // 源文件配置
        Sources = new List<string>
        {
            "../Sources/PlayerServiceMain.cpp"
        };
        
        // 头文件包含目录
        IncludeDirs = new List<string>
        {
            "../Sources",
            "../../LibNut/Sources",           // LibNut头文件
            "../../../ThirdParty/tcmalloc/src",
            "../../../ThirdParty/spdlog/include"
        };
        
        // Protocol Buffer 文件
        ProtoFiles = new List<string>
        {
            "../Protos/playerservice.proto"
        };
        
        // 配置文件
        ConfigFiles = new List<string>
        {
            "../Configs/PlayerServiceConfig.json"
        };
        
        // 依赖库
        Dependencies = new List<string>
        {
            "LibNut",                         // 依赖LibNut静态库
            "protobuf",
            "grpc",
            "tcmalloc",
            "spdlog"
        };
    }
}