#include "CGarbageCollector.h"

#include "Containers/CArray.h"
#include "Containers/CSet.h"
#include "Core/CObject.h"
#include "Logging/CLogger.h"
#include "CMemoryManager.h"

#include <algorithm>
#include <chrono>
#include <string>

namespace NLib
{

CGarbageCollector::CGarbageCollector()
{
	// 初始化容器
	RegisteredObjects = std::make_unique<CSet<CObject*>>();
	RootObjects = std::make_unique<CSet<CObject*>>();
	MarkStack = std::make_unique<CArray<CObject*>>();
	ObjectsToDelete = std::make_unique<CArray<CObject*>>();

	// 预分配容量以提高性能
	MarkStack->Reserve(1024);
	ObjectsToDelete->Reserve(256);

	CLogger::Info("CGarbageCollector created");
}

CGarbageCollector::~CGarbageCollector()
{
	if (bInitialized && !bShutdown)
	{
		Shutdown();
	}

	CLogger::Info("CGarbageCollector destroyed");
}

CGarbageCollector& CGarbageCollector::GetInstance()
{
	static CGarbageCollector Instance;
	return Instance;
}

void CGarbageCollector::Initialize(EGCMode Mode, uint32_t CollectionIntervalMs, bool bEnableBackgroundCollection)
{
	if (bInitialized)
	{
		CLogger::Warn("CGarbageCollector already initialized");
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
		BackgroundThread = std::make_unique<std::thread>(&CGarbageCollector::BackgroundCollectionThread, this);
		CLogger::Info("GC Background thread started");
	}

	bInitialized = true;

	CLogger::Info("CGarbageCollector initialized with mode: " + std::to_string(static_cast<int>(Mode)) +
	              ", interval: " + std::to_string(CollectionIntervalMs) + "ms");
}

void CGarbageCollector::Shutdown()
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
		CLogger::Info("GC Background thread stopped");
	}

	// 执行最后一次完整回收
	CLogger::Info("Performing final garbage collection...");
	uint32_t FinalCollected = Collect(true);

	// 清理剩余对象
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		if (!RegisteredObjects->IsEmpty())
		{
			CLogger::Warn("GC Shutdown: " + std::to_string(RegisteredObjects->GetSize()) + " objects still registered");
		}
		RegisteredObjects->Clear();
		RootObjects->Clear();
	}

	CLogger::Info("CGarbageCollector shutdown completed. Final collection recovered " + std::to_string(FinalCollected) +
	              " objects");
}

void CGarbageCollector::RegisterObject(CObject* Object)
{
	if (Object == nullptr)
	{
		return;
	}

	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	RegisteredObjects->Insert(Object);

	CLogger::Debug("GC Registered object ID: " + std::to_string(Object->GetObjectId()));
}

void CGarbageCollector::UnregisterObject(CObject* Object)
{
	if (Object == nullptr)
	{
		return;
	}

	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	RegisteredObjects->Erase(Object);
	RootObjects->Erase(Object); // 也从根对象中移除

	CLogger::Debug("GC Unregistered object ID: " + std::to_string(Object->GetObjectId()));
}

size_t CGarbageCollector::GetRegisteredObjectCount() const
{
	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	return RegisteredObjects->GetSize();
}

uint32_t CGarbageCollector::Collect(bool bForceFullCollection)
{
	if (!bInitialized || bShutdown)
	{
		return 0;
	}

	// 防止并发收集
	std::lock_guard<std::mutex> CollectionLock(CollectionMutex);

	if (bIsCollecting)
	{
		CLogger::Debug("GC collection already in progress, skipping");
		return 0;
	}

	bIsCollecting = true;
	auto StartTime = std::chrono::steady_clock::now();

	CLogger::Info("GC Starting collection (Force: " + std::string(bForceFullCollection ? "true" : "false") + ")");

	uint32_t TotalCollected = 0;

	try
	{
		// 标记阶段
		uint32_t MarkedObjects = MarkPhase();
		CLogger::Debug("GC Mark phase completed: " + std::to_string(MarkedObjects) + " objects marked");

		// 清扫阶段
		uint32_t SweptObjects = SweepPhase();
		CLogger::Debug("GC Sweep phase completed: " + std::to_string(SweptObjects) + " objects collected");

		TotalCollected = SweptObjects;

		// 计算耗时
		auto EndTime = std::chrono::steady_clock::now();
		auto DurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime).count();

		// 更新统计信息
		UpdateStats(TotalCollected, static_cast<uint64_t>(DurationMs));

		CLogger::Info("GC Collection completed: " + std::to_string(TotalCollected) + " objects collected in " +
		              std::to_string(DurationMs) + "ms");
	}
	catch (const std::exception& e)
	{
		CLogger::Error("GC Collection failed: " + std::string(e.what()));
	}

	bIsCollecting = false;
	return TotalCollected;
}

void CGarbageCollector::CollectAsync()
{
	if (!bInitialized || bShutdown)
	{
		return;
	}

	// 通知后台线程执行回收
	CollectionCondition.notify_one();
}

void CGarbageCollector::SetRootObjects(const CArray<CObject*>& RootObjectList)
{
	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	RootObjects->Clear();
	for (CObject* Obj : RootObjectList)
	{
		if (Obj != nullptr && Obj->IsValid())
		{
			RootObjects->Insert(Obj);
		}
	}

	CLogger::Info("GC Root objects set: " + std::to_string(RootObjects->GetSize()) + " objects");
}

void CGarbageCollector::AddRootObject(CObject* Object)
{
	if (Object == nullptr || !Object->IsValid())
	{
		return;
	}

	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	RootObjects->Insert(Object);

	CLogger::Debug("GC Added root object ID: " + std::to_string(Object->GetObjectId()));
}

void CGarbageCollector::RemoveRootObject(CObject* Object)
{
	if (Object == nullptr)
	{
		return;
	}

	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	RootObjects->Erase(Object);

	CLogger::Debug("GC Removed root object ID: " + std::to_string(Object->GetObjectId()));
}

void CGarbageCollector::SetGCMode(EGCMode Mode)
{
	CurrentMode = Mode;
	CLogger::Info("GC Mode changed to: " + std::to_string(static_cast<int>(Mode)));
}

void CGarbageCollector::SetCollectionInterval(uint32_t IntervalMs)
{
	CollectionIntervalMs = IntervalMs;
	CLogger::Info("GC Collection interval set to: " + std::to_string(IntervalMs) + "ms");
}

void CGarbageCollector::SetMemoryThreshold(size_t ThresholdBytes)
{
	MemoryThreshold = ThresholdBytes;
	CLogger::Info("GC Memory threshold set to: " + std::to_string(ThresholdBytes) + " bytes");
}

void CGarbageCollector::SetIncrementalCollection(bool bEnable)
{
	bEnableIncrementalCollection = bEnable;
	CLogger::Info("GC Incremental collection " + std::string(bEnable ? "enabled" : "disabled"));
}

CGarbageCollector::GCStats CGarbageCollector::GetStats() const
{
	std::lock_guard<std::mutex> Lock(StatsMutex);
	GCStats CurrentStats = Stats;
	CurrentStats.ObjectsAlive = GetRegisteredObjectCount();
	return CurrentStats;
}

void CGarbageCollector::ResetStats()
{
	std::lock_guard<std::mutex> Lock(StatsMutex);
	Stats = {};
	CLogger::Info("GC Statistics reset");
}

uint32_t CGarbageCollector::MarkPhase()
{
	uint32_t MarkedCount = 0;

	// 清除所有对象的标记
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		for (CObject* Obj : *RegisteredObjects)
		{
			if (Obj != nullptr && Obj->IsValid())
			{
				Obj->UnMark();
			}
		}
	}

	// 从根对象开始标记
	std::lock_guard<std::mutex> Lock(ObjectsMutex);
	for (CObject* RootObj : *RootObjects)
	{
		if (RootObj != nullptr && RootObj->IsValid() && !RootObj->IsMarked())
		{
			MarkFromRoot(RootObj, MarkedCount);
		}
	}

	// 标记引用计数大于0的对象（这些对象被外部引用，不应被回收）
	for (CObject* Obj : *RegisteredObjects)
	{
		if (Obj != nullptr && Obj->IsValid() && Obj->GetRefCount() > 0 && !Obj->IsMarked())
		{
			MarkFromRoot(Obj, MarkedCount);
		}
	}

	return MarkedCount;
}

uint32_t CGarbageCollector::SweepPhase()
{
	ObjectsToDelete->Clear();

	// 收集未标记的对象
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		for (CObject* Obj : *RegisteredObjects)
		{
			if (Obj != nullptr && Obj->IsValid() && !Obj->IsMarked())
			{
				ObjectsToDelete->PushBack(Obj);
			}
		}
	}

	// 删除未标记的对象
	uint32_t SweptCount = 0;
	for (CObject* Obj : *ObjectsToDelete)
	{
		if (Obj != nullptr && Obj->IsValid())
		{
			CLogger::Debug("GC Sweeping object ID: " + std::to_string(Obj->GetObjectId()));

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

void CGarbageCollector::MarkFromRoot(CObject* Object, uint32_t& MarkedCount)
{
	if (Object == nullptr || !Object->IsValid() || Object->IsMarked())
	{
		return;
	}

	// 使用栈来避免递归，防止栈溢出
	MarkStack->Clear();
	MarkStack->PushBack(Object);

	while (!(MarkStack->GetSize() == 0))
	{
		CObject* Current = MarkStack->Back();
		MarkStack->PopBack();

		if ((Current == nullptr) || !Current->IsValid() || Current->IsMarked())
		{
			continue;
		}

		// 标记当前对象
		Current->Mark();
		MarkedCount++;

		// 收集当前对象引用的其他对象
		CArray<CObject*> References;
		Current->CollectReferences(References);

		// 将引用的对象加入标记栈
		for (CObject* RefObj : References)
		{
			if (RefObj != nullptr && RefObj->IsValid() && !RefObj->IsMarked())
			{
				MarkStack->PushBack(RefObj);
			}
		}
	}
}

void CGarbageCollector::BackgroundCollectionThread()
{
	CLogger::Info("GC Background collection thread started");

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
			CollectionCondition.wait_for(
			    Lock, std::chrono::milliseconds(CollectionIntervalMs), [this] { return bShutdown.load(); });
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

	CLogger::Info("GC Background collection thread ended");
}

bool CGarbageCollector::ShouldTriggerCollection() const
{
	if (CurrentMode == EGCMode::Manual)
	{
		return false;
	}

	// 检查内存使用情况
	if (CurrentMode == EGCMode::Adaptive)
	{
		auto& MemMgr = CMemoryManager::GetInstance();
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

void CGarbageCollector::UpdateStats(uint32_t CollectedObjects, uint64_t CollectionTimeMs)
{
	std::lock_guard<std::mutex> Lock(StatsMutex);

	Stats.TotalCollections++;
	Stats.ObjectsCollected += CollectedObjects;
	Stats.LastCollectionTime = CollectionTimeMs;
	Stats.TotalCollectionTime += CollectionTimeMs;
	Stats.LastCollectionTimestamp = std::chrono::steady_clock::now();

	// 估算回收的内存（假设每个对象平均占用64字节）
	Stats.BytesReclaimed += (CollectedObjects * 64);
}

} // namespace NLib
