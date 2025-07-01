#include "ServiceAllocateManager.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <algorithm>
#include <chrono>
#include <random>

ServiceAllocateManager::ServiceAllocateManager(ConfigManager* configManager)
    : m_configManager(configManager)
    , m_logger(std::make_unique<Logger>())
    , m_running(false)
    , m_healthCheckInterval(30)
    , m_heartbeatTimeout(60)
    , m_maxRetries(3) {
}

ServiceAllocateManager::~ServiceAllocateManager() {
    Shutdown();
}

bool ServiceAllocateManager::Initialize() {
    m_logger->Info("初始化服务分配管理器...");
    
    // 从配置加载参数
    m_healthCheckInterval = m_configManager->GetInt("health_check_interval", 30);
    m_heartbeatTimeout = m_configManager->GetInt("heartbeat_timeout", 60);
    m_maxRetries = m_configManager->GetInt("max_retries", 3);
    
    m_logger->Info("服务分配管理器初始化完成");
    return true;
}

void ServiceAllocateManager::Start() {
    if (m_running) {
        m_logger->Warn("服务分配管理器已经在运行");
        return;
    }
    
    m_running = true;
    m_healthCheckThread = std::thread(&ServiceAllocateManager::HealthCheckThread, this);
    m_logger->Info("服务分配管理器启动成功");
}

void ServiceAllocateManager::Stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    if (m_healthCheckThread.joinable()) {
        m_healthCheckThread.join();
    }
    m_logger->Info("服务分配管理器已停止");
}

void ServiceAllocateManager::Shutdown() {
    Stop();
    m_logger->Info("服务分配管理器已关闭");
}

bool ServiceAllocateManager::IsRunning() const {
    return m_running;
}

bool ServiceAllocateManager::RegisterService(const ServiceInfo& serviceInfo) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    // 检查服务是否已存在
    if (m_services.find(serviceInfo.serviceName) != m_services.end()) {
        m_logger->Warn("服务已存在: " + serviceInfo.serviceName);
        return false;
    }
    
    // 注册服务
    m_services[serviceInfo.serviceName] = serviceInfo;
    
    // 更新类型索引
    m_serviceTypeIndex[serviceInfo.serviceType].push_back(serviceInfo.serviceName);
    
    m_logger->Info("注册服务成功: " + serviceInfo.serviceName + 
                  " (" + serviceInfo.serviceType + ") " +
                  serviceInfo.host + ":" + std::to_string(serviceInfo.port));
    
    return true;
}

bool ServiceAllocateManager::UnregisterService(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    auto it = m_services.find(serviceName);
    if (it == m_services.end()) {
        m_logger->Warn("服务不存在: " + serviceName);
        return false;
    }
    
    // 从类型索引中移除
    std::string serviceType = it->second.serviceType;
    auto& typeServices = m_serviceTypeIndex[serviceType];
    typeServices.erase(std::remove(typeServices.begin(), typeServices.end(), serviceName), typeServices.end());
    
    // 如果类型下没有服务了，删除类型索引
    if (typeServices.empty()) {
        m_serviceTypeIndex.erase(serviceType);
    }
    
    // 从服务列表中移除
    m_services.erase(it);
    
    m_logger->Info("注销服务成功: " + serviceName);
    return true;
}

std::vector<ServiceInfo> ServiceAllocateManager::GetServicesByType(const std::string& serviceType) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    std::vector<ServiceInfo> result;
    auto it = m_serviceTypeIndex.find(serviceType);
    if (it != m_serviceTypeIndex.end()) {
        for (const auto& serviceName : it->second) {
            auto serviceIt = m_services.find(serviceName);
            if (serviceIt != m_services.end() && serviceIt->second.isHealthy) {
                result.push_back(serviceIt->second);
            }
        }
    }
    
    return result;
}

AllocationResult ServiceAllocateManager::AllocateService(const std::string& serviceType, const std::string& clientId) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    AllocationResult result;
    
    ServiceInfo* bestService = SelectBestService(serviceType);
    if (!bestService) {
        result.errorMessage = "没有可用的服务: " + serviceType;
        m_logger->Warn(result.errorMessage);
        return result;
    }
    
    // 检查负载
    if (bestService->currentLoad >= bestService->maxLoad) {
        result.errorMessage = "服务负载过高: " + bestService->serviceName;
        m_logger->Warn(result.errorMessage);
        return result;
    }
    
    // 分配服务
    result.success = true;
    result.serviceName = bestService->serviceName;
    result.host = bestService->host;
    result.port = bestService->port;
    
    // 增加负载
    bestService->currentLoad++;
    
    m_logger->Info("分配服务成功: " + result.serviceName + 
                  " 给客户端: " + (clientId.empty() ? "unknown" : clientId) +
                  " (当前负载: " + std::to_string(bestService->currentLoad) + "/" + 
                  std::to_string(bestService->maxLoad) + ")");
    
    return result;
}

void ServiceAllocateManager::UpdateServiceHealth(const std::string& serviceName, bool isHealthy) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    ServiceInfo* service = FindService(serviceName);
    if (service) {
        service->isHealthy = isHealthy;
        m_logger->Info("更新服务健康状态: " + serviceName + " -> " + (isHealthy ? "健康" : "不健康"));
    }
}

void ServiceAllocateManager::UpdateServiceLoad(const std::string& serviceName, int currentLoad) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    ServiceInfo* service = FindService(serviceName);
    if (service) {
        service->currentLoad = currentLoad;
        m_logger->Debug("更新服务负载: " + serviceName + " -> " + std::to_string(currentLoad));
    }
}

void ServiceAllocateManager::ProcessHeartbeat(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    ServiceInfo* service = FindService(serviceName);
    if (service) {
        service->lastHeartbeat = std::chrono::system_clock::now();
        service->isHealthy = true;
        m_logger->Debug("处理心跳: " + serviceName);
    }
}

void ServiceAllocateManager::HealthCheckThread() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(m_healthCheckInterval));
        CleanupUnhealthyServices();
    }
}

void ServiceAllocateManager::CleanupUnhealthyServices() {
    std::lock_guard<std::mutex> lock(m_servicesMutex);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> servicesToRemove;
    
    for (auto& pair : m_services) {
        ServiceInfo& service = pair.second;
        auto timeSinceHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(
            now - service.lastHeartbeat).count();
        
        if (timeSinceHeartbeat > m_heartbeatTimeout) {
            service.isHealthy = false;
            m_logger->Warn("服务心跳超时: " + service.serviceName + 
                          " (超时: " + std::to_string(timeSinceHeartbeat) + "s)");
        }
    }
}

ServiceInfo* ServiceAllocateManager::FindService(const std::string& serviceName) {
    auto it = m_services.find(serviceName);
    return (it != m_services.end()) ? &it->second : nullptr;
}

ServiceInfo* ServiceAllocateManager::SelectBestService(const std::string& serviceType) {
    auto it = m_serviceTypeIndex.find(serviceType);
    if (it == m_serviceTypeIndex.end()) {
        return nullptr;
    }
    
    std::vector<ServiceInfo*> availableServices;
    
    // 收集健康的服务
    for (const auto& serviceName : it->second) {
        ServiceInfo* service = FindService(serviceName);
        if (service && service->isHealthy && service->currentLoad < service->maxLoad) {
            availableServices.push_back(service);
        }
    }
    
    if (availableServices.empty()) {
        return nullptr;
    }
    
    // 简单的负载均衡策略：选择负载最低的服务
    auto bestService = std::min_element(availableServices.begin(), availableServices.end(),
        [](const ServiceInfo* a, const ServiceInfo* b) {
            return a->currentLoad < b->currentLoad;
        });
    
    return *bestService;
} 