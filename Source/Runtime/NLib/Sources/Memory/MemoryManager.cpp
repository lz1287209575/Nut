#include "MemoryManager.h"

#include "Logging/LogCategory.h"
#include "Logging/Logger.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace NLib
{
// === CMemoryManager 实现 ===

CMemoryManager& CMemoryManager::GetInstance()
{
	static CMemoryManager Instance;
	return Instance;
}

CMemoryManager::CMemoryManager()
    : bStatsEnabled(true),
      bInitialized(false)
{
	InitializeTcmalloc();
	bInitialized = true;

	NLOG_MEMORY(Info, "MemoryManager initialized with tcmalloc");
}

CMemoryManager::~CMemoryManager()
{
	if (bStatsEnabled.load())
	{
		PrintMemoryReport(true);
	}

	// 释放所有未使用的内存给系统
	ReleaseMemoryToSystem();

	NLOG_MEMORY(Info, "MemoryManager shutdown completed");
}

void CMemoryManager::InitializeTcmalloc()
{
	// 配置tcmalloc参数
	// 设置内存释放率
	MallocExtension::instance()->SetMemoryReleaseRate(1.0);

	// 初始化统计数据
	std::lock_guard<std::mutex> Lock(StatsMutex);
	Stats = SMemoryStats{};
}

// === 内存分配接口 ===

void* CMemoryManager::Allocate(size_t Size, size_t Alignment)
{
	if (Size == 0)
	{
		return nullptr;
	}

	void* Ptr = nullptr;

	if (Alignment > 0 && Alignment > sizeof(void*))
	{
		// 使用对齐分配
		Ptr = AllocateAligned(Size, Alignment);
	}
	else
	{
		// 使用标准tcmalloc分配
		Ptr = tc_malloc(Size);
	}

	if (Ptr && bStatsEnabled.load())
	{
		UpdateStats(Size, true);
	}

	return Ptr;
}

void CMemoryManager::Deallocate(void* Ptr)
{
	if (!Ptr)
	{
		return;
	}

	if (bStatsEnabled.load())
	{
		size_t Size = GetBlockSize(Ptr);
		UpdateStats(Size, false);
	}

	tc_free(Ptr);
}

void* CMemoryManager::Reallocate(void* Ptr, size_t NewSize)
{
	if (NewSize == 0)
	{
		Deallocate(Ptr);
		return nullptr;
	}

	if (!Ptr)
	{
		return Allocate(NewSize);
	}

	size_t OldSize = 0;
	if (bStatsEnabled.load())
	{
		OldSize = GetBlockSize(Ptr);
	}

	void* NewPtr = tc_realloc(Ptr, NewSize);

	if (NewPtr && bStatsEnabled.load())
	{
		UpdateStats(OldSize, false); // 减少旧的大小
		UpdateStats(NewSize, true);  // 增加新的大小
	}

	return NewPtr;
}

void* CMemoryManager::AllocateZeroed(size_t Count, size_t Size)
{
	void* Ptr = tc_calloc(Count, Size);

	if (Ptr && bStatsEnabled.load())
	{
		UpdateStats(Count * Size, true);
	}

	return Ptr;
}

// === 对齐内存分配 ===

void* CMemoryManager::AllocateAligned(size_t Size, size_t Alignment)
{
	// 验证对齐参数
	if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0)
	{
		NLOG_MEMORY(Error, "Alignment must be a power of 2");
		return nullptr;
	}

	void* Ptr = nullptr;
	int Result = tc_posix_memalign(&Ptr, Alignment, Size);

	if (Result != 0)
	{
		NLOG_MEMORY(Error, "Aligned allocation failed with code: {}", Result);
		return nullptr;
	}

	if (Ptr && bStatsEnabled.load())
	{
		UpdateStats(Size, true);
	}

	return Ptr;
}

void CMemoryManager::DeallocateAligned(void* Ptr)
{
	// tc_free可以处理aligned内存
	Deallocate(Ptr);
}

// === NObject专用接口 ===

void* CMemoryManager::AllocateObject(size_t Size)
{
	// 为NObject使用对齐分配，确保合适的对齐
	constexpr size_t ObjectAlignment = std::max(sizeof(void*), alignof(std::max_align_t));
	return AllocateAligned(Size, ObjectAlignment);
}

void CMemoryManager::DeallocateObject(void* Ptr)
{
	DeallocateAligned(Ptr);
}

// === 内存统计和调试 ===

SMemoryStats CMemoryManager::GetMemoryStats() const
{
	std::lock_guard<std::mutex> Lock(StatsMutex);
	return Stats;
}

size_t CMemoryManager::GetCurrentHeapSize() const
{
	size_t HeapSize = 0;
	MallocExtension::instance()->GetNumericProperty("generic.current_allocated_bytes", &HeapSize);
	return HeapSize;
}

size_t CMemoryManager::GetTotalAllocatedBytes() const
{
	size_t TotalAllocated = 0;
	MallocExtension::instance()->GetNumericProperty("generic.total_physical_bytes", &TotalAllocated);
	return TotalAllocated;
}

void CMemoryManager::ReleaseMemoryToSystem()
{
	MallocExtension::instance()->ReleaseFreeMemory();
	NLOG_MEMORY(Debug, "Released unused memory to system");
}

void CMemoryManager::SetMemoryStatsEnabled(bool bEnable)
{
	bStatsEnabled = bEnable;
	NLOG_MEMORY(Info, "Memory stats {}", bEnable ? "enabled" : "disabled");
}

bool CMemoryManager::IsMemoryStatsEnabled() const
{
	return bStatsEnabled.load();
}

// === 调试和诊断 ===

bool CMemoryManager::VerifyHeap() const
{
	// tcmalloc没有直接的堆验证接口，返回true
	// 可以通过其他方式实现验证逻辑
	return true;
}

size_t CMemoryManager::GetBlockSize(void* Ptr) const
{
	if (!Ptr)
	{
		return 0;
	}

	return tc_malloc_size(Ptr);
}

void CMemoryManager::PrintMemoryReport(bool bDetailed) const
{
	std::lock_guard<std::mutex> Lock(StatsMutex);

	NLOG_MEMORY(Info, "=== NLib Memory Report ===");
	NLOG_MEMORY(Info, "Current Used:      {:>12} bytes", Stats.CurrentUsed);
	NLOG_MEMORY(Info, "Peak Used:         {:>12} bytes", Stats.PeakUsed);
	NLOG_MEMORY(Info, "Total Allocated:   {:>12} bytes", Stats.TotalAllocated);
	NLOG_MEMORY(Info, "Total Deallocated: {:>12} bytes", Stats.TotalDeallocated);
	NLOG_MEMORY(Info, "Allocation Count:  {:>12}", Stats.AllocationCount);
	NLOG_MEMORY(Info, "Deallocation Count:{:>12}", Stats.DeallocationCount);

	if (bDetailed)
	{
		NLOG_MEMORY(Info, "=== tcmalloc Details ===");
		NLOG_MEMORY(Info, "Heap Size:       {:>12} bytes", GetCurrentHeapSize());
		NLOG_MEMORY(Info, "Total Physical:  {:>12} bytes", GetTotalAllocatedBytes());

		// 获取更多tcmalloc统计信息
		size_t Value = 0;
		if (MallocExtension::instance()->GetNumericProperty("tcmalloc.pageheap_free_bytes", &Value))
		{
			NLOG_MEMORY(Info, "Free Pages:      {:>12} bytes", Value);
		}
		if (MallocExtension::instance()->GetNumericProperty("tcmalloc.central_cache_free_bytes", &Value))
		{
			NLOG_MEMORY(Info, "Central Cache:   {:>12} bytes", Value);
		}
	}

	NLOG_MEMORY(Info, "=========================");
}

// === 内部方法 ===

void CMemoryManager::UpdateStats(size_t Size, bool bAllocation)
{
	std::lock_guard<std::mutex> Lock(StatsMutex);

	if (bAllocation)
	{
		Stats.TotalAllocated += Size;
		Stats.CurrentUsed += Size;
		Stats.AllocationCount++;

		// 更新峰值使用量
		Stats.PeakUsed = std::max(Stats.PeakUsed, Stats.CurrentUsed);
	}
	else
	{
		Stats.TotalDeallocated += Size;
		if (Stats.CurrentUsed >= Size)
		{
			Stats.CurrentUsed -= Size;
		}
		else
		{
			// 防止下溢
			Stats.CurrentUsed = 0;
		}
		Stats.DeallocationCount++;
	}
}

} // namespace NLib