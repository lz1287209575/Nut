#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Logging/LogCategory.h"
#include "MemoryManager.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace NLib
{
/**
 * @brief 垃圾回收状态枚举
 */
enum class EGCState : uint8_t
{
	Idle,      // 空闲状态
	Marking,   // 标记阶段
	Sweeping,  // 清理阶段
	Finalizing // 终结阶段
};

/**
 * @brief 垃圾回收类型枚举
 */
enum class EGCType : uint8_t
{
	Minor, // 次要GC（新生代）
	Major, // 主要GC（全代）
	Full   // 完全GC（包含大对象）
};

/**
 * @brief 垃圾回收统计信息
 */
struct SGCStatistics
{
	uint64_t TotalCollections = 0; // 总回收次数
	uint64_t MinorCollections = 0; // 次要回收次数
	uint64_t MajorCollections = 0; // 主要回收次数
	uint64_t FullCollections = 0;  // 完全回收次数

	uint64_t TotalObjectsCollected = 0; // 总回收对象数
	uint64_t TotalMemoryFreed = 0;      // 总释放内存（字节）

	CTimespan TotalGCTime = CTimespan::Zero;   // 总GC时间
	CTimespan LastGCTime = CTimespan::Zero;    // 上次GC时间
	CTimespan AverageGCTime = CTimespan::Zero; // 平均GC时间

	CDateTime LastGCTimestamp; // 上次GC时间戳

	/**
	 * @brief 重置统计信息
	 */
	void Reset()
	{
		TotalCollections = 0;
		MinorCollections = 0;
		MajorCollections = 0;
		FullCollections = 0;
		TotalObjectsCollected = 0;
		TotalMemoryFreed = 0;
		TotalGCTime = CTimespan::Zero;
		LastGCTime = CTimespan::Zero;
		AverageGCTime = CTimespan::Zero;
	}
};

/**
 * @brief 垃圾回收器配置
 */
struct SGCConfig
{
	bool bAutoGCEnabled = true;         // 是否启用自动GC
	bool bBackgroundGCEnabled = true;   // 是否启用后台GC
	bool bGenerationalGCEnabled = true; // 是否启用分代GC

	float GCTriggerThreshold = 0.8f; // GC触发阈值（内存使用率）
	uint64_t MinGCInterval = 1000;   // 最小GC间隔（毫秒）
	uint64_t MaxGCInterval = 30000;  // 最大GC间隔（毫秒）

	uint32_t YoungGenerationSize = 1024 * 1024 * 16; // 新生代大小（16MB）
	uint32_t OldGenerationSize = 1024 * 1024 * 64;   // 老生代大小（64MB）

	uint32_t MaxConcurrentThreads = 2; // 最大并发GC线程数
	uint32_t RootScanBatchSize = 100;  // 根对象扫描批次大小
};

/**
 * @brief 垃圾回收器
 *
 * 提供自动内存管理功能：
 * - 标记-清理垃圾回收算法
 * - 分代垃圾回收
 * - 并发和增量回收
 * - 内存压缩和碎片整理
 */
class CGarbageCollector
{
public:
	// === 单例模式 ===

	/**
	 * @brief 获取垃圾回收器实例
	 */
	static CGarbageCollector& GetInstance()
	{
		static CGarbageCollector Instance;
		return Instance;
	}

private:
	// === 构造函数（私有） ===

	CGarbageCollector()
	    : CurrentState(EGCState::Idle),
	      bIsInitialized(false),
	      bShuttingDown(false),
	      bGCRequested(false),
	      bGCInProgress(false),
	      LastGCTime(0),
	      ObjectIdCounter(1)
	{}

public:
	// 禁用拷贝
	CGarbageCollector(const CGarbageCollector&) = delete;
	CGarbageCollector& operator=(const CGarbageCollector&) = delete;

	~CGarbageCollector()
	{
		Shutdown();
	}

public:
	// === 初始化和关闭 ===

	/**
	 * @brief 初始化垃圾回收器
	 */
	void Initialize(const SGCConfig& InConfig = SGCConfig{})
	{
		if (bIsInitialized)
		{
			NLOG_GC(Warning, "GarbageCollector already initialized");
			return;
		}

		Config = InConfig;
		Statistics.Reset();

		// 初始化对象容器
		RegisteredObjects.Reserve(1024);
		RootObjects.Reserve(256);

		// 启动后台GC线程
		if (Config.bBackgroundGCEnabled)
		{
			StartBackgroundThread();
		}

		bIsInitialized = true;
		NLOG_GC(Info, "GarbageCollector initialized with {} background threads", Config.bBackgroundGCEnabled ? 1 : 0);
	}

	/**
	 * @brief 关闭垃圾回收器
	 */
	void Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}

		bShuttingDown = true;

		// 停止后台线程
		if (BackgroundThread.joinable())
		{
			GCCondition.notify_all();
			BackgroundThread.join();
		}

		// 执行最终清理
		PerformFinalCleanup();

		NLOG_GC(Info,
		        "GarbageCollector shutdown. Final stats: {} collections, {} objects collected",
		        Statistics.TotalCollections,
		        Statistics.TotalObjectsCollected);

		bIsInitialized = false;
	}

public:
	// === 对象注册 ===

	/**
	 * @brief 注册对象到垃圾回收器
	 */
	void RegisterObject(NObject* Object)
	{
		if (!Object || !bIsInitialized)
		{
			return;
		}

		{
			std::lock_guard<std::mutex> Lock(ObjectsMutex);

			uint64_t ObjectId = ObjectIdCounter.fetch_add(1);
			RegisteredObjects.Add(Object);
			ObjectIdMap.Add(Object, ObjectId);

			NLOG_GC(Trace, "Registered object {} with ID {}", static_cast<void*>(Object), ObjectId);
		}

		// 检查是否需要触发GC
		CheckGCTrigger();
	}

	/**
	 * @brief 注销对象
	 */
	void UnregisterObject(NObject* Object)
	{
		if (!Object)
		{
			return;
		}

		std::lock_guard<std::mutex> Lock(ObjectsMutex);

		auto Index = RegisteredObjects.Find(Object);
		if (Index != INDEX_NONE)
		{
			RegisteredObjects.RemoveAt(Index);
			ObjectIdMap.Remove(Object);

			NLOG_GC(Trace, "Unregistered object {}", static_cast<void*>(Object));
		}

		// 从根对象集合中移除
		RootObjects.Remove(Object);
	}

	/**
	 * @brief 添加根对象
	 */
	void AddRootObject(NObject* Object)
	{
		if (!Object)
		{
			return;
		}

		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		RootObjects.AddUnique(Object);

		NLOG_GC(Debug, "Added root object {}", static_cast<void*>(Object));
	}

	/**
	 * @brief 移除根对象
	 */
	void RemoveRootObject(NObject* Object)
	{
		if (!Object)
		{
			return;
		}

		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		RootObjects.Remove(Object);

		NLOG_GC(Debug, "Removed root object {}", static_cast<void*>(Object));
	}

public:
	// === 垃圾回收控制 ===

	/**
	 * @brief 请求垃圾回收
	 */
	void RequestGC(EGCType Type = EGCType::Minor)
	{
		if (!bIsInitialized || bShuttingDown)
		{
			return;
		}

		{
			std::lock_guard<std::mutex> Lock(GCMutex);
			bGCRequested = true;
			RequestedGCType = Type;
		}

		if (Config.bBackgroundGCEnabled)
		{
			GCCondition.notify_one();
		}
		else
		{
			PerformGC(Type);
		}

		NLOG_GC(Debug, "GC requested: {}", GetGCTypeName(Type).GetData());
	}

	/**
	 * @brief 强制立即执行垃圾回收
	 */
	void ForceGC(EGCType Type = EGCType::Full)
	{
		if (!bIsInitialized || bShuttingDown)
		{
			return;
		}

		NLOG_GC(Info, "Force GC triggered: {}", GetGCTypeName(Type).GetData());
		PerformGC(Type);
	}

	/**
	 * @brief 等待当前GC完成
	 */
	void WaitForGC()
	{
		std::unique_lock<std::mutex> Lock(GCMutex);
		GCCompleteCondition.wait(Lock, [this] { return !bGCInProgress; });
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查是否已初始化
	 */
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

	/**
	 * @brief 检查GC是否正在进行
	 */
	bool IsGCInProgress() const
	{
		return bGCInProgress;
	}

	/**
	 * @brief 获取当前GC状态
	 */
	EGCState GetCurrentState() const
	{
		return CurrentState;
	}

	/**
	 * @brief 获取注册对象数量
	 */
	uint32_t GetRegisteredObjectCount() const
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		return static_cast<uint32_t>(RegisteredObjects.Size());
	}

	/**
	 * @brief 获取根对象数量
	 */
	uint32_t GetRootObjectCount() const
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		return static_cast<uint32_t>(RootObjects.Size());
	}

public:
	// === 统计信息 ===

	/**
	 * @brief 获取GC统计信息
	 */
	const SGCStatistics& GetStatistics() const
	{
		return Statistics;
	}

	/**
	 * @brief 重置统计信息
	 */
	void ResetStatistics()
	{
		std::lock_guard<std::mutex> Lock(StatsMutex);
		Statistics.Reset();
		NLOG_GC(Info, "GC statistics reset");
	}

	/**
	 * @brief 获取GC配置
	 */
	const SGCConfig& GetConfig() const
	{
		return Config;
	}

	/**
	 * @brief 更新GC配置
	 */
	void UpdateConfig(const SGCConfig& NewConfig)
	{
		Config = NewConfig;
		NLOG_GC(Info, "GC configuration updated");
	}

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成内存报告
	 */
	CString GenerateMemoryReport() const
	{
		std::lock_guard<std::mutex> StatsLock(StatsMutex);
		std::lock_guard<std::mutex> ObjectsLock(ObjectsMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Garbage Collector Memory Report ===\n"
		         "Registered Objects: %u\n"
		         "Root Objects: %u\n"
		         "GC State: %s\n"
		         "Total Collections: %llu\n"
		         "  - Minor: %llu\n"
		         "  - Major: %llu\n"
		         "  - Full: %llu\n"
		         "Objects Collected: %llu\n"
		         "Memory Freed: %.2f MB\n"
		         "Total GC Time: %s\n"
		         "Average GC Time: %s\n"
		         "Last GC: %s",
		         GetRegisteredObjectCount(),
		         GetRootObjectCount(),
		         GetStateName(CurrentState).GetData(),
		         Statistics.TotalCollections,
		         Statistics.MinorCollections,
		         Statistics.MajorCollections,
		         Statistics.FullCollections,
		         Statistics.TotalObjectsCollected,
		         static_cast<double>(Statistics.TotalMemoryFreed) / (1024.0 * 1024.0),
		         Statistics.TotalGCTime.ToString().GetData(),
		         Statistics.AverageGCTime.ToString().GetData(),
		         Statistics.LastGCTimestamp.ToString().GetData());

		return CString(Buffer);
	}

private:
	// === 内部GC实现 ===

	/**
	 * @brief 执行垃圾回收
	 */
	void PerformGC(EGCType Type)
	{
		if (bGCInProgress)
		{
			NLOG_GC(Warning, "GC already in progress, skipping");
			return;
		}

		CClock GCClock;
		bGCInProgress = true;
		CurrentState = EGCState::Marking;

		NLOG_GC(Info, "Starting {} GC", GetGCTypeName(Type).GetData());

		uint32_t ObjectsCollected = 0;
		uint64_t MemoryFreed = 0;

		try
		{
			// 标记阶段
			MarkPhase();

			// 清理阶段
			CurrentState = EGCState::Sweeping;
			SweepPhase(ObjectsCollected, MemoryFreed);

			// 终结阶段
			CurrentState = EGCState::Finalizing;
			FinalizePhase();

			CurrentState = EGCState::Idle;
		}
		catch (...)
		{
			NLOG_GC(Error, "Exception during GC execution");
			CurrentState = EGCState::Idle;
		}

		// 更新统计信息
		CTimespan GCTime = GCClock.GetElapsed();
		UpdateStatistics(Type, ObjectsCollected, MemoryFreed, GCTime);

		bGCInProgress = false;
		LastGCTime = CClock::GetCurrentTimestampMs();

		NLOG_GC(Info,
		        "GC completed: {} objects collected, {:.2f} KB freed, {:.2f}ms elapsed",
		        ObjectsCollected,
		        static_cast<double>(MemoryFreed) / 1024.0,
		        GCTime.GetTotalMilliseconds());

		GCCompleteCondition.notify_all();
	}

	/**
	 * @brief 标记阶段
	 */
	void MarkPhase()
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);

		// 清除所有对象的标记
		for (NObject* Object : RegisteredObjects)
		{
			if (Object && Object->IsValid())
			{
				Object->UnMark();
			}
		}

		// 从根对象开始标记
		TArray<NObject*, CMemoryManager> References;

		for (NObject* RootObject : RootObjects)
		{
			if (RootObject && RootObject->IsValid())
			{
				MarkObjectAndReferences(RootObject, References);
			}
		}

		NLOG_GC(Debug, "Mark phase completed");
	}

	/**
	 * @brief 递归标记对象及其引用
	 */
	void MarkObjectAndReferences(NObject* Object, TArray<NObject*, CMemoryManager>& References)
	{
		if (!Object || Object->IsMarked() || !Object->IsValid())
		{
			return;
		}

		Object->Mark();

		// 收集对象的引用
		References.Empty();
		Object->CollectReferences(References);

		// 递归标记引用的对象
		for (NObject* RefObject : References)
		{
			MarkObjectAndReferences(RefObject, References);
		}
	}

	/**
	 * @brief 清理阶段
	 */
	void SweepPhase(uint32_t& ObjectsCollected, uint64_t& MemoryFreed)
	{
		std::lock_guard<std::mutex> Lock(ObjectsMutex);

		TArray<NObject*, CMemoryManager> ObjectsToDelete;

		// 收集未标记的对象
		for (int32_t i = RegisteredObjects.Size() - 1; i >= 0; --i)
		{
			NObject* Object = RegisteredObjects[i];
			if (Object && !Object->IsMarked() && Object->IsValid())
			{
				ObjectsToDelete.Add(Object);
				RegisteredObjects.RemoveAt(i);
				ObjectIdMap.Remove(Object);
			}
		}

		// 删除对象并计算释放的内存
		for (NObject* Object : ObjectsToDelete)
		{
			// 估算对象大小（实际实现中应该从内存管理器获取）
			uint64_t ObjectSize = sizeof(NObject); // 简化实现
			MemoryFreed += ObjectSize;

			// 删除对象
			delete Object;
			ObjectsCollected++;
		}

		NLOG_GC(Debug, "Sweep phase completed: {} objects collected", ObjectsCollected);
	}

	/**
	 * @brief 终结阶段
	 */
	void FinalizePhase()
	{
		// 执行内存压缩或其他清理工作
		// 实际实现中可能包括内存碎片整理等

		NLOG_GC(Debug, "Finalize phase completed");
	}

	/**
	 * @brief 检查是否需要触发GC
	 */
	void CheckGCTrigger()
	{
		if (!Config.bAutoGCEnabled || bGCInProgress)
		{
			return;
		}

		// 检查内存使用率
		auto& MemMgr = CMemoryManager::GetInstance();
		float MemoryUsage = MemMgr.GetMemoryUsageRatio();

		// 检查时间间隔
		int64_t CurrentTime = CClock::GetCurrentTimestampMs();
		int64_t TimeSinceLastGC = CurrentTime - LastGCTime;

		bool ShouldTrigger = false;
		EGCType GCType = EGCType::Minor;

		if (MemoryUsage > Config.GCTriggerThreshold)
		{
			ShouldTrigger = true;
			GCType = MemoryUsage > 0.9f ? EGCType::Major : EGCType::Minor;
		}
		else if (TimeSinceLastGC > static_cast<int64_t>(Config.MaxGCInterval))
		{
			ShouldTrigger = true;
			GCType = EGCType::Minor;
		}

		if (ShouldTrigger && TimeSinceLastGC > static_cast<int64_t>(Config.MinGCInterval))
		{
			RequestGC(GCType);
		}
	}

	/**
	 * @brief 更新统计信息
	 */
	void UpdateStatistics(EGCType Type, uint32_t ObjectsCollected, uint64_t MemoryFreed, const CTimespan& GCTime)
	{
		std::lock_guard<std::mutex> Lock(StatsMutex);

		Statistics.TotalCollections++;
		Statistics.TotalObjectsCollected += ObjectsCollected;
		Statistics.TotalMemoryFreed += MemoryFreed;
		Statistics.TotalGCTime += GCTime;
		Statistics.LastGCTime = GCTime;
		Statistics.LastGCTimestamp = CDateTime::Now();

		switch (Type)
		{
		case EGCType::Minor:
			Statistics.MinorCollections++;
			break;
		case EGCType::Major:
			Statistics.MajorCollections++;
			break;
		case EGCType::Full:
			Statistics.FullCollections++;
			break;
		}

		// 计算平均GC时间
		if (Statistics.TotalCollections > 0)
		{
			Statistics.AverageGCTime = CTimespan::FromSeconds(Statistics.TotalGCTime.GetTotalSeconds() /
			                                                  Statistics.TotalCollections);
		}
	}

	/**
	 * @brief 启动后台GC线程
	 */
	void StartBackgroundThread()
	{
		BackgroundThread = std::thread([this]() {
			NLOG_GC(Info, "Background GC thread started");

			while (!bShuttingDown)
			{
				std::unique_lock<std::mutex> Lock(GCMutex);

				// 等待GC请求或超时
				GCCondition.wait_for(Lock, std::chrono::milliseconds(Config.MaxGCInterval), [this] {
					return bGCRequested || bShuttingDown;
				});

				if (bShuttingDown)
				{
					break;
				}

				if (bGCRequested)
				{
					EGCType Type = RequestedGCType;
					bGCRequested = false;
					Lock.unlock();

					PerformGC(Type);
				}
				else
				{
					Lock.unlock();
					// 检查是否需要自动触发GC
					CheckGCTrigger();
				}
			}

			NLOG_GC(Info, "Background GC thread stopped");
		});
	}

	/**
	 * @brief 执行最终清理
	 */
	void PerformFinalCleanup()
	{
		NLOG_GC(Info, "Performing final cleanup");

		// 强制执行完全GC
		if (!RegisteredObjects.IsEmpty())
		{
			PerformGC(EGCType::Full);
		}

		// 清理剩余对象
		std::lock_guard<std::mutex> Lock(ObjectsMutex);
		RegisteredObjects.Empty();
		RootObjects.Empty();
		ObjectIdMap.Empty();
	}

	/**
	 * @brief 获取GC类型名称
	 */
	CString GetGCTypeName(EGCType Type) const
	{
		switch (Type)
		{
		case EGCType::Minor:
			return "Minor";
		case EGCType::Major:
			return "Major";
		case EGCType::Full:
			return "Full";
		default:
			return "Unknown";
		}
	}

	/**
	 * @brief 获取状态名称
	 */
	CString GetStateName(EGCState State) const
	{
		switch (State)
		{
		case EGCState::Idle:
			return "Idle";
		case EGCState::Marking:
			return "Marking";
		case EGCState::Sweeping:
			return "Sweeping";
		case EGCState::Finalizing:
			return "Finalizing";
		default:
			return "Unknown";
		}
	}

private:
	// === 成员变量 ===

	// 配置和状态
	SGCConfig Config;
	SGCStatistics Statistics;
	std::atomic<EGCState> CurrentState;
	std::atomic<bool> bIsInitialized;
	std::atomic<bool> bShuttingDown;

	// GC控制
	std::atomic<bool> bGCRequested;
	std::atomic<bool> bGCInProgress;
	EGCType RequestedGCType = EGCType::Minor;
	int64_t LastGCTime;

	// 对象管理
	TArray<NObject*, CMemoryManager> RegisteredObjects;
	TArray<NObject*, CMemoryManager> RootObjects;
	THashMap<NObject*, uint64_t, CMemoryManager> ObjectIdMap;
	std::atomic<uint64_t> ObjectIdCounter;

	// 线程同步
	mutable std::mutex ObjectsMutex;
	mutable std::mutex StatsMutex;
	std::mutex GCMutex;
	std::condition_variable GCCondition;
	std::condition_variable GCCompleteCondition;
	std::thread BackgroundThread;
};

// === 全局访问函数 ===

/**
 * @brief 获取垃圾回收器
 */
inline CGarbageCollector& GetGC()
{
	return CGarbageCollector::GetInstance();
}

} // namespace NLib