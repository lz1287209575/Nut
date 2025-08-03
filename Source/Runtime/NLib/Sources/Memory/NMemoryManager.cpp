#include "CMemoryManager.h"

#include "Logging/CLogger.h"

#include <algorithm>
#include <sstream>

namespace NLib
{

CMemoryManager& CMemoryManager::GetInstance()
{
	static CMemoryManager Instance;
	return Instance;
}

void CMemoryManager::Initialize(bool bEnableProfiling)
{
	if (bInitialized)
	{
		CLogger::Warn("CMemoryManager already initialized");
		return;
	}

	bProfilingEnabled = bEnableProfiling;

	// 配置tcmalloc参数
	if (!SetTcmallocProperty("tcmalloc.max_total_thread_cache_bytes", 32 * 1024 * 1024))
	{
		CLogger::Warn("Failed to set tcmalloc thread cache size");
	}

	// 如果启用了性能分析，可以设置采样率
	if (bProfilingEnabled)
	{
		if (!SetTcmallocProperty("tcmalloc.sampling_period_bytes", 1024 * 1024))
		{
			CLogger::Warn("Failed to set tcmalloc sampling period");
		}
	}

	bInitialized = true;

	CLogger::Info("CMemoryManager initialized with tcmalloc" +
	              std::string(bProfilingEnabled ? " (profiling enabled)" : ""));
}

void CMemoryManager::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	// 释放未使用的内存回系统
	ReleaseMemoryToSystem();

	// 输出最终统计信息
	auto Stats = GetStats();
	CLogger::Info("CMemoryManager shutdown - Final stats:");
	CLogger::Info("  Total allocated: " + std::to_string(Stats.TotalAllocated) + " bytes");
	CLogger::Info("  Total freed: " + std::to_string(Stats.TotalFreed) + " bytes");
	CLogger::Info("  Peak usage: " + std::to_string(Stats.PeakUsage) + " bytes");
	CLogger::Info("  Allocation count: " + std::to_string(Stats.AllocationCount));

	bInitialized = false;
}

void* CMemoryManager::Allocate(size_t Size, size_t Alignment)
{
	if (!bInitialized)
	{
		Initialize();
	}

	if (Size == 0)
	{
		return nullptr;
	}

	void* Ptr = nullptr;

	if (Alignment > 0)
	{
		// 使用对齐分配
		if (posix_memalign(&Ptr, Alignment, Size) != 0)
		{
			Ptr = nullptr;
		}
	}
	else
	{
		// 使用tcmalloc标准分配
		Ptr = tc_malloc(Size);
	}

	if (Ptr)
	{
		UpdateStats(Size, true);

		// 检查内存限制
		if (MemoryLimit > 0)
		{
			auto Stats = GetStats();
			if (Stats.CurrentUsage > MemoryLimit)
			{
				CLogger::Warn("Memory usage exceeded limit: " + std::to_string(Stats.CurrentUsage) + " > " +
				              std::to_string(MemoryLimit));
			}
		}
	}
	else
	{
		CLogger::Error("Memory allocation failed for size: " + std::to_string(Size));
	}

	return Ptr;
}

void CMemoryManager::Deallocate(void* Ptr)
{
	if (!Ptr)
	{
		return;
	}

	// 获取分配的大小（tcmalloc提供此功能）
	size_t Size = tc_malloc_size(Ptr);

	tc_free(Ptr);

	if (Size > 0)
	{
		UpdateStats(Size, false);
	}
}

void* CMemoryManager::Reallocate(void* Ptr, size_t NewSize)
{
	if (!bInitialized)
	{
		Initialize();
	}

	if (NewSize == 0)
	{
		Deallocate(Ptr);
		return nullptr;
	}

	if (!Ptr)
	{
		return Allocate(NewSize);
	}

	size_t OldSize = tc_malloc_size(Ptr);
	void* NewPtr = tc_realloc(Ptr, NewSize);

	if (NewPtr)
	{
		// 更新统计信息
		if (NewSize > OldSize)
		{
			UpdateStats(NewSize - OldSize, true);
		}
		else if (OldSize > NewSize)
		{
			UpdateStats(OldSize - NewSize, false);
		}
	}
	else
	{
		CLogger::Error("Memory reallocation failed for size: " + std::to_string(NewSize));
	}

	return NewPtr;
}

CMemoryManager::MemoryStats CMemoryManager::GetStats() const
{
	MemoryStats Stats;

	Stats.TotalAllocated = TotalAllocated.load();
	Stats.TotalFreed = TotalFreed.load();
	Stats.CurrentUsage = Stats.TotalAllocated - Stats.TotalFreed;
	Stats.PeakUsage = PeakUsage.load();
	Stats.AllocationCount = AllocationCount.load();
	Stats.FreeCount = FreeCount.load();

	// 获取tcmalloc堆大小
	Stats.TcmallocHeapSize = GetTcmallocProperty("generic.heap_size");

	return Stats;
}

void CMemoryManager::ReleaseMemoryToSystem()
{
	MallocExtension::instance()->ReleaseFreeMemory();
	CLogger::Debug("Released unused memory to system");
}

void CMemoryManager::SetMemoryLimit(size_t Limit)
{
	MemoryLimit = Limit;
	if (Limit > 0)
	{
		CLogger::Info("Memory limit set to: " + std::to_string(Limit) + " bytes");
	}
	else
	{
		CLogger::Info("Memory limit removed");
	}
}

bool CMemoryManager::IsApproachingMemoryLimit(float Threshold) const
{
	if (MemoryLimit == 0)
	{
		return false;
	}

	auto Stats = GetStats();
	return Stats.CurrentUsage >= (MemoryLimit * Threshold);
}

size_t CMemoryManager::GetTcmallocProperty(const char* Property) const
{
	size_t Value = 0;
	if (MallocExtension::instance()->GetNumericProperty(Property, &Value))
	{
		return Value;
	}
	return 0;
}

bool CMemoryManager::SetTcmallocProperty(const char* Property, size_t Value)
{
	return MallocExtension::instance()->SetNumericProperty(Property, Value);
}

std::string CMemoryManager::GetTcmallocStats() const
{
	char Stats[1024 * 1024] = {0};
	MallocExtension::instance()->GetStats(Stats, 1024 * 1024); // 1MB buffer
	return Stats;
}

void CMemoryManager::UpdateStats(size_t Size, bool bIsAllocation)
{
	if (bIsAllocation)
	{
		TotalAllocated.fetch_add(Size);
		AllocationCount.fetch_add(1);

		// 更新峰值使用量
		size_t CurrentUsage = TotalAllocated.load() - TotalFreed.load();
		UpdatePeakUsage(CurrentUsage);
	}
	else
	{
		TotalFreed.fetch_add(Size);
		FreeCount.fetch_add(1);
	}
}

void CMemoryManager::UpdatePeakUsage(size_t CurrentUsage) const
{
	size_t CurrentPeak = PeakUsage.load();
	while (CurrentUsage > CurrentPeak)
	{
		if (PeakUsage.compare_exchange_weak(CurrentPeak, CurrentUsage))
		{
			break;
		}
	}
}

} // namespace NLib
