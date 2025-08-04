#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

// tcmalloc headers - include without extern "C" for C++ compatibility
#include "gperftools/malloc_extension.h"
#include "gperftools/tcmalloc.h"

namespace NLib
{
/**
 * @brief 内存分配统计信息
 */
struct SMemoryStats
{
	size_t TotalAllocated = 0;      // 总分配字节数
	size_t TotalDeallocated = 0;    // 总释放字节数
	size_t CurrentUsed = 0;         // 当前使用字节数
	size_t PeakUsed = 0;            // 峰值使用字节数
	uint64_t AllocationCount = 0;   // 分配次数
	uint64_t DeallocationCount = 0; // 释放次数
};

/**
 * @brief tcmalloc内存管理器包装类
 *
 * 为NLib提供统一的内存管理接口，基于tcmalloc实现
 * 提供内存分配、释放、统计和调试功能
 */
class CMemoryManager
{
public:
	/**
	 * @brief 获取内存管理器的全局实例
	 * @return 内存管理器实例引用
	 */
	static CMemoryManager& GetInstance();

	// 禁用拷贝和赋值
	CMemoryManager(const CMemoryManager&) = delete;
	CMemoryManager& operator=(const CMemoryManager&) = delete;

public:
	// === 内存分配接口 ===

	/**
	 * @brief 分配内存
	 * @param Size 要分配的字节数
	 * @param Alignment 内存对齐要求（默认为0，使用默认对齐）
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
	 * @param NewSize 新的大小
	 * @return 新的内存指针
	 */
	void* Reallocate(void* Ptr, size_t NewSize);

	/**
	 * @brief 分配并清零内存
	 * @param Count 元素数量
	 * @param Size 每个元素的大小
	 * @return 分配的内存指针
	 */
	void* AllocateZeroed(size_t Count, size_t Size);

public:
	// === 对齐内存分配 ===

	/**
	 * @brief 分配对齐内存
	 * @param Size 要分配的字节数
	 * @param Alignment 对齐字节数（必须是2的幂）
	 * @return 分配的内存指针
	 */
	void* AllocateAligned(size_t Size, size_t Alignment);

	/**
	 * @brief 释放对齐内存
	 * @param Ptr 要释放的内存指针
	 */
	void DeallocateAligned(void* Ptr);

public:
	// === NObject专用接口 ===

	/**
	 * @brief 为NObject分配内存
	 * @param Size 对象大小
	 * @return 分配的内存指针
	 */
	void* AllocateObject(size_t Size);

	/**
	 * @brief 释放NObject内存
	 * @param Ptr NObject内存指针
	 */
	void DeallocateObject(void* Ptr);

public:
	// === 内存统计和调试 ===

	/**
	 * @brief 获取内存使用统计
	 * @return 内存统计信息
	 */
	SMemoryStats GetMemoryStats() const;

	/**
	 * @brief 获取当前堆使用量
	 * @return 当前堆使用的字节数
	 */
	size_t GetCurrentHeapSize() const;

	/**
	 * @brief 获取tcmalloc分配的总内存
	 * @return 总分配字节数
	 */
	size_t GetTotalAllocatedBytes() const;

	/**
	 * @brief 释放未使用的内存给系统
	 */
	void ReleaseMemoryToSystem();

	/**
	 * @brief 启用/禁用内存统计
	 * @param bEnable 是否启用统计
	 */
	void SetMemoryStatsEnabled(bool bEnable);

	/**
	 * @brief 检查内存统计是否启用
	 * @return true if enabled, false otherwise
	 */
	bool IsMemoryStatsEnabled() const;

public:
	// === 调试和诊断 ===

	/**
	 * @brief 验证堆的完整性
	 * @return true if heap is valid, false otherwise
	 */
	bool VerifyHeap() const;

	/**
	 * @brief 获取内存块大小
	 * @param Ptr 内存指针
	 * @return 内存块的实际大小
	 */
	size_t GetBlockSize(void* Ptr) const;

	/**
	 * @brief 打印内存使用报告
	 * @param bDetailed 是否显示详细信息
	 */
	void PrintMemoryReport(bool bDetailed = false) const;

private:
	// === 构造函数 ===
	CMemoryManager();
	~CMemoryManager();

	// === 内部方法 ===
	void UpdateStats(size_t Size, bool bAllocation);
	void InitializeTcmalloc();

private:
	// === 成员变量 ===
	mutable std::mutex StatsMutex;   // 统计数据互斥锁
	mutable SMemoryStats Stats;      // 内存统计数据
	std::atomic<bool> bStatsEnabled; // 是否启用统计
	std::atomic<bool> bInitialized;  // 是否已初始化
};

/**
 * @brief RAII内存分配器模板类
 *
 * 为特定类型提供自动内存管理，基于CMemoryManager
 */
template <typename TType>
class TAllocator
{
public:
	using ValueType = TType;
	using Pointer = TType*;
	using ConstPointer = const TType*;
	using Reference = TType&;
	using ConstReference = const TType&;
	using SizeType = size_t;
	using DifferenceType = ptrdiff_t;

	template <typename TOther>
	struct Rebind
	{
		using Other = TAllocator<TOther>;
	};

public:
	TAllocator() = default;

	template <typename TOther>
	TAllocator(const TAllocator<TOther>&)
	{}

	/**
	 * @brief 分配内存
	 * @param Count 元素数量
	 * @return 分配的内存指针
	 */
	Pointer Allocate(SizeType Count)
	{
		auto& MemMgr = CMemoryManager::GetInstance();
		void* Ptr = MemMgr.Allocate(Count * sizeof(TType), alignof(TType));
		return static_cast<Pointer>(Ptr);
	}

	/**
	 * @brief 释放内存
	 * @param Ptr 内存指针
	 * @param Count 元素数量（未使用）
	 */
	void Deallocate(Pointer Ptr, SizeType Count = 0)
	{
		(void)Count; // 忽略Count参数
		auto& MemMgr = CMemoryManager::GetInstance();
		MemMgr.Deallocate(Ptr);
	}

	/**
	 * @brief 构造对象
	 * @param Ptr 内存位置
	 * @param Args 构造函数参数
	 */
	template <typename... TArgs>
	void Construct(Pointer Ptr, TArgs&&... Args)
	{
		new (Ptr) TType(std::forward<TArgs>(Args)...);
	}

	/**
	 * @brief 销毁对象
	 * @param Ptr 对象指针
	 */
	void Destroy(Pointer Ptr)
	{
		Ptr->~TType();
	}

	/**
	 * @brief 获取最大可分配元素数量
	 * @return 最大元素数量
	 */
	SizeType MaxSize() const
	{
		return SIZE_MAX / sizeof(TType);
	}

	// 比较操作符
	template <typename TOther>
	bool operator==(const TAllocator<TOther>&) const
	{
		return true;
	}

	template <typename TOther>
	bool operator!=(const TAllocator<TOther>&) const
	{
		return false;
	}
};

} // namespace NLib