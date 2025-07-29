#include "NGarbageCollector.h"
#include "Logging/NLogger.h"
#include "NMemoryManager.h"
#include "Core/NObject.h"
#include <algorithm>
#include <chrono>

namespace Nut
{

NGarbageCollector::NGarbageCollector()
{
    // 预分配容量以提高性能
    MarkStack.reserve(1024);
    ObjectsToDelete.reserve(256);

    NLogger::Info("NGarbageCollector created");
}

NGarbageCollector::~NGarbageCollector()
{
    if (bInitialized && !bShutdown)
    {
        Shutdown();
    }

    NLogger::Info("NGarbageCollector destroyed");
}

NGarbageCollector &NGarbageCollector::GetInstance()
{
    static NGarbageCollector Instance;
    return Instance;
}

void NGarbageCollector::Initialize(EGCMode Mode, uint32_t CollectionIntervalMs, bool bEnableBackgroundCollection)
{
    if (bInitialized)
    {
        NLogger::Warn("NGarbageCollector already initialized");
        return;
    }

    CurrentMode = Mode;
    this->CollectionIntervalMs = CollectionIntervalMs;
    bEnableBackgroundThread = bEnableBackgroundCollection;

    // 重置统计信息
    {
        std::lock_guard<std::mutex> Lock(StatsMutex);
        Stats = {};
    }

    // 启动后台线程
    if (bEnableBackgroundThread && (Mode == EGCMode::Automatic || Mode == EGCMode::Adaptive))
    {
        BackgroundThread = std::make_unique<std::thread>(&NGarbageCollector::BackgroundCollectionThread, this);
        NLogger::Info("GC Background thread started");
    }

    bInitialized = true;

    NLogger::Info("NGarbageCollector initialized with mode: " + std::to_string(static_cast<int>(Mode)) +
                  ", interval: " + std::to_string(CollectionIntervalMs) + "ms");
}

void NGarbageCollector::Shutdown()
{
    if (!bInitialized || bShutdown)
    {
        return;
    }

    bShutdown = true;

    // 停止后台线程
    if (BackgroundThread && BackgroundThread->joinable())
    {
        CollectionCondition.notify_all();
        BackgroundThread->join();
        BackgroundThread.reset();
        NLogger::Info("GC Background thread stopped");
    }

    // 执行最后一次完整回收
    NLogger::Info("Performing final garbage collection...");
    uint32_t FinalCollected = Collect(true);

    // 清理剩余对象
    {
        std::lock_guard<std::mutex> Lock(ObjectsMutex);
        if (!RegisteredObjects.empty())
        {
            NLogger::Warn("GC Shutdown: " + std::to_string(RegisteredObjects.size()) + " objects still registered");
        }
        RegisteredObjects.clear();
        RootObjects.clear();
    }

    NLogger::Info("NGarbageCollector shutdown completed. Final collection recovered " + std::to_string(FinalCollected) +
                  " objects");
}

void NGarbageCollector::RegisterObject(NObject *Object)
{
    if (!Object)
    {
        return;
    }

    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    RegisteredObjects.insert(Object);

    NLogger::Debug("GC Registered object ID: " + std::to_string(Object->GetObjectId()));
}

void NGarbageCollector::UnregisterObject(NObject *Object)
{
    if (!Object)
    {
        return;
    }

    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    RegisteredObjects.erase(Object);
    RootObjects.erase(Object); // 也从根对象中移除

    NLogger::Debug("GC Unregistered object ID: " + std::to_string(Object->GetObjectId()));
}

size_t NGarbageCollector::GetRegisteredObjectCount() const
{
    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    return RegisteredObjects.size();
}

uint32_t NGarbageCollector::Collect(bool bForceFullCollection)
{
    if (!bInitialized || bShutdown)
    {
        return 0;
    }

    // 防止并发收集
    std::lock_guard<std::mutex> CollectionLock(CollectionMutex);

    if (bIsCollecting)
    {
        NLogger::Debug("GC collection already in progress, skipping");
        return 0;
    }

    bIsCollecting = true;
    auto StartTime = std::chrono::steady_clock::now();

    NLogger::Info("GC Starting collection (Force: " + std::string(bForceFullCollection ? "true" : "false") + ")");

    uint32_t TotalCollected = 0;

    try
    {
        // 标记阶段
        uint32_t MarkedObjects = MarkPhase();
        NLogger::Debug("GC Mark phase completed: " + std::to_string(MarkedObjects) + " objects marked");

        // 清扫阶段
        uint32_t SweptObjects = SweepPhase();
        NLogger::Debug("GC Sweep phase completed: " + std::to_string(SweptObjects) + " objects collected");

        TotalCollected = SweptObjects;

        // 计算耗时
        auto EndTime = std::chrono::steady_clock::now();
        auto DurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime).count();

        // 更新统计信息
        UpdateStats(TotalCollected, static_cast<uint64_t>(DurationMs));

        NLogger::Info("GC Collection completed: " + std::to_string(TotalCollected) + " objects collected in " +
                      std::to_string(DurationMs) + "ms");
    }
    catch (const std::exception &e)
    {
        NLogger::Error("GC Collection failed: " + std::string(e.what()));
    }

    bIsCollecting = false;
    return TotalCollected;
}

void NGarbageCollector::CollectAsync()
{
    if (!bInitialized || bShutdown)
    {
        return;
    }

    // 通知后台线程执行回收
    CollectionCondition.notify_one();
}

void NGarbageCollector::SetRootObjects(const NVector<NObject *> &RootObjectList)
{
    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    RootObjects.clear();
    for (NObject *Obj : RootObjectList)
    {
        if (Obj && Obj->IsValid())
        {
            RootObjects.insert(Obj);
        }
    }

    NLogger::Info("GC Root objects set: " + std::to_string(RootObjects.size()) + " objects");
}

void NGarbageCollector::AddRootObject(NObject *Object)
{
    if (!Object || !Object->IsValid())
    {
        return;
    }

    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    RootObjects.insert(Object);

    NLogger::Debug("GC Added root object ID: " + std::to_string(Object->GetObjectId()));
}

void NGarbageCollector::RemoveRootObject(NObject *Object)
{
    if (!Object)
    {
        return;
    }

    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    RootObjects.erase(Object);

    NLogger::Debug("GC Removed root object ID: " + std::to_string(Object->GetObjectId()));
}

void NGarbageCollector::SetGCMode(EGCMode Mode)
{
    CurrentMode = Mode;
    NLogger::Info("GC Mode changed to: " + std::to_string(static_cast<int>(Mode)));
}

void NGarbageCollector::SetCollectionInterval(uint32_t IntervalMs)
{
    CollectionIntervalMs = IntervalMs;
    NLogger::Info("GC Collection interval set to: " + std::to_string(IntervalMs) + "ms");
}

void NGarbageCollector::SetMemoryThreshold(size_t ThresholdBytes)
{
    MemoryThreshold = ThresholdBytes;
    NLogger::Info("GC Memory threshold set to: " + std::to_string(ThresholdBytes) + " bytes");
}

void NGarbageCollector::SetIncrementalCollection(bool bEnable)
{
    bEnableIncrementalCollection = bEnable;
    NLogger::Info("GC Incremental collection " + std::string(bEnable ? "enabled" : "disabled"));
}

NGarbageCollector::GCStats NGarbageCollector::GetStats() const
{
    std::lock_guard<std::mutex> Lock(StatsMutex);
    GCStats CurrentStats = Stats;
    CurrentStats.ObjectsAlive = GetRegisteredObjectCount();
    return CurrentStats;
}

void NGarbageCollector::ResetStats()
{
    std::lock_guard<std::mutex> Lock(StatsMutex);
    Stats = {};
    NLogger::Info("GC Statistics reset");
}

uint32_t NGarbageCollector::MarkPhase()
{
    uint32_t MarkedCount = 0;

    // 清除所有对象的标记
    {
        std::lock_guard<std::mutex> Lock(ObjectsMutex);
        for (NObject *Obj : RegisteredObjects)
        {
            if (Obj && Obj->IsValid())
            {
                Obj->UnMark();
            }
        }
    }

    // 从根对象开始标记
    std::lock_guard<std::mutex> Lock(ObjectsMutex);
    for (NObject *RootObj : RootObjects)
    {
        if (RootObj && RootObj->IsValid() && !RootObj->IsMarked())
        {
            MarkFromRoot(RootObj, MarkedCount);
        }
    }

    // 标记引用计数大于0的对象（这些对象被外部引用，不应被回收）
    for (NObject *Obj : RegisteredObjects)
    {
        if (Obj && Obj->IsValid() && Obj->GetRefCount() > 0 && !Obj->IsMarked())
        {
            MarkFromRoot(Obj, MarkedCount);
        }
    }

    return MarkedCount;
}

uint32_t NGarbageCollector::SweepPhase()
{
    ObjectsToDelete.clear();

    // 收集未标记的对象
    {
        std::lock_guard<std::mutex> Lock(ObjectsMutex);
        for (NObject *Obj : RegisteredObjects)
        {
            if (Obj && Obj->IsValid() && !Obj->IsMarked())
            {
                ObjectsToDelete.push_back(Obj);
            }
        }
    }

    // 删除未标记的对象
    uint32_t SweptCount = 0;
    for (NObject *Obj : ObjectsToDelete)
    {
        if (Obj && Obj->IsValid())
        {
            NLogger::Debug("GC Sweeping object ID: " + std::to_string(Obj->GetObjectId()));

            // 注意：这里不直接delete，而是调用Release()
            // 如果引用计数为0，Release()会自动删除对象
            // 如果引用计数不为0，说明有外部引用，不应删除
            if (Obj->GetRefCount() <= 0)
            {
                delete Obj; // 强制删除无引用的对象
                SweptCount++;
            }
        }
    }

    return SweptCount;
}

void NGarbageCollector::MarkFromRoot(NObject *Object, uint32_t &MarkedCount)
{
    if (!Object || !Object->IsValid() || Object->IsMarked())
    {
        return;
    }

    // 使用栈来避免递归，防止栈溢出
    MarkStack.clear();
    MarkStack.push_back(Object);

    while (!MarkStack.empty())
    {
        NObject *Current = MarkStack.back();
        MarkStack.pop_back();

        if (!Current || !Current->IsValid() || Current->IsMarked())
        {
            continue;
        }

        // 标记当前对象
        Current->Mark();
        MarkedCount++;

        // 收集当前对象引用的其他对象
        NVector<NObject *> References;
        Current->CollectReferences(References);

        // 将引用的对象加入标记栈
        for (NObject *RefObj : References)
        {
            if (RefObj && RefObj->IsValid() && !RefObj->IsMarked())
            {
                MarkStack.push_back(RefObj);
            }
        }
    }
}

void NGarbageCollector::BackgroundCollectionThread()
{
    NLogger::Info("GC Background collection thread started");

    while (!bShutdown)
    {
        std::unique_lock<std::mutex> Lock(CollectionWaitMutex);

        // 等待收集信号或超时
        if (CurrentMode == EGCMode::Manual)
        {
            // 手动模式，只等待显式信号
            CollectionCondition.wait(Lock, [this] { return bShutdown.load(); });
        }
        else
        {
            // 自动或自适应模式，定时检查
            CollectionCondition.wait_for(Lock, std::chrono::milliseconds(CollectionIntervalMs),
                                         [this] { return bShutdown.load(); });
        }

        if (bShutdown)
        {
            break;
        }

        // 检查是否需要触发回收
        if (ShouldTriggerCollection())
        {
            Lock.unlock();
            Collect(false);
        }
    }

    NLogger::Info("GC Background collection thread ended");
}

bool NGarbageCollector::ShouldTriggerCollection() const
{
    if (CurrentMode == EGCMode::Manual)
    {
        return false;
    }

    // 检查内存使用情况
    if (CurrentMode == EGCMode::Adaptive)
    {
        auto &MemMgr = NMemoryManager::GetInstance();
        auto MemStats = MemMgr.GetStats();

        // 如果当前内存使用超过阈值，触发回收
        if (MemStats.CurrentUsage > MemoryThreshold)
        {
            return true;
        }

        // 如果对象数量过多，也触发回收
        size_t ObjectCount = GetRegisteredObjectCount();
        if (ObjectCount > 10000)
        { // 默认对象数量阈值
            return true;
        }
    }

    return true; // 自动模式总是执行定时回收
}

void NGarbageCollector::UpdateStats(uint32_t CollectedObjects, uint64_t CollectionTimeMs)
{
    std::lock_guard<std::mutex> Lock(StatsMutex);

    Stats.TotalCollections++;
    Stats.ObjectsCollected += CollectedObjects;
    Stats.LastCollectionTime = CollectionTimeMs;
    Stats.TotalCollectionTime += CollectionTimeMs;
    Stats.LastCollectionTimestamp = std::chrono::steady_clock::now();

    // 估算回收的内存（假设每个对象平均占用64字节）
    Stats.BytesReclaimed += CollectedObjects * 64;
}

} // namespace Nut
