#include "NLib.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace NLib
{
// 库状态
static std::atomic<bool> GInitialized{false};
static std::mutex GInitMutex;

// 统计数据
static MemoryStats GMemoryStats;
static PerformanceStats GPerformanceStats;
static std::mutex GStatsMutex;

// 初始化时间戳
static std::chrono::steady_clock::time_point GInitTime;

bool Initialize()
{
	std::lock_guard<std::mutex> Lock(GInitMutex);

	if (GInitialized.load())
	{
		NLIB_LOG_WARNING("NLib::Initialize() called multiple times");
		return true;
	}

	try
	{
		GInitTime = std::chrono::steady_clock::now();

		// 初始化内存管理器
		CMemoryManager::GetInstance().Initialize();
		if (!CMemoryManager::GetInstance().IsInitialize())
		{
			NLIB_LOG_ERROR("Failed to initialize CMemoryManager");
			return false;
		}

		// 初始化垃圾回收器
		CGarbageCollector::GetInstance().Initialize();
		if (!CGarbageCollector::GetInstance().IsInitialize())
		{
			NLIB_LOG_ERROR("Failed to initialize CGarbageCollector");
			return false;
		}

		// 初始化日志系统
		CLogger::Info("NLib {} initialized successfully", NLIB_VERSION);

		// 重置统计数据
		{
			std::lock_guard<std::mutex> StatsLock(GStatsMutex);
			GMemoryStats = MemoryStats();
			GPerformanceStats = PerformanceStats();
		}

		GInitialized.store(true);
		return true;
	}
	catch (const std::exception& e)
	{
		NLIB_LOG_ERROR("Exception during NLib initialization: {}", e.what());
		return false;
	}
	catch (...)
	{
		NLIB_LOG_ERROR("Unknown exception during NLib initialization");
		return false;
	}
}

void Shutdown()
{
	std::lock_guard<std::mutex> Lock(GInitMutex);

	if (!GInitialized.load())
	{
		NLIB_LOG_WARNING("NLib::Shutdown() called but library not initialized");
		return;
	}

	try
	{
		// 强制执行最后一次垃圾回收
		ForceGarbageCollect();

		// 计算运行时间
		auto ShutdownTime = std::chrono::steady_clock::now();
		auto RuntimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(ShutdownTime - GInitTime).count();

		// 输出最终统计信息
		MemoryStats FinalMemory = GetMemoryStats();
		PerformanceStats FinalPerf = GetPerformanceStats();

		CLogger::Info("NLib shutdown - Runtime: {}ms", RuntimeMs);
		CLogger::Info("Final Memory Stats - Allocated: {} bytes, Peak: {} bytes",
		              FinalMemory.TotalAllocated,
		              FinalMemory.PeakUsage);
		CLogger::Info(
		    "Final Performance Stats - Objects: {}, GC Runs: {}", FinalPerf.ObjectCreations, FinalPerf.GcRuns);

		// 清理完成

		// 关闭垃圾回收器
		CGarbageCollector::GetInstance().Shutdown();

		// 关闭内存管理器
		CMemoryManager::GetInstance().Shutdown();

		GInitialized.store(false);
	}
	catch (const std::exception& e)
	{
		NLIB_LOG_ERROR("Exception during NLib shutdown: {}", e.what());
	}
	catch (...)
	{
		NLIB_LOG_ERROR("Unknown exception during NLib shutdown");
	}
}

bool IsInitialized()
{
	return GInitialized.load();
}

MemoryStats GetMemoryStats()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);

	// 从内存管理器获取实时数据
	const auto& MemoryManager = CMemoryManager::GetInstance();
	auto MemStats = MemoryManager.GetStats();
	GMemoryStats.TotalAllocated = MemStats.TotalAllocated;
	GMemoryStats.TotalUsed = MemStats.CurrentUsage;
	GMemoryStats.PeakUsage = MemStats.PeakUsage;

	// 从垃圾回收器获取对象计数
	const auto& GC = CGarbageCollector::GetInstance();
	GMemoryStats.GcObjectsCount = GC.GetRegisteredObjectCount();

	// TODO: 从容器系统获取容器计数
	GMemoryStats.ContainerCount = 0; // 暂时为0

	return GMemoryStats;
}

void ForceGarbageCollect()
{
	if (!IsInitialized())
	{
		NLIB_LOG_WARNING("ForceGarbageCollect() called but NLib not initialized");
		return;
	}

	auto StartTime = std::chrono::high_resolution_clock::now();

	// 执行垃圾回收
	CGarbageCollector::GetInstance().Collect();

	auto EndTime = std::chrono::high_resolution_clock::now();
	auto DurationMs = std::chrono::duration<double, std::milli>(EndTime - StartTime).count();

	// 更新统计数据
	{
		std::lock_guard<std::mutex> Lock(GStatsMutex);
		GPerformanceStats.GcRuns++;

		// 更新平均GC时间
		double TotalTime = GPerformanceStats.AverageGcTimeMs * (GPerformanceStats.GcRuns - 1);
		GPerformanceStats.AverageGcTimeMs = (TotalTime + DurationMs) / GPerformanceStats.GcRuns;
	}

	NLIB_LOG_INFO("Garbage collection completed in {:.2f}ms", DurationMs);
}

PerformanceStats GetPerformanceStats()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);

	// TODO: 从各个模块收集性能数据
	// 目前返回存储的统计数据
	return GPerformanceStats;
}

void ResetStats()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);
	GMemoryStats = MemoryStats();
	GPerformanceStats = PerformanceStats();

	NLIB_LOG_INFO("NLib statistics reset");
}

// 便利创建函数实现
CString MakeString(const char* str)
{
	return CString(str);
}

CString MakeString(const std::string& str)
{
	return CString(str.c_str());
}

CString MakeString(std::string_view str)
{
	return CString(str);
}

// 内部函数：更新性能统计 (供其他模块调用)
namespace Detail
{
void IncrementObjectCreations()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);
	GPerformanceStats.ObjectCreations++;
}

void IncrementObjectDestructions()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);
	GPerformanceStats.ObjectDestructions++;
}

void IncrementMemoryAllocations()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);
	GPerformanceStats.MemoryAllocations++;
}

void IncrementMemoryDeallocations()
{
	std::lock_guard<std::mutex> Lock(GStatsMutex);
	GPerformanceStats.MemoryDeallocations++;
}
} // namespace Detail
} // namespace NLib
