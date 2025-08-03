#pragma once

#include "CAllocator.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace NLib
{

// 前向声明
class CObject;
template <typename TType>
class CArray;
template <typename TType>
class CSet;

/**
 * @brief 垃圾回收器 - 管理NObject对象的自动内存回收
 *
 * 实现混合垃圾回收策略：
 * 1. 引用计数：处理大部分对象的即时回收
 * 2. 标记-清扫：处理循环引用和复杂对象图
 *
 * 线程安全设计，支持后台异步回收
 * 遵循UE命名规范
 */
class CGarbageCollector
{
public:
	/**
	 * @brief GC运行模式
	 */
	enum class EGCMode
	{
		Manual,    // 手动触发GC
		Automatic, // 自动定时GC
		Adaptive   // 自适应GC（根据内存使用情况调整）
	};

	/**
	 * @brief GC统计信息
	 */
	struct GCStats
	{
		uint64_t TotalCollections;                                     // 总回收次数
		uint64_t ObjectsCollected;                                     // 已回收对象总数
		uint64_t ObjectsAlive;                                         // 当前存活对象数
		uint64_t LastCollectionTime;                                   // 上次回收耗时（毫秒）
		uint64_t TotalCollectionTime;                                  // 总回收耗时
		uint64_t BytesReclaimed;                                       // 已回收内存字节数
		std::chrono::steady_clock::time_point LastCollectionTimestamp; // 上次回收时间戳
	};

private:
	CGarbageCollector();

public:
	~CGarbageCollector();

	// 单例模式
	static CGarbageCollector& GetInstance();

	// 禁用拷贝和移动
	CGarbageCollector(const CGarbageCollector&) = delete;
	CGarbageCollector& operator=(const CGarbageCollector&) = delete;
	CGarbageCollector(CGarbageCollector&&) = delete;
	CGarbageCollector& operator=(CGarbageCollector&&) = delete;

	// === 初始化和配置 ===

	/**
	 * @brief 初始化垃圾回收器
	 * @param Mode GC运行模式
	 * @param CollectionIntervalMs 自动回收间隔（毫秒）
	 * @param bEnableBackgroundCollection 是否启用后台回收线程
	 */
	void Initialize(EGCMode Mode = EGCMode::Adaptive,
	                uint32_t CollectionIntervalMs = 5000,
	                bool bEnableBackgroundCollection = true);

	/**
	 * @brief 关闭垃圾回收器
	 */
	void Shutdown();

	/**
	 * @brief 获取初始化状态
	 */
	bool IsInitialize() const
	{
		return bInitialized;
	}

	// === 对象注册管理 ===

	/**
	 * @brief 注册对象到GC系统
	 * @param Object 要注册的对象
	 */
	void RegisterObject(CObject* Object);

	/**
	 * @brief 从GC系统注销对象
	 * @param Object 要注销的对象
	 */
	void UnregisterObject(CObject* Object);

	/**
	 * @brief 获取当前注册对象数量
	 */
	size_t GetRegisteredObjectCount() const;

	// === 垃圾回收执行 ===

	/**
	 * @brief 手动触发垃圾回收
	 * @param bForceFullCollection 是否强制执行完整回收
	 * @return 回收的对象数量
	 */
	uint32_t Collect(bool bForceFullCollection = false);

	/**
	 * @brief 异步触发垃圾回收
	 */
	void CollectAsync();

	/**
	 * @brief 设置根对象集合（不会被回收的对象）
	 * @param RootObjects 根对象列表（使用tcmalloc分配器）
	 */
	void SetRootObjects(const CArray<CObject*>& RootObjectList);

	/**
	 * @brief 添加根对象
	 * @param Object 根对象
	 */
	void AddRootObject(CObject* Object);

	/**
	 * @brief 移除根对象
	 * @param Object 根对象
	 */
	void RemoveRootObject(CObject* Object);

	// === 配置和调优 ===

	/**
	 * @brief 设置GC模式
	 * @param Mode 新的GC模式
	 */
	void SetGCMode(EGCMode Mode);

	/**
	 * @brief 设置自动回收间隔
	 * @param IntervalMs 间隔时间（毫秒）
	 */
	void SetCollectionInterval(uint32_t IntervalMs);

	/**
	 * @brief 设置内存阈值（当超过此阈值时触发GC）
	 * @param ThresholdBytes 阈值字节数
	 */
	void SetMemoryThreshold(size_t ThresholdBytes);

	/**
	 * @brief 启用/禁用增量回收
	 * @param bEnable 是否启用
	 */
	void SetIncrementalCollection(bool bEnable);

public:
	// === 统计和监控 ===

	/**
	 * @brief 获取GC统计信息
	 */
	GCStats GetStats() const;

	/**
	 * @brief 重置统计信息
	 */
	void ResetStats();

	/**
	 * @brief 检查是否正在执行垃圾回收
	 */
	bool IsCollecting() const
	{
		return bIsCollecting;
	}

	/**
	 * @brief 获取当前GC模式
	 */
	EGCMode GetGCMode() const
	{
		return CurrentMode;
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 标记阶段 - 标记所有可达对象
	 * @return 标记的对象数量
	 */
	uint32_t MarkPhase();

	/**
	 * @brief 清扫阶段 - 回收未标记的对象
	 * @return 回收的对象数量
	 */
	uint32_t SweepPhase();

	/**
	 * @brief 从根对象开始递归标记
	 * @param Object 当前对象
	 * @param MarkedCount 已标记对象计数
	 */
	void MarkFromRoot(CObject* Object, uint32_t& MarkedCount);

	/**
	 * @brief 后台回收线程函数
	 */
	void BackgroundCollectionThread();

	/**
	 * @brief 检查是否需要触发自动回收
	 */
	bool ShouldTriggerCollection() const;

	/**
	 * @brief 更新统计信息
	 * @param CollectedObjects 本次回收的对象数
	 * @param CollectionTimeMs 回收耗时
	 */
	void UpdateStats(uint32_t CollectedObjects, uint64_t CollectionTimeMs);

	// === 成员变量 ===

	// 初始化状态
	std::atomic<bool> bInitialized{false};
	std::atomic<bool> bShutdown{false};

	// GC配置
	EGCMode CurrentMode{EGCMode::Adaptive};
	uint32_t CollectionIntervalMs{5000};
	size_t MemoryThreshold{100 * 1024 * 1024}; // 100MB默认阈值
	bool bEnableIncrementalCollection{true};
	bool bEnableBackgroundThread{true};

	// 对象管理
	mutable std::mutex ObjectsMutex;
	std::unique_ptr<CSet<CObject*>> RegisteredObjects; // 使用tcmalloc分配器
	std::unique_ptr<CSet<CObject*>> RootObjects;       // 使用tcmalloc分配器

	// 回收状态
	std::atomic<bool> bIsCollecting{false};
	mutable std::mutex CollectionMutex;

	// 后台线程
	std::unique_ptr<std::thread> BackgroundThread;
	std::condition_variable CollectionCondition;
	std::mutex CollectionWaitMutex;

	// 统计信息
	mutable std::mutex StatsMutex;
	GCStats Stats{};

	// 性能优化
	std::unique_ptr<CArray<CObject*>> MarkStack;       // 标记栈，使用tcmalloc分配器
	std::unique_ptr<CArray<CObject*>> ObjectsToDelete; // 待删除对象列表，使用tcmalloc分配器
};

} // namespace NLib
