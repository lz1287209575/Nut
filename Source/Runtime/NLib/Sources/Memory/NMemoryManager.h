#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

// 引入tcmalloc头文件
#include <gperftools/malloc_extension.h>
#include <gperftools/tcmalloc.h>

namespace NLib
{

/**
 * @brief 内存管理器 - 基于tcmalloc的高性能内存分配器
 *
 * 提供统一的内存分配接口，集成tcmalloc优化和内存统计功能
 * 为GC系统提供底层内存管理支持
 */
class CMemoryManager
{
public:
	/**
	 * @brief 内存统计信息
	 */
	struct MemoryStats
	{
		size_t TotalAllocated;   // 总分配内存（字节）
		size_t TotalFreed;       // 总释放内存（字节）
		size_t CurrentUsage;     // 当前使用内存（字节）
		size_t PeakUsage;        // 峰值使用内存（字节）
		size_t AllocationCount;  // 分配次数
		size_t FreeCount;        // 释放次数
		size_t TcmallocHeapSize; // tcmalloc堆大小

		MemoryStats()
		    : TotalAllocated(0),
		      TotalFreed(0),
		      CurrentUsage(0),
		      PeakUsage(0),
		      AllocationCount(0),
		      FreeCount(0),
		      TcmallocHeapSize(0)
		{}
	};

public:
	/**
	 * @brief 获取内存管理器单例
	 */
	static CMemoryManager& GetInstance();

	/**
	 * @brief 初始化内存管理器
	 * @param bEnableProfiling 是否启用性能分析
	 */
	void Initialize(bool bEnableProfiling = false);

	/**
	 * @brief 关闭内存管理器
	 */
	void Shutdown();

	/**
	 * @brief 获取初始化状态
	 */
	inline bool IsInitialize() const
	{
		return bInitialized;
	}

public:
	// === 内存分配接口 ===

	/**
	 * @brief 分配内存
	 * @param Size 分配大小（字节）
	 * @param Alignment 对齐要求（默认为0，使用默认对齐）
	 * @return 分配的内存指针，失败返回nullptr
	 */
	void* Allocate(size_t Size, size_t Alignment = 0);

	/**
	 * @brief 释放内存
	 * @param Ptr 要释放的内存指针
	 */
	void Deallocate(void* Ptr);

	/**
	 * @brief 重新分配内存
	 * @param Ptr 原内存指针
	 * @param NewSize 新大小
	 * @return 新内存指针
	 */
	void* Reallocate(void* Ptr, size_t NewSize);

public:
	// === 统计和监控 ===

	/**
	 * @brief 获取当前内存统计信息
	 */
	MemoryStats GetStats() const;

	/**
	 * @brief 强制释放系统未使用的内存
	 */
	void ReleaseMemoryToSystem();

	/**
	 * @brief 设置内存限制（字节）
	 * @param Limit 内存限制，0表示无限制
	 */
	void SetMemoryLimit(size_t Limit);

	/**
	 * @brief 检查是否接近内存限制
	 * @param Threshold 阈值百分比（0.0-1.0）
	 * @return true if approaching limit
	 */
	bool IsApproachingMemoryLimit(float Threshold = 0.9f) const;

public:
	// === tcmalloc特定功能 ===

	/**
	 * @brief 获取tcmalloc属性
	 * @param Property 属性名
	 * @return 属性值，失败返回0
	 */
	size_t GetTcmallocProperty(const char* Property) const;

	/**
	 * @brief 设置tcmalloc属性
	 * @param Property 属性名
	 * @param Value 属性值
	 * @return 成功返回true
	 */
	bool SetTcmallocProperty(const char* Property, size_t Value);

	/**
	 * @brief 获取详细的tcmalloc统计信息
	 * @return 统计信息字符串
	 */
	std::string GetTcmallocStats() const;

private:
	CMemoryManager() = default;
	~CMemoryManager() = default;

	// 禁用拷贝和赋值
	CMemoryManager(const CMemoryManager&) = delete;
	CMemoryManager& operator=(const CMemoryManager&) = delete;

private:
	bool bInitialized = false;
	bool bProfilingEnabled = false;
	size_t MemoryLimit = 0; // 0表示无限制

	// 统计信息（原子操作确保线程安全）
	mutable std::atomic<size_t> TotalAllocated{0};
	mutable std::atomic<size_t> TotalFreed{0};
	mutable std::atomic<size_t> AllocationCount{0};
	mutable std::atomic<size_t> FreeCount{0};
	mutable std::atomic<size_t> PeakUsage{0};

	mutable std::mutex StatsMutex;

	// 内部辅助方法
	void UpdateStats(size_t AllocatedSize, bool bIsAllocation);
	void UpdatePeakUsage(size_t CurrentUsage) const;
};

/**
 * @brief 基于tcmalloc的RAII内存分配器
 * 可用于标准容器
 */
template <typename TType>
class TcmallocAllocator
{
public:
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template <typename U>
	struct rebind
	{
		using other = TcmallocAllocator<U>;
	};

	TcmallocAllocator() = default;

	template <typename U>
	TcmallocAllocator(const TcmallocAllocator<U>&) noexcept
	{}

	pointer allocate(size_type Count)
	{
		void* Ptr = CMemoryManager::GetInstance().Allocate(Count * sizeof(T));
		if (!Ptr && Count > 0)
		{
			throw std::bad_alloc();
		}
		return static_cast<pointer>(Ptr);
	}

	void deallocate(pointer Ptr, size_type)
	{
		CMemoryManager::GetInstance().Deallocate(Ptr);
	}

	template <typename U>
	bool operator==(const TcmallocAllocator<U>&) const noexcept
	{
		return true;
	}

	template <typename U>
	bool operator!=(const TcmallocAllocator<U>&) const noexcept
	{
		return false;
	}
};

// === 便利的类型别名 ===
template <typename TType>
using TcVector = std::vector<T, TcmallocAllocator<T>>;

using TcString = std::basic_string<char, std::char_traits<char>, TcmallocAllocator<char>>;

} // namespace NLib
