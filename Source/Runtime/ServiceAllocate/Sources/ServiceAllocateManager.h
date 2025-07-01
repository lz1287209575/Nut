#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

class FConfigManager;
class FLogger;

// 服务信息结构
struct FServiceInfo {
    std::string ServiceName;
    std::string ServiceType;
    std::string Host;
    int Port;
    int CurrentLoad;
    int MaxLoad;
    bool bIsHealthy;
    std::chrono::system_clock::time_point LastHeartbeat;
    
    FServiceInfo() : Port(0), CurrentLoad(0), MaxLoad(100), bIsHealthy(true) {}
};

// 服务分配结果
struct FAllocationResult {
    bool bSuccess;
    std::string ServiceName;
    std::string Host;
    int Port;
    std::string ErrorMessage;
    
    FAllocationResult() : bSuccess(false), Port(0) {}
};

class FServiceAllocateManager {
public:
    explicit FServiceAllocateManager(FConfigManager* InConfigManager);
    ~FServiceAllocateManager();
    
    bool Initialize();
    void Start();
    void Stop();
    void Shutdown();
    [[nodiscard]] bool IsRunning() const;
    
    // 服务注册与发现
    bool RegisterService(const FServiceInfo& InServiceInfo);
    bool UnregisterService(const std::string& InServiceName);
    std::vector<FServiceInfo> GetServicesByType(const std::string& InServiceType);
    
    // 服务分配
    FAllocationResult AllocateService(const std::string& InServiceType, const std::string& InClientId = "");
    
    // 健康检查
    void UpdateServiceHealth(const std::string& InServiceName, bool bInIsHealthy);
    void UpdateServiceLoad(const std::string& InServiceName, int InCurrentLoad);
    
    // 心跳处理
    void ProcessHeartbeat(const std::string& InServiceName);
    
private:
    void HealthCheckThread();
    void CleanupUnhealthyServices();
    FServiceInfo* FindService(const std::string& InServiceName);
    FServiceInfo* SelectBestService(const std::string& InServiceType);
    
    FConfigManager* ConfigManager;
    std::unique_ptr<FLogger> Logger;
    
    std::map<std::string, FServiceInfo> Services;
    std::map<std::string, std::vector<std::string>> ServiceTypeIndex;
    
    std::atomic<bool> bRunning;
    std::thread HealthCheckThreadHandle;
    std::mutex ServicesMutex;
    
    // 配置参数
    int HealthCheckInterval;  // 健康检查间隔（秒）
    int HeartbeatTimeout;     // 心跳超时时间（秒）
    int MaxRetries;           // 最大重试次数
}; 