using NutBuildTools.BuildSystem;
using System.Collections.Generic;

/// <summary>
/// PlayerService Kubernetes 部署目标
/// 用于在 Kubernetes 集群中部署 PlayerService
/// </summary>
public class PlayerServiceKubernetesTarget : NutKubernetesTarget
{
    public PlayerServiceKubernetesTarget() : base()
    {
        // 基本配置
        Name = "PlayerService";
        DeploymentName = "playerservice-service";
        
        // 副本数量
        Replicas = 2;
        
        // 容器端口
        ContainerPort = 50051;
        
        // 环境变量配置
        Environment = new Dictionary<string, string>
        {
            { "CONFIG_PATH", "/app/PlayerServiceConfig.json" },
            { "GRPC_PORT", "50051" },
            { "LOG_LEVEL", "INFO" }
        };
    }
}