#include "ServiceAllocateManager.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <algorithm>
#include <chrono>

FServiceAllocateManager::FServiceAllocateManager(FConfigManager *InConfigManager)
    : ConfigManager(InConfigManager), Logger(std::make_unique<FLogger>()), bRunning(false), HealthCheckInterval(30),
      HeartbeatTimeout(60), MaxRetries(3)
{
}

FServiceAllocateManager::~FServiceAllocateManager()
{
    Shutdown();
}

bool FServiceAllocateManager::Initialize()
{
    Logger->Info("初始化服务分配管理器...");

    // 从配置加载参数
    HealthCheckInterval = ConfigManager->GetInt("health_check_interval", 30);
    HeartbeatTimeout = ConfigManager->GetInt("heartbeat_timeout", 60);
    MaxRetries = ConfigManager->GetInt("max_retries", 3);

    Logger->Info("服务分配管理器初始化完成");
    return true;
}

void FServiceAllocateManager::Start()
{
    if (bRunning)
    {
        Logger->Warn("服务分配管理器已经在运行");
        return;
    }

    bRunning = true;
    HealthCheckThreadHandle = std::thread(&FServiceAllocateManager::HealthCheckThread, this);
    Logger->Info("服务分配管理器启动成功");
}

void FServiceAllocateManager::Stop()
{
    if (!bRunning)
    {
        return;
    }

    bRunning = false;
    if (HealthCheckThreadHandle.joinable())
    {
        HealthCheckThreadHandle.join();
    }
    Logger->Info("服务分配管理器已停止");
}

void FServiceAllocateManager::Shutdown()
{
    Stop();
    Logger->Info("服务分配管理器已关闭");
}

bool FServiceAllocateManager::IsRunning() const
{
    return bRunning;
}

bool FServiceAllocateManager::RegisterService(const FServiceInfo &InServiceInfo)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    // 检查服务是否已存在
    if (Services.find(InServiceInfo.ServiceName) != Services.end())
    {
        Logger->Warn("服务已存在: " + InServiceInfo.ServiceName);
        return false;
    }

    // 注册服务
    Services[InServiceInfo.ServiceName] = InServiceInfo;

    // 更新类型索引
    ServiceTypeIndex[InServiceInfo.ServiceType].push_back(InServiceInfo.ServiceName);

    Logger->Info("注册服务成功: " + InServiceInfo.ServiceName + " (" + InServiceInfo.ServiceType + ") " +
                 InServiceInfo.Host + ":" + std::to_string(InServiceInfo.Port));

    return true;
}

bool FServiceAllocateManager::UnregisterService(const std::string &InServiceName)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    auto It = Services.find(InServiceName);
    if (It == Services.end())
    {
        Logger->Warn("服务不存在: " + InServiceName);
        return false;
    }

    // 从类型索引中移除
    std::string ServiceType = It->second.ServiceType;
    auto &TypeServices = ServiceTypeIndex[ServiceType];
    TypeServices.erase(std::remove(TypeServices.begin(), TypeServices.end(), InServiceName), TypeServices.end());

    // 如果类型下没有服务了，删除类型索引
    if (TypeServices.empty())
    {
        ServiceTypeIndex.erase(ServiceType);
    }

    // 从服务列表中移除
    Services.erase(It);

    Logger->Info("注销服务成功: " + InServiceName);
    return true;
}

std::vector<FServiceInfo> FServiceAllocateManager::GetServicesByType(const std::string &InServiceType)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    std::vector<FServiceInfo> Result;
    auto It = ServiceTypeIndex.find(InServiceType);
    if (It != ServiceTypeIndex.end())
    {
        for (const auto &ServiceName : It->second)
        {
            auto ServiceIt = Services.find(ServiceName);
            if (ServiceIt != Services.end() && ServiceIt->second.bIsHealthy)
            {
                Result.push_back(ServiceIt->second);
            }
        }
    }

    return Result;
}

FAllocationResult FServiceAllocateManager::AllocateService(const std::string &InServiceType,
                                                           const std::string &InClientId)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    FAllocationResult Result;

    FServiceInfo *BestService = SelectBestService(InServiceType);
    if (!BestService)
    {
        Result.ErrorMessage = "没有可用的服务: " + InServiceType;
        Logger->Warn(Result.ErrorMessage);
        return Result;
    }

    // 检查负载
    if (BestService->CurrentLoad >= BestService->MaxLoad)
    {
        Result.ErrorMessage = "服务负载过高: " + BestService->ServiceName;
        Logger->Warn(Result.ErrorMessage);
        return Result;
    }

    // 分配服务
    Result.bSuccess = true;
    Result.ServiceName = BestService->ServiceName;
    Result.Host = BestService->Host;
    Result.Port = BestService->Port;

    // 增加负载
    BestService->CurrentLoad++;

    Logger->Info("分配服务成功: " + Result.ServiceName + " 给客户端: " + (InClientId.empty() ? "unknown" : InClientId) +
                 " (当前负载: " + std::to_string(BestService->CurrentLoad) + "/" +
                 std::to_string(BestService->MaxLoad) + ")");

    return Result;
}

void FServiceAllocateManager::UpdateServiceHealth(const std::string &InServiceName, bool bInIsHealthy)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    FServiceInfo *Service = FindService(InServiceName);
    if (Service)
    {
        Service->bIsHealthy = bInIsHealthy;
        Logger->Info("更新服务健康状态: " + InServiceName + " -> " + (bInIsHealthy ? "健康" : "不健康"));
    }
}

void FServiceAllocateManager::UpdateServiceLoad(const std::string &InServiceName, int InCurrentLoad)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    FServiceInfo *Service = FindService(InServiceName);
    if (Service)
    {
        Service->CurrentLoad = InCurrentLoad;
        Logger->Debug("更新服务负载: " + InServiceName + " -> " + std::to_string(InCurrentLoad));
    }
}

void FServiceAllocateManager::ProcessHeartbeat(const std::string &InServiceName)
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    FServiceInfo *Service = FindService(InServiceName);
    if (Service)
    {
        Service->LastHeartbeat = std::chrono::system_clock::now();
        Service->bIsHealthy = true;
        Logger->Debug("处理心跳: " + InServiceName);
    }
}

void FServiceAllocateManager::HealthCheckThread()
{
    while (bRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(HealthCheckInterval));
        CleanupUnhealthyServices();
    }
}

void FServiceAllocateManager::CleanupUnhealthyServices()
{
    std::lock_guard<std::mutex> Lock(ServicesMutex);

    auto Now = std::chrono::system_clock::now();
    std::vector<std::string> servicesToRemove;

    for (auto &Pair : Services)
    {
        FServiceInfo &Service = Pair.second;
        auto TimeSinceHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(Now - Service.LastHeartbeat).count();

        if (TimeSinceHeartbeat > HeartbeatTimeout)
        {
            Service.bIsHealthy = false;
            Logger->Warn("服务心跳超时: " + Service.ServiceName + " (超时: " + std::to_string(TimeSinceHeartbeat) +
                         "s)");
        }
    }
}

FServiceInfo *FServiceAllocateManager::FindService(const std::string &InServiceName)
{
    auto It = Services.find(InServiceName);
    return (It != Services.end()) ? &It->second : nullptr;
}

FServiceInfo *FServiceAllocateManager::SelectBestService(const std::string &InServiceType)
{
    auto It = ServiceTypeIndex.find(InServiceType);
    if (It == ServiceTypeIndex.end())
    {
        return nullptr;
    }

    std::vector<FServiceInfo *> AvailableServices;

    // 收集健康的服务
    for (const auto &ServiceName : It->second)
    {
        FServiceInfo *Service = FindService(ServiceName);
        if (Service && Service->bIsHealthy && Service->CurrentLoad < Service->MaxLoad)
        {
            AvailableServices.push_back(Service);
        }
    }

    if (AvailableServices.empty())
    {
        return nullptr;
    }

    // 简单的负载均衡策略：选择负载最低的服务
    auto BestService =
        std::min_element(AvailableServices.begin(), AvailableServices.end(),
                         [](const FServiceInfo *A, const FServiceInfo *B) { return A->CurrentLoad < B->CurrentLoad; });

    return *BestService;
}
