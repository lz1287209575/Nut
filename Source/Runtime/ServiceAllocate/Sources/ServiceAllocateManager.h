#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

class ConfigManager;
class Logger;

// 服务信息结构
struct ServiceInfo {
    std::string serviceName;
    std::string serviceType;
    std::string host;
    int port;
    int currentLoad;
    int maxLoad;
    bool isHealthy;
    std::chrono::system_clock::time_point lastHeartbeat;
    
    ServiceInfo() : port(0), currentLoad(0), maxLoad(100), isHealthy(true) {}
};

// 服务分配结果
struct AllocationResult {
    bool success;
    std::string serviceName;
    std::string host;
    int port;
    std::string errorMessage;
    
    AllocationResult() : success(false), port(0) {}
};

class ServiceAllocateManager {
public:
    explicit ServiceAllocateManager(ConfigManager* configManager);
    ~ServiceAllocateManager();
    
    bool Initialize();
    void Start();
    void Stop();
    void Shutdown();
    bool IsRunning() const;
    
    // 服务注册与发现
    bool RegisterService(const ServiceInfo& serviceInfo);
    bool UnregisterService(const std::string& serviceName);
    std::vector<ServiceInfo> GetServicesByType(const std::string& serviceType);
    
    // 服务分配
    AllocationResult AllocateService(const std::string& serviceType, const std::string& clientId = "");
    
    // 健康检查
    void UpdateServiceHealth(const std::string& serviceName, bool isHealthy);
    void UpdateServiceLoad(const std::string& serviceName, int currentLoad);
    
    // 心跳处理
    void ProcessHeartbeat(const std::string& serviceName);
    
private:
    void HealthCheckThread();
    void CleanupUnhealthyServices();
    ServiceInfo* FindService(const std::string& serviceName);
    ServiceInfo* SelectBestService(const std::string& serviceType);
    
    ConfigManager* m_configManager;
    std::unique_ptr<Logger> m_logger;
    
    std::map<std::string, ServiceInfo> m_services;
    std::map<std::string, std::vector<std::string>> m_serviceTypeIndex;
    
    std::atomic<bool> m_running;
    std::thread m_healthCheckThread;
    std::mutex m_servicesMutex;
    
    // 配置参数
    int m_healthCheckInterval;  // 健康检查间隔（秒）
    int m_heartbeatTimeout;     // 心跳超时时间（秒）
    int m_maxRetries;          // 最大重试次数
}; 