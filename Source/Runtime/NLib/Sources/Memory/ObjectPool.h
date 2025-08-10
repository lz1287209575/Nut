#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "MemoryManager.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <mutex>
#include <type_traits>

namespace NLib
{
// 常量定义
constexpr int32_t INDEX_NONE = -1;

/**
 * @brief 对象池策略枚举
 */
enum class EPoolStrategy : uint8_t
{
	FixedSize, // 固定大小池
	Dynamic,   // 动态扩展池
	LRU,       // 最近最少使用策略
	Circular   // 循环使用策略
};

/**
 * @brief 对象池统计信息
 */
struct SObjectPoolStats
{
	uint32_t PoolSize = 0;           // 池大小
	uint32_t ActiveObjects = 0;      // 活跃对象数
	uint32_t AvailableObjects = 0;   // 可用对象数
	uint32_t TotalAllocations = 0;   // 总分配次数
	uint32_t TotalDeallocations = 0; // 总释放次数
	uint32_t CacheHits = 0;          // 缓存命中次数
	uint32_t CacheMisses = 0;        // 缓存未命中次数
	uint32_t PoolExpansions = 0;     // 池扩展次数
	uint32_t PoolShrinks = 0;        // 池收缩次数

	CDateTime CreationTime;   // 创建时间
	CDateTime LastAccessTime; // 最后访问时间

	/**
	 * @brief 获取缓存命中率
	 */
	float GetHitRatio() const
	{
		uint32_t TotalRequests = CacheHits + CacheMisses;
		return TotalRequests > 0 ? static_cast<float>(CacheHits) / TotalRequests : 0.0f;
	}

	/**
	 * @brief 重置统计信息
	 */
	void Reset()
	{
		TotalAllocations = 0;
		TotalDeallocations = 0;
		CacheHits = 0;
		CacheMisses = 0;
		PoolExpansions = 0;
		PoolShrinks = 0;
		LastAccessTime = CDateTime::Now();
	}
};

/**
 * @brief 对象池配置
 */
struct SObjectPoolConfig
{
	EPoolStrategy Strategy = EPoolStrategy::Dynamic;    // 池策略
	uint32_t InitialSize = 16;                          // 初始大小
	uint32_t MaxSize = 1024;                            // 最大大小
	uint32_t GrowthIncrement = 8;                       // 增长增量
	uint32_t ShrinkThreshold = 4;                       // 收缩阈值
	bool bAutoShrink = true;                            // 自动收缩
	bool bPrewarm = false;                              // 预热对象
	bool bResetOnReturn = true;                         // 归还时重置对象
	CTimespan MaxIdleTime = CTimespan::FromMinutes(10); // 最大空闲时间
};

/**
 * @brief 池化对象包装器
 */
template <typename TObjectType>
struct SPooledObject
{
	TObjectType* Object = nullptr; // 对象指针
	bool bInUse = false;           // 是否在使用
	CDateTime CreationTime;        // 创建时间
	CDateTime LastAccessTime;      // 最后访问时间
	uint32_t UsageCount = 0;       // 使用次数

	SPooledObject()
	    : CreationTime(CDateTime::Now()),
	      LastAccessTime(CDateTime::Now())
	{}

	SPooledObject(TObjectType* InObject)
	    : Object(InObject),
	      CreationTime(CDateTime::Now()),
	      LastAccessTime(CDateTime::Now())
	{}

	void MarkUsed()
	{
		bInUse = true;
		LastAccessTime = CDateTime::Now();
		UsageCount++;
	}

	void MarkUnused()
	{
		bInUse = false;
		LastAccessTime = CDateTime::Now();
	}

	bool IsExpired(const CTimespan& MaxIdleTime) const
	{
		return !bInUse && (CDateTime::Now() - LastAccessTime) > MaxIdleTime;
	}
};

/**
 * @brief 通用对象池接口
 */
class IObjectPool
{
public:
	virtual ~IObjectPool() = default;

	/**
	 * @brief 获取池名称
	 */
	virtual const char* GetPoolName() const = 0;

	/**
	 * @brief 获取池统计信息
	 */
	virtual SObjectPoolStats GetStatistics() const = 0;

	/**
	 * @brief 清空池
	 */
	virtual void Clear() = 0;

	/**
	 * @brief 收缩池
	 */
	virtual void Shrink() = 0;

	/**
	 * @brief 预热池
	 */
	virtual void Prewarm() = 0;
};

/**
 * @brief 类型化对象池
 *
 * 为特定类型提供对象池功能：
 * - 自动对象创建和回收
 * - 多种池策略支持
 * - 性能统计和监控
 * - 线程安全操作
 */
template <typename TObjectType>
class TObjectPool : public IObjectPool
{
	static_assert(std::is_base_of_v<NObject, TObjectType>, "TObjectType must inherit from NObject");

public:
	// === 类型定义 ===
	using ObjectPtr = TSharedPtr<TObjectType>;
	using PooledObjectType = SPooledObject<TObjectType>;

public:
	// === 构造函数 ===

	/**
	 * @brief 构造函数
	 */
	TObjectPool(const char* InPoolName, const SObjectPoolConfig& InConfig = SObjectPoolConfig{})
	    : PoolName(InPoolName),
	      Config(InConfig),
	      bIsInitialized(false)
	{
		Initialize();
	}

	/**
	 * @brief 析构函数
	 */
	~TObjectPool()
	{
		Shutdown();
	}

public:
	// === 对象获取和归还 ===

	/**
	 * @brief 从池中获取对象
	 */
	ObjectPtr Acquire()
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);

		Stats.LastAccessTime = CDateTime::Now();

		// 查找可用对象
		TObjectType* Object = FindAvailableObject();

		if (!Object)
		{
			// 没有可用对象，创建新对象
			Object = CreateNewObject();
			Stats.CacheMisses++;

			if (!Object)
			{
				NLOG_MEMORY(Error, "Failed to create object in pool '{}'", PoolName);
				return nullptr;
			}
		}
		else
		{
			Stats.CacheHits++;
		}

		// 标记对象为使用中
		auto PooledObjIndex = FindPooledObjectIndex(Object);
		if (PooledObjIndex != INDEX_NONE)
		{
			PooledObjects[PooledObjIndex].MarkUsed();
		}

		Stats.ActiveObjects++;
		Stats.AvailableObjects--;
		Stats.TotalAllocations++;

		// 重置对象状态
		if (Config.bResetOnReturn)
		{
			ResetObject(Object);
		}

		NLOG_MEMORY(
		    Trace, "Object acquired from pool '{}', active: {}/{}", PoolName, Stats.ActiveObjects, Stats.PoolSize);

		// 创建智能指针，并设置自定义删除器
		return TSharedPtr<TObjectType>(Object, [this](TObjectType* Obj) { this->Release(Obj); });
	}

	/**
	 * @brief 归还对象到池
	 */
	void Release(TObjectType* Object)
	{
		if (!Object)
		{
			return;
		}

		std::lock_guard<std::mutex> Lock(PoolMutex);

		auto PooledObjIndex = FindPooledObjectIndex(Object);
		if (PooledObjIndex == INDEX_NONE)
		{
			NLOG_MEMORY(Warning, "Attempting to release object not from pool '{}'", PoolName);
			return;
		}

		// 标记对象为未使用
		PooledObjects[PooledObjIndex].MarkUnused();

		Stats.ActiveObjects--;
		Stats.AvailableObjects++;
		Stats.TotalDeallocations++;

		// 重置对象状态
		if (Config.bResetOnReturn)
		{
			ResetObject(Object);
		}

		NLOG_MEMORY(
		    Trace, "Object released to pool '{}', active: {}/{}", PoolName, Stats.ActiveObjects, Stats.PoolSize);

		// 检查是否需要收缩池
		if (Config.bAutoShrink)
		{
			CheckShrink();
		}
	}

public:
	// === IObjectPool接口实现 ===

	const char* GetPoolName() const override
	{
		return PoolName;
	}

	SObjectPoolStats GetStatistics() const override
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		return Stats;
	}

	void Clear() override
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);

		// 删除所有对象
		for (auto& PooledObj : PooledObjects)
		{
			if (PooledObj.Object)
			{
				delete PooledObj.Object;
			}
		}

		PooledObjects.Empty();
		Stats.PoolSize = 0;
		Stats.ActiveObjects = 0;
		Stats.AvailableObjects = 0;

		NLOG_MEMORY(Info, "Pool '{}' cleared", PoolName);
	}

	void Shrink() override
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		PerformShrink();
	}

	void Prewarm() override
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		PerformPrewarm();
	}

public:
	// === 配置和状态 ===

	/**
	 * @brief 获取池配置
	 */
	const SObjectPoolConfig& GetConfig() const
	{
		return Config;
	}

	/**
	 * @brief 更新池配置
	 */
	void UpdateConfig(const SObjectPoolConfig& NewConfig)
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		Config = NewConfig;
		NLOG_MEMORY(Info, "Pool '{}' configuration updated", PoolName);
	}

	/**
	 * @brief 获取当前池大小
	 */
	uint32_t GetPoolSize() const
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		return Stats.PoolSize;
	}

	/**
	 * @brief 获取活跃对象数
	 */
	uint32_t GetActiveObjectCount() const
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		return Stats.ActiveObjects;
	}

	/**
	 * @brief 获取可用对象数
	 */
	uint32_t GetAvailableObjectCount() const
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);
		return Stats.AvailableObjects;
	}

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成池状态报告
	 */
	CString GenerateReport() const
	{
		std::lock_guard<std::mutex> Lock(PoolMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Object Pool Report: %s ===\n"
		         "Strategy: %s\n"
		         "Pool Size: %u (Max: %u)\n"
		         "Active Objects: %u\n"
		         "Available Objects: %u\n"
		         "Total Allocations: %u\n"
		         "Total Deallocations: %u\n"
		         "Cache Hit Ratio: %.2f%%\n"
		         "Pool Expansions: %u\n"
		         "Pool Shrinks: %u\n"
		         "Creation Time: %s\n"
		         "Last Access: %s",
		         PoolName,
		         GetStrategyName().GetData(),
		         Stats.PoolSize,
		         Config.MaxSize,
		         Stats.ActiveObjects,
		         Stats.AvailableObjects,
		         Stats.TotalAllocations,
		         Stats.TotalDeallocations,
		         Stats.GetHitRatio() * 100.0f,
		         Stats.PoolExpansions,
		         Stats.PoolShrinks,
		         Stats.CreationTime.ToString().GetData(),
		         Stats.LastAccessTime.ToString().GetData());

		return CString(Buffer);
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 初始化对象池
	 */
	void Initialize()
	{
		if (bIsInitialized)
		{
			return;
		}

		PooledObjects.Reserve(Config.InitialSize);
		Stats.CreationTime = CDateTime::Now();
		Stats.LastAccessTime = CDateTime::Now();

		// 创建初始对象
		for (uint32_t i = 0; i < Config.InitialSize; ++i)
		{
			TObjectType* Object = CreateObjectInstance();
			if (Object)
			{
				PooledObjects.Add(PooledObjectType(Object));
				Stats.PoolSize++;
				Stats.AvailableObjects++;
			}
		}

		// 预热对象
		if (Config.bPrewarm)
		{
			PerformPrewarm();
		}

		bIsInitialized = true;

		NLOG_MEMORY(Info, "Object pool '{}' initialized with {} objects", PoolName, Stats.PoolSize);
	}

	/**
	 * @brief 关闭对象池
	 */
	void Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}

		Clear();
		bIsInitialized = false;

		NLOG_MEMORY(Info, "Object pool '{}' shutdown", PoolName);
	}

	/**
	 * @brief 查找可用对象
	 */
	TObjectType* FindAvailableObject()
	{
		for (auto& PooledObj : PooledObjects)
		{
			if (!PooledObj.bInUse && PooledObj.Object)
			{
				return PooledObj.Object;
			}
		}
		return nullptr;
	}

	/**
	 * @brief 创建新对象
	 */
	TObjectType* CreateNewObject()
	{
		// 检查是否可以扩展池
		if (Stats.PoolSize >= Config.MaxSize)
		{
			NLOG_MEMORY(Warning, "Pool '{}' reached maximum size ({})", PoolName, Config.MaxSize);
			return nullptr;
		}

		TObjectType* Object = CreateObjectInstance();
		if (Object)
		{
			PooledObjects.Add(PooledObjectType(Object));
			Stats.PoolSize++;
			Stats.AvailableObjects++;
			Stats.PoolExpansions++;

			NLOG_MEMORY(Debug, "Pool '{}' expanded to {} objects", PoolName, Stats.PoolSize);
		}

		return Object;
	}

	/**
	 * @brief 创建对象实例
	 */
	TObjectType* CreateObjectInstance()
	{
		try
		{
			// 使用工厂函数创建对象
			auto ObjectPtr = NewNObject<TObjectType>();
			return ObjectPtr.Get(); // 注意：这里需要特殊处理智能指针
		}
		catch (...)
		{
			NLOG_MEMORY(Error, "Failed to create object instance for pool '{}'", PoolName);
			return nullptr;
		}
	}

	/**
	 * @brief 重置对象状态
	 */
	void ResetObject(TObjectType* Object)
	{
		// 默认实现，派生类可以重写
		// 这里可以调用对象的Reset方法或重置特定属性
		(void)Object; // 避免未使用参数警告
	}

	/**
	 * @brief 查找池化对象索引
	 */
	int32_t FindPooledObjectIndex(TObjectType* Object) const
	{
		for (int32_t i = 0; i < PooledObjects.Size(); ++i)
		{
			if (PooledObjects[i].Object == Object)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * @brief 检查是否需要收缩池
	 */
	void CheckShrink()
	{
		if (Stats.AvailableObjects > Config.ShrinkThreshold * 2)
		{
			PerformShrink();
		}
	}

	/**
	 * @brief 执行池收缩
	 */
	void PerformShrink()
	{
		uint32_t TargetSize = Stats.ActiveObjects + Config.ShrinkThreshold;
		uint32_t ObjectsToRemove = Stats.PoolSize > TargetSize ? Stats.PoolSize - TargetSize : 0;

		if (ObjectsToRemove == 0)
		{
			return;
		}

		uint32_t RemovedCount = 0;

		// 移除过期的未使用对象
		for (int32_t i = PooledObjects.Size() - 1; i >= 0 && RemovedCount < ObjectsToRemove; --i)
		{
			auto& PooledObj = PooledObjects[i];
			if (!PooledObj.bInUse && PooledObj.IsExpired(Config.MaxIdleTime))
			{
				delete PooledObj.Object;
				PooledObjects.RemoveAt(i);
				RemovedCount++;
				Stats.PoolSize--;
				Stats.AvailableObjects--;
			}
		}

		if (RemovedCount > 0)
		{
			Stats.PoolShrinks++;
			NLOG_MEMORY(Debug, "Pool '{}' shrunk by {} objects to {}", PoolName, RemovedCount, Stats.PoolSize);
		}
	}

	/**
	 * @brief 执行预热
	 */
	void PerformPrewarm()
	{
		// 预热策略：创建一些对象并立即释放，以便测试创建过程
		TArray<TObjectType*, CMemoryManager> TempObjects;

		uint32_t PrewarmCount = Config.InitialSize / 2;
		for (uint32_t i = 0; i < PrewarmCount; ++i)
		{
			TObjectType* Object = FindAvailableObject();
			if (Object)
			{
				TempObjects.Add(Object);
				auto PooledObjIndex = FindPooledObjectIndex(Object);
				if (PooledObjIndex != INDEX_NONE)
				{
					PooledObjects[PooledObjIndex].MarkUsed();
				}
			}
		}

		// 释放预热对象
		for (TObjectType* Object : TempObjects)
		{
			auto PooledObjIndex = FindPooledObjectIndex(Object);
			if (PooledObjIndex != INDEX_NONE)
			{
				PooledObjects[PooledObjIndex].MarkUnused();
			}
		}

		NLOG_MEMORY(Debug, "Pool '{}' prewarmed with {} objects", PoolName, PrewarmCount);
	}

	/**
	 * @brief 获取策略名称
	 */
	CString GetStrategyName() const
	{
		switch (Config.Strategy)
		{
		case EPoolStrategy::FixedSize:
			return "FixedSize";
		case EPoolStrategy::Dynamic:
			return "Dynamic";
		case EPoolStrategy::LRU:
			return "LRU";
		case EPoolStrategy::Circular:
			return "Circular";
		default:
			return "Unknown";
		}
	}

private:
	// === 成员变量 ===

	const char* PoolName;     // 池名称
	SObjectPoolConfig Config; // 池配置
	SObjectPoolStats Stats;   // 统计信息

	TArray<PooledObjectType, CMemoryManager> PooledObjects; // 池化对象数组

	mutable std::mutex PoolMutex;     // 池互斥锁
	std::atomic<bool> bIsInitialized; // 是否已初始化
};

/**
 * @brief 对象池管理器
 *
 * 管理所有对象池，提供统一的访问接口
 */
class CObjectPoolManager
{
public:
	// === 单例模式 ===

	static CObjectPoolManager& GetInstance()
	{
		static CObjectPoolManager Instance;
		return Instance;
	}

private:
	CObjectPoolManager() = default;

public:
	// 禁用拷贝
	CObjectPoolManager(const CObjectPoolManager&) = delete;
	CObjectPoolManager& operator=(const CObjectPoolManager&) = delete;

public:
	// === 池管理 ===

	/**
	 * @brief 注册对象池
	 */
	void RegisterPool(const char* PoolName, TSharedPtr<IObjectPool> Pool)
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		CString Name(PoolName);
		if (Pools.Contains(Name))
		{
			NLOG_MEMORY(Warning, "Pool '{}' already registered", PoolName);
			return;
		}

		Pools.Insert(Name, Pool);
		NLOG_MEMORY(Info, "Pool '{}' registered", PoolName);
	}

	/**
	 * @brief 注销对象池
	 */
	void UnregisterPool(const char* PoolName)
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		CString Name(PoolName);
		if (Pools.Remove(Name))
		{
			NLOG_MEMORY(Info, "Pool '{}' unregistered", PoolName);
		}
	}

	/**
	 * @brief 获取对象池
	 */
	TSharedPtr<IObjectPool> GetPool(const char* PoolName)
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		CString Name(PoolName);
		auto* PoolPtr = Pools.Find(Name);
		return PoolPtr ? *PoolPtr : nullptr;
	}

	/**
	 * @brief 清空所有池
	 */
	void ClearAllPools()
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		for (auto& Pair : Pools)
		{
			if (Pair.second)
			{
				Pair.second->Clear();
			}
		}

		NLOG_MEMORY(Info, "All pools cleared");
	}

	/**
	 * @brief 收缩所有池
	 */
	void ShrinkAllPools()
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		for (auto& Pair : Pools)
		{
			if (Pair.second)
			{
				Pair.second->Shrink();
			}
		}

		NLOG_MEMORY(Info, "All pools shrunk");
	}

	/**
	 * @brief 生成所有池的报告
	 */
	CString GeneratePoolsReport() const
	{
		std::lock_guard<std::mutex> Lock(PoolsMutex);

		CString Report("=== Object Pool Manager Report ===\n");
		char Buffer[256];
		snprintf(Buffer, sizeof(Buffer), "Total Pools: %lu\n\n", static_cast<unsigned long>(Pools.Size()));
		Report += CString(Buffer);

		for (const auto& Pair : Pools)
		{
			if (Pair.second)
			{
				SObjectPoolStats Stats = Pair.second->GetStatistics();
				char PoolBuffer[512];
				snprintf(PoolBuffer, sizeof(PoolBuffer), "Pool: %s\n"
				                   "  Size: %u, Active: %u, Available: %u\n"
				                   "  Hit Ratio: %.2f%%, Allocations: %u\n\n",
				                   Pair.first.GetData(),
				                   Stats.PoolSize,
				                   Stats.ActiveObjects,
				                   Stats.AvailableObjects,
				                   Stats.GetHitRatio() * 100.0f,
				                   Stats.TotalAllocations);
				Report += CString(PoolBuffer);
			}
		}

		return Report;
	}

private:
	THashMap<CString, TSharedPtr<IObjectPool>, std::hash<CString>, std::equal_to<CString>, CMemoryManager> Pools;
	mutable std::mutex PoolsMutex;
};

// === 便捷宏和函数 ===

/**
 * @brief 获取对象池管理器
 */
inline CObjectPoolManager& GetPoolManager()
{
	return CObjectPoolManager::GetInstance();
}

/**
 * @brief 创建类型化对象池
 */
template <typename TObjectType>
TSharedPtr<TObjectPool<TObjectType>> CreateObjectPool(const char* PoolName,
                                                      const SObjectPoolConfig& Config = SObjectPoolConfig{})
{
	auto Pool = MakeShared<TObjectPool<TObjectType>>(PoolName, Config);
	GetPoolManager().RegisterPool(PoolName, Pool);
	return Pool;
}

} // namespace NLib