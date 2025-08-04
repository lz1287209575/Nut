#pragma once

/**
 * @file Memory.h
 * @brief NLib内存管理库主头文件
 *
 * 提供完整的内存管理功能：
 * - 内存分配器和管理器
 * - 垃圾回收系统
 * - 对象池管理
 * - 内存统计和诊断
 */

// 基础内存管理
#include "MemoryManager.h"

// 垃圾回收系统
#include "GarbageCollector.h"

// 对象池系统
#include "Math/MathTypes.h"
#include "ObjectPool.h"
#include "Time/Timer.h"

#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace NLib
{
/**
 * @brief 内存管理工具类
 */
class CMemoryUtils
{
public:
	// === 内存信息获取 ===

	/**
	 * @brief 获取系统内存信息
	 */
	struct SSystemMemoryInfo
	{
		uint64_t TotalPhysicalMemory = 0;     // 总物理内存
		uint64_t AvailablePhysicalMemory = 0; // 可用物理内存
		uint64_t TotalVirtualMemory = 0;      // 总虚拟内存
		uint64_t AvailableVirtualMemory = 0;  // 可用虚拟内存
		uint64_t ProcessMemoryUsage = 0;      // 进程内存使用量
		float MemoryUsageRatio = 0.0f;        // 内存使用率
	};

	/**
	 * @brief 获取系统内存信息
	 */
	static SSystemMemoryInfo GetSystemMemoryInfo()
	{
		SSystemMemoryInfo Info;

		// 简化实现，实际需要使用平台特定的API
		// Windows: GlobalMemoryStatusEx
		// Linux: /proc/meminfo
		// macOS: host_statistics64

#ifdef _WIN32
		// Windows实现
		MEMORYSTATUSEX MemStatus;
		MemStatus.dwLength = sizeof(MemStatus);
		if (GlobalMemoryStatusEx(&MemStatus))
		{
			Info.TotalPhysicalMemory = MemStatus.ullTotalPhys;
			Info.AvailablePhysicalMemory = MemStatus.ullAvailPhys;
			Info.TotalVirtualMemory = MemStatus.ullTotalVirtual;
			Info.AvailableVirtualMemory = MemStatus.ullAvailVirtual;
			Info.MemoryUsageRatio = static_cast<float>(MemStatus.dwMemoryLoad) / 100.0f;
		}
#elif defined(__APPLE__) || defined(__linux__)
		// Unix/Linux实现（简化）
		Info.TotalPhysicalMemory = 8ULL * 1024 * 1024 * 1024;     // 假设8GB
		Info.AvailablePhysicalMemory = 4ULL * 1024 * 1024 * 1024; // 假设4GB可用
		Info.MemoryUsageRatio = 0.5f;
#endif

		return Info;
	}

	/**
	 * @brief 获取进程内存使用信息
	 */
	struct SProcessMemoryInfo
	{
		uint64_t VirtualMemorySize = 0; // 虚拟内存大小
		uint64_t ResidentSetSize = 0;   // 常驻内存大小
		uint64_t HeapSize = 0;          // 堆大小
		uint64_t StackSize = 0;         // 栈大小
		uint32_t PageFaults = 0;        // 页错误次数
	};

	static SProcessMemoryInfo GetProcessMemoryInfo()
	{
		SProcessMemoryInfo Info;

		// 简化实现，实际需要使用平台特定的API
		auto& MemMgr = CMemoryManager::GetInstance();
		Info.HeapSize = MemMgr.GetTotalAllocatedMemory();

		return Info;
	}

public:
	// === 内存操作工具 ===

	/**
	 * @brief 安全的内存拷贝
	 */
	static bool SafeMemCopy(void* Dest, size_t DestSize, const void* Src, size_t SrcSize)
	{
		if (!Dest || !Src || DestSize == 0 || SrcSize == 0)
		{
			return false;
		}

		size_t CopySize = SrcSize < DestSize ? SrcSize : DestSize;
		memcpy(Dest, Src, CopySize);

		return true;
	}

	/**
	 * @brief 安全的内存设置
	 */
	static bool SafeMemSet(void* Dest, size_t DestSize, int Value)
	{
		if (!Dest || DestSize == 0)
		{
			return false;
		}

		memset(Dest, Value, DestSize);
		return true;
	}

	/**
	 * @brief 安全的内存比较
	 */
	static int SafeMemCompare(const void* Ptr1, const void* Ptr2, size_t Size)
	{
		if (!Ptr1 || !Ptr2 || Size == 0)
		{
			return 0;
		}

		return memcmp(Ptr1, Ptr2, Size);
	}

	/**
	 * @brief 内存对齐
	 */
	static size_t AlignSize(size_t Size, size_t Alignment)
	{
		return (Size + Alignment - 1) & ~(Alignment - 1);
	}

	/**
	 * @brief 检查指针是否对齐
	 */
	static bool IsAligned(const void* Ptr, size_t Alignment)
	{
		return (reinterpret_cast<uintptr_t>(Ptr) % Alignment) == 0;
	}

public:
	// === 内存诊断 ===

	/**
	 * @brief 内存泄漏检测信息
	 */
	struct SMemoryLeakInfo
	{
		uint32_t LeakedBlocks = 0;                    // 泄漏块数
		uint64_t LeakedBytes = 0;                     // 泄漏字节数
		TArray<void*, CMemoryManager> LeakedPointers; // 泄漏指针列表
	};

	/**
	 * @brief 检测内存泄漏
	 */
	static SMemoryLeakInfo DetectMemoryLeaks()
	{
		SMemoryLeakInfo Info;

		// 通过内存管理器检测泄漏
		auto& MemMgr = CMemoryManager::GetInstance();
		// 这里需要内存管理器提供泄漏检测功能

		return Info;
	}

	/**
	 * @brief 生成内存使用报告
	 */
	static CString GenerateMemoryReport()
	{
		auto SystemInfo = GetSystemMemoryInfo();
		auto ProcessInfo = GetProcessMemoryInfo();
		auto& MemMgr = CMemoryManager::GetInstance();
		auto& GC = CGarbageCollector::GetInstance();

		char Buffer[2048];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Memory Management Report ===\n\n"
		         "System Memory:\n"
		         "  Total Physical: %.2f MB\n"
		         "  Available Physical: %.2f MB\n"
		         "  Memory Usage: %.1f%%\n\n"
		         "Process Memory:\n"
		         "  Heap Size: %.2f MB\n"
		         "  Virtual Size: %.2f MB\n\n"
		         "NLib Memory Manager:\n"
		         "  Total Allocated: %.2f MB\n"
		         "  Peak Allocated: %.2f MB\n"
		         "  Allocation Count: %u\n"
		         "  Deallocation Count: %u\n\n"
		         "Garbage Collector:\n"
		         "  Registered Objects: %u\n"
		         "  Total Collections: %llu\n"
		         "  Objects Collected: %llu\n\n"
		         "Object Pools:\n%s",
		         static_cast<double>(SystemInfo.TotalPhysicalMemory) / (1024.0 * 1024.0),
		         static_cast<double>(SystemInfo.AvailablePhysicalMemory) / (1024.0 * 1024.0),
		         SystemInfo.MemoryUsageRatio * 100.0f,
		         static_cast<double>(ProcessInfo.HeapSize) / (1024.0 * 1024.0),
		         static_cast<double>(ProcessInfo.VirtualMemorySize) / (1024.0 * 1024.0),
		         static_cast<double>(MemMgr.GetTotalAllocatedMemory()) / (1024.0 * 1024.0),
		         static_cast<double>(MemMgr.GetPeakAllocatedMemory()) / (1024.0 * 1024.0),
		         MemMgr.GetAllocationCount(),
		         MemMgr.GetDeallocationCount(),
		         GC.GetRegisteredObjectCount(),
		         GC.GetStatistics().TotalCollections,
		         GC.GetStatistics().TotalObjectsCollected,
		         GetPoolManager().GeneratePoolsReport().GetData());

		return CString(Buffer);
	}

public:
	// === 内存压力测试 ===

	/**
	 * @brief 执行内存压力测试
	 */
	static void PerformMemoryStressTest(uint32_t AllocCount = 1000, size_t AllocSize = 1024)
	{
		NLOG_MEMORY(Info, "Starting memory stress test: {} allocations of {} bytes", AllocCount, AllocSize);

		TArray<void*, CMemoryManager> Allocations;
		Allocations.Reserve(AllocCount);

		auto& MemMgr = CMemoryManager::GetInstance();
		CClock TestClock;

		// 分配阶段
		for (uint32_t i = 0; i < AllocCount; ++i)
		{
			void* Ptr = MemMgr.AllocateMemory(AllocSize);
			if (Ptr)
			{
				Allocations.Add(Ptr);
				// 写入数据以确保内存可用
				memset(Ptr, static_cast<int>(i % 256), AllocSize);
			}
		}

		CTimespan AllocTime = TestClock.Lap();

		// 释放阶段
		for (void* Ptr : Allocations)
		{
			MemMgr.DeallocateMemory(Ptr);
		}

		CTimespan DeallocTime = TestClock.GetElapsed();

		NLOG_MEMORY(Info,
		            "Memory stress test completed:\n"
		            "  Successful Allocations: {}\n"
		            "  Allocation Time: {:.2f}ms\n"
		            "  Deallocation Time: {:.2f}ms\n"
		            "  Total Time: {:.2f}ms",
		            Allocations.Size(),
		            AllocTime.GetTotalMilliseconds(),
		            DeallocTime.GetTotalMilliseconds(),
		            (AllocTime + DeallocTime).GetTotalMilliseconds());
	}

public:
	// === 内存优化建议 ===

	/**
	 * @brief 内存优化建议
	 */
	struct SMemoryOptimizationSuggestions
	{
		bool bShouldTriggerGC = false;     // 是否应该触发GC
		bool bShouldShrinkPools = false;   // 是否应该收缩对象池
		bool bShouldCompactMemory = false; // 是否应该压缩内存
		float MemoryPressure = 0.0f;       // 内存压力等级 (0.0-1.0)
		CString Suggestions;               // 具体建议
	};

	/**
	 * @brief 获取内存优化建议
	 */
	static SMemoryOptimizationSuggestions GetOptimizationSuggestions()
	{
		SMemoryOptimizationSuggestions Suggestions;

		auto SystemInfo = GetSystemMemoryInfo();
		auto& MemMgr = CMemoryManager::GetInstance();
		auto& GC = CGarbageCollector::GetInstance();

		float MemoryUsage = SystemInfo.MemoryUsageRatio;
		float HeapUsage = MemMgr.GetMemoryUsageRatio();
		uint32_t RegisteredObjects = GC.GetRegisteredObjectCount();

		Suggestions.MemoryPressure = CMath::Max(MemoryUsage, HeapUsage);

		CString SuggestionText;

		// 高内存使用率
		if (MemoryUsage > 0.85f)
		{
			Suggestions.bShouldTriggerGC = true;
			Suggestions.bShouldShrinkPools = true;
			SuggestionText += "High system memory usage detected. ";
		}

		// 高堆使用率
		if (HeapUsage > 0.8f)
		{
			Suggestions.bShouldTriggerGC = true;
			SuggestionText += "High heap usage detected. ";
		}

		// 大量注册对象
		if (RegisteredObjects > 10000)
		{
			Suggestions.bShouldTriggerGC = true;
			SuggestionText += "Large number of managed objects. ";
		}

		// 内存碎片
		if (MemMgr.GetFragmentationRatio() > 0.3f)
		{
			Suggestions.bShouldCompactMemory = true;
			SuggestionText += "High memory fragmentation detected. ";
		}

		if (SuggestionText.IsEmpty())
		{
			SuggestionText = "Memory usage is optimal. ";
		}

		Suggestions.Suggestions = SuggestionText;
		return Suggestions;
	}

	/**
	 * @brief 应用内存优化建议
	 */
	static void ApplyOptimizationSuggestions()
	{
		auto Suggestions = GetOptimizationSuggestions();

		NLOG_MEMORY(Info, "Applying memory optimization suggestions: {}", Suggestions.Suggestions.GetData());

		if (Suggestions.bShouldTriggerGC)
		{
			GetGC().RequestGC(EGCType::Major);
		}

		if (Suggestions.bShouldShrinkPools)
		{
			GetPoolManager().ShrinkAllPools();
		}

		if (Suggestions.bShouldCompactMemory)
		{
			// 执行内存压缩（如果内存管理器支持）
			auto& MemMgr = CMemoryManager::GetInstance();
			// MemMgr.CompactMemory(); // 假设的API
		}
	}
};

// === 便捷函数 ===

/**
 * @brief 获取内存管理器
 */
inline CMemoryManager& GetMemoryManager()
{
	return CMemoryManager::GetInstance();
}

/**
 * @brief 生成完整的内存报告
 */
inline CString GenerateFullMemoryReport()
{
	return CMemoryUtils::GenerateMemoryReport();
}

/**
 * @brief 执行内存清理
 */
inline void PerformMemoryCleanup()
{
	NLOG_MEMORY(Info, "Performing memory cleanup");

	// 强制GC
	GetGC().ForceGC(EGCType::Full);

	// 收缩对象池
	GetPoolManager().ShrinkAllPools();

	// 应用优化建议
	CMemoryUtils::ApplyOptimizationSuggestions();

	NLOG_MEMORY(Info, "Memory cleanup completed");
}

// === 类型别名 ===
using MemoryUtils = CMemoryUtils;

} // namespace NLib