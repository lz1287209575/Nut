using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// PlayerService Docker 构建目标
/// 用于构建 PlayerService 的 Docker 镜像
/// </summary>
public class PlayerServiceDockerTarget : NutDockerTarget
{
    public PlayerServiceDockerTarget() : base()
    {
        // 基本配置
        Name = "PlayerService";
        BaseImage = "ubuntu:22.04";
        
        // 需要复制到容器的文件
        CopyFiles = new List<string>
        {
            "../Sources/PlayerServiceMain",
            "../Configs/PlayerServiceConfig.json"
        };
        
        // 暴露端口 (gRPC 默认端口)
        ExposePorts = new List<int?>
        {
            50051
        };
        
        // 容器启动命令
        Entrypoint = new List<string>
        {
            "./PlayerServiceMain",
            "--config",
            "PlayerServiceConfig.json"
        };
    }
}