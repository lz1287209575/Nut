#pragma once

/**
 * @file Threading.h
 * @brief NLib线程系统库主头文件
 *
 * 提供完整的多线程编程功能：
 * - 线程管理（NThread, IRunnable）
 * - 任务系统（TTask, TFuture, TPromise）
 * - 线程池（NThreadPool）
 * - 异步操作（CAsync）
 * - 线程安全容器
 */

// 基础线程管理
#include "Thread.h"

// 任务系统
#include "Task.h"

// 线程池
#include "ThreadPool.h"

// 异步操作
#include "Async.h"

// 协程系统
#include "Coroutine.h"
#include "CoroutineScheduler.h"
#include "CoroutineUtils.h"

// 线程安全容器
#include "Containers/TQueue.h"

namespace NLib
{
/**
 * @brief 线程系统工具类
 */
class CThreadingUtils
{
public:
	// === 系统信息 ===

	/**
	 * @brief 获取系统线程数
	 */
	static uint32_t GetHardwareConcurrency()
	{
		return std::thread::hardware_concurrency();
	}

	/**
	 * @brief 获取建议的线程池大小
	 */
	static uint32_t GetRecommendedThreadPoolSize()
	{
		uint32_t CoreCount = GetHardwareConcurrency();
		return CoreCount > 0 ? CoreCount : 4;
	}

	/**
	 * @brief 获取建议的最大线程池大小
	 */
	static uint32_t GetRecommendedMaxThreadPoolSize()
	{
		return GetRecommendedThreadPoolSize() * 2;
	}

public:
	// === 性能测试 ===

	/**
	 * @brief 线程性能测试结果
	 */
	struct SThreadPerformanceTest
	{
		uint32_t ThreadCount = 0;      // 线程数量
		uint32_t TasksPerThread = 0;   // 每线程任务数
		CTimespan TotalExecutionTime;  // 总执行时间
		CTimespan AverageTaskTime;     // 平均任务时间
		double TasksPerSecond = 0.0;   // 每秒任务数
		double ThreadEfficiency = 0.0; // 线程效率
	};

	/**
	 * @brief 执行线程性能测试
	 */
	static SThreadPerformanceTest TestThreadPerformance(uint32_t ThreadCount, uint32_t TasksPerThread)
	{
		SThreadPerformanceTest Result;
		Result.ThreadCount = ThreadCount;
		Result.TasksPerThread = TasksPerThread;

		auto ThreadPool = CreateThreadPool(
		    SThreadPoolConfig{.MinThreads = ThreadCount, .MaxThreads = ThreadCount, .bPrestart = true});

		if (!ThreadPool.IsValid())
		{
			return Result;
		}

		CClock TestClock;
		TArray<TFuture<void>, CMemoryManager> Futures;
		Futures.Reserve(ThreadCount * TasksPerThread);

		// 提交所有任务
		for (uint32_t i = 0; i < ThreadCount * TasksPerThread; ++i)
		{
			auto Future = ThreadPool->SubmitTask(
			    []() {
				    // 模拟CPU密集型工作
				    volatile uint64_t Sum = 0;
				    for (uint32_t j = 0; j < 10000; ++j)
				    {
					    Sum += j * j;
				    }
			    },
			    TString("PerfTestTask"));

			Futures.Add(std::move(Future));
		}

		// 等待所有任务完成
		for (auto& Future : Futures)
		{
			Future.Wait();
		}

		Result.TotalExecutionTime = TestClock.GetElapsed();

		uint32_t TotalTasks = ThreadCount * TasksPerThread;
		Result.AverageTaskTime = CTimespan::FromSeconds(Result.TotalExecutionTime.GetTotalSeconds() / TotalTasks);
		Result.TasksPerSecond = TotalTasks / Result.TotalExecutionTime.GetTotalSeconds();
		Result.ThreadEfficiency = Result.TasksPerSecond / ThreadCount;

		return Result;
	}

	/**
	 * @brief 测试线程池扩展性
	 */
	struct SThreadPoolScalabilityTest
	{
		TArray<SThreadPerformanceTest, CMemoryManager> Results;
		uint32_t OptimalThreadCount = 0;
		double BestEfficiency = 0.0;
	};

	static SThreadPoolScalabilityTest TestThreadPoolScalability(uint32_t MaxThreads = 0, uint32_t TasksPerThread = 100)
	{
		SThreadPoolScalabilityTest Result;

		if (MaxThreads == 0)
		{
			MaxThreads = GetRecommendedMaxThreadPoolSize();
		}

		Result.Results.Reserve(MaxThreads);

		for (uint32_t ThreadCount = 1; ThreadCount <= MaxThreads; ++ThreadCount)
		{
			auto TestResult = TestThreadPerformance(ThreadCount, TasksPerThread);
			Result.Results.Add(TestResult);

			if (TestResult.ThreadEfficiency > Result.BestEfficiency)
			{
				Result.BestEfficiency = TestResult.ThreadEfficiency;
				Result.OptimalThreadCount = ThreadCount;
			}
		}

		return Result;
	}

public:
	// === 内存使用分析 ===

	/**
	 * @brief 线程系统内存使用信息
	 */
	struct SThreadingMemoryInfo
	{
		size_t ThreadMemoryUsage = 0;     // 线程内存使用
		size_t TaskMemoryUsage = 0;       // 任务内存使用
		size_t ThreadPoolMemoryUsage = 0; // 线程池内存使用
		size_t TotalMemoryUsage = 0;      // 总内存使用
		uint32_t TotalThreads = 0;        // 总线程数
		uint32_t TotalTasks = 0;          // 总任务数
		uint32_t TotalThreadPools = 0;    // 总线程池数
	};

	/**
	 * @brief 获取线程系统内存使用信息
	 */
	static SThreadingMemoryInfo GetMemoryInfo()
	{
		SThreadingMemoryInfo Info;

		// 粗略估算内存使用
		Info.ThreadMemoryUsage = Info.TotalThreads * (sizeof(NThread) + 8192); // 假设每线程8KB堆栈
		Info.TaskMemoryUsage = Info.TotalTasks * sizeof(TTask<void>);
		Info.ThreadPoolMemoryUsage = Info.TotalThreadPools * sizeof(NThreadPool);
		Info.TotalMemoryUsage = Info.ThreadMemoryUsage + Info.TaskMemoryUsage + Info.ThreadPoolMemoryUsage;

		return Info;
	}

public:
	// === 并发模式实现 ===

	/**
	 * @brief 并行For循环
	 */
	template <typename TFunc>
	static void ParallelFor(int32_t StartIndex, int32_t EndIndex, TFunc&& Function, uint32_t ThreadCount = 0)
	{
		if (ThreadCount == 0)
		{
			ThreadCount = GetRecommendedThreadPoolSize();
		}

		if (StartIndex >= EndIndex)
		{
			return;
		}

		int32_t TotalCount = EndIndex - StartIndex;
		int32_t ChunkSize = (TotalCount + ThreadCount - 1) / ThreadCount;

		TArray<TFuture<void>, CMemoryManager> Futures;
		Futures.Reserve(ThreadCount);

		for (uint32_t i = 0; i < ThreadCount; ++i)
		{
			int32_t ChunkStart = StartIndex + i * ChunkSize;
			int32_t ChunkEnd = ChunkStart + ChunkSize < EndIndex ? ChunkStart + ChunkSize : EndIndex;

			if (ChunkStart >= ChunkEnd)
			{
				break;
			}

			auto Future = Async(
			    [Function, ChunkStart, ChunkEnd]() {
				    for (int32_t Index = ChunkStart; Index < ChunkEnd; ++Index)
				    {
					    Function(Index);
				    }
			    },
			    TString("ParallelForChunk"));

			Futures.Add(std::move(Future));
		}

		// 等待所有任务完成
		for (auto& Future : Futures)
		{
			Future.Wait();
		}
	}

	/**
	 * @brief 并行处理容器
	 */
	template <typename TContainer, typename TFunc>
	static void ParallelForEach(const TContainer& Container, TFunc&& Function, uint32_t ThreadCount = 0)
	{
		if (ThreadCount == 0)
		{
			ThreadCount = GetRecommendedThreadPoolSize();
		}

		if (Container.IsEmpty())
		{
			return;
		}

		int32_t TotalCount = Container.Size();
		int32_t ChunkSize = (TotalCount + ThreadCount - 1) / ThreadCount;

		TArray<TFuture<void>, CMemoryManager> Futures;
		Futures.Reserve(ThreadCount);

		for (uint32_t i = 0; i < ThreadCount; ++i)
		{
			int32_t ChunkStart = i * ChunkSize;
			int32_t ChunkEnd = ChunkStart + ChunkSize < TotalCount ? ChunkStart + ChunkSize : TotalCount;

			if (ChunkStart >= ChunkEnd)
			{
				break;
			}

			auto Future = Async(
			    [&Container, Function, ChunkStart, ChunkEnd]() {
				    for (int32_t Index = ChunkStart; Index < ChunkEnd; ++Index)
				    {
					    Function(Container[Index]);
				    }
			    },
			    TString("ParallelForEachChunk"));

			Futures.Add(std::move(Future));
		}

		// 等待所有任务完成
		for (auto& Future : Futures)
		{
			Future.Wait();
		}
	}

	/**
	 * @brief Map-Reduce模式
	 */
	template <typename TContainer, typename TMapFunc, typename TReduceFunc>
	static auto MapReduce(const TContainer& Container,
	                      TMapFunc&& MapFunction,
	                      TReduceFunc&& ReduceFunction,
	                      uint32_t ThreadCount = 0)
	{
		using MapResult = std::invoke_result_t<TMapFunc, typename TContainer::ElementType>;

		if (ThreadCount == 0)
		{
			ThreadCount = GetRecommendedThreadPoolSize();
		}

		if (Container.IsEmpty())
		{
			throw std::runtime_error("Cannot perform MapReduce on empty container");
		}

		// Map阶段
		TArray<TFuture<TArray<MapResult, CMemoryManager>>, CMemoryManager> MapFutures;
		MapFutures.Reserve(ThreadCount);

		int32_t TotalCount = Container.Size();
		int32_t ChunkSize = (TotalCount + ThreadCount - 1) / ThreadCount;

		for (uint32_t i = 0; i < ThreadCount; ++i)
		{
			int32_t ChunkStart = i * ChunkSize;
			int32_t ChunkEnd = ChunkStart + ChunkSize < TotalCount ? ChunkStart + ChunkSize : TotalCount;

			if (ChunkStart >= ChunkEnd)
			{
				break;
			}

			auto Future = Async(
			    [&Container, MapFunction, ChunkStart, ChunkEnd]() -> TArray<MapResult, CMemoryManager> {
				    TArray<MapResult, CMemoryManager> Results;
				    Results.Reserve(ChunkEnd - ChunkStart);

				    for (int32_t Index = ChunkStart; Index < ChunkEnd; ++Index)
				    {
					    Results.Add(MapFunction(Container[Index]));
				    }

				    return Results;
			    },
			    TString("MapPhase"));

			MapFutures.Add(std::move(Future));
		}

		// 收集Map结果
		TArray<MapResult, CMemoryManager> AllMapResults;
		for (auto& Future : MapFutures)
		{
			auto ChunkResults = Future.Get();
			for (auto& Result : ChunkResults)
			{
				AllMapResults.Add(std::move(Result));
			}
		}

		// Reduce阶段
		auto ReduceResult = AllMapResults[0];
		for (int32_t i = 1; i < AllMapResults.Size(); ++i)
		{
			ReduceResult = ReduceFunction(ReduceResult, AllMapResults[i]);
		}

		return ReduceResult;
	}

public:
	// === 协程并发模式 ===

	/**
	 * @brief 协程并行For循环
	 */
	template <typename TFunc>
	static void CoroutineParallelFor(int32_t StartIndex,
	                                 int32_t EndIndex,
	                                 TFunc&& Function,
	                                 uint32_t CoroutineCount = 0)
	{
		if (CoroutineCount == 0)
		{
			CoroutineCount = GetRecommendedThreadPoolSize();
		}

		if (StartIndex >= EndIndex)
		{
			return;
		}

		int32_t TotalCount = EndIndex - StartIndex;
		int32_t ChunkSize = (TotalCount + CoroutineCount - 1) / CoroutineCount;

		TArray<TSharedPtr<NCoroutine>, CMemoryManager> Coroutines;
		Coroutines.Reserve(CoroutineCount);

		for (uint32_t i = 0; i < CoroutineCount; ++i)
		{
			int32_t ChunkStart = StartIndex + i * ChunkSize;
			int32_t ChunkEnd = ChunkStart + ChunkSize < EndIndex ? ChunkStart + ChunkSize : EndIndex;

			if (ChunkStart >= ChunkEnd)
			{
				break;
			}

			TString CoroutineName = TString("ParallelForCoroutine") + TString::FromInt(i);
			auto Coroutine = StartCoroutine(
			    [Function, ChunkStart, ChunkEnd]() {
				    for (int32_t Index = ChunkStart; Index < ChunkEnd; ++Index)
				    {
					    Function(Index);

					    // 定期让出执行权
					    if ((Index - ChunkStart) % 100 == 0)
					    {
						    CoroutineYield();
					    }
				    }
			    },
			    CoroutineName);

			if (Coroutine.IsValid())
			{
				Coroutines.Add(Coroutine);
			}
		}

		// 等待所有协程完成
		NCoroutineUtils::WaitAll(Coroutines);
	}

	/**
	 * @brief 协程并行处理容器
	 */
	template <typename TContainer, typename TFunc>
	static void CoroutineParallelForEach(const TContainer& Container, TFunc&& Function, uint32_t CoroutineCount = 0)
	{
		if (CoroutineCount == 0)
		{
			CoroutineCount = GetRecommendedThreadPoolSize();
		}

		if (Container.IsEmpty())
		{
			return;
		}

		int32_t TotalCount = Container.Size();
		int32_t ChunkSize = (TotalCount + CoroutineCount - 1) / CoroutineCount;

		TArray<TSharedPtr<NCoroutine>, CMemoryManager> Coroutines;
		Coroutines.Reserve(CoroutineCount);

		for (uint32_t i = 0; i < CoroutineCount; ++i)
		{
			int32_t ChunkStart = i * ChunkSize;
			int32_t ChunkEnd = ChunkStart + ChunkSize < TotalCount ? ChunkStart + ChunkSize : TotalCount;

			if (ChunkStart >= ChunkEnd)
			{
				break;
			}

			TString CoroutineName = TString("ParallelForEachCoroutine") + TString::FromInt(i);
			auto Coroutine = StartCoroutine(
			    [&Container, Function, ChunkStart, ChunkEnd]() {
				    for (int32_t Index = ChunkStart; Index < ChunkEnd; ++Index)
				    {
					    Function(Container[Index]);

					    // 定期让出执行权
					    if ((Index - ChunkStart) % 100 == 0)
					    {
						    CoroutineYield();
					    }
				    }
			    },
			    CoroutineName);

			if (Coroutine.IsValid())
			{
				Coroutines.Add(Coroutine);
			}
		}

		// 等待所有协程完成
		NCoroutineUtils::WaitAll(Coroutines);
	}

public:
	// === 综合报告生成 ===

	/**
	 * @brief 生成线程系统综合报告
	 */
	static TString GenerateComprehensiveReport()
	{
		auto MemoryInfo = GetMemoryInfo();
		auto& Scheduler = GetCoroutineScheduler();
		auto SchedulerStats = Scheduler.GetStats();

		char Buffer[4096];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Threading System Comprehensive Report ===\n\n"
		         "System Information:\n"
		         "  Hardware Concurrency: %u\n"
		         "  Recommended Thread Pool Size: %u\n"
		         "  Recommended Max Thread Pool Size: %u\n\n"
		         "Memory Usage:\n"
		         "  Thread Memory: %.2f KB\n"
		         "  Task Memory: %.2f KB\n"
		         "  ThreadPool Memory: %.2f KB\n"
		         "  Total Memory: %.2f KB\n\n"
		         "Active Resources:\n"
		         "  Total Threads: %u\n"
		         "  Total Tasks: %u\n"
		         "  Total Thread Pools: %u\n\n"
		         "Coroutine System:\n"
		         "  Scheduler Running: %s\n"
		         "  Total Coroutines: %u\n"
		         "  Active Coroutines: %u\n"
		         "  Suspended Coroutines: %u\n"
		         "  Completed Coroutines: %u\n"
		         "  Failed Coroutines: %u\n"
		         "  Scheduling Cycles: %u\n"
		         "  Average Scheduling Time: %.3f ms\n"
		         "  Default Stack Size: %zu bytes\n",
		         GetHardwareConcurrency(),
		         GetRecommendedThreadPoolSize(),
		         GetRecommendedMaxThreadPoolSize(),
		         static_cast<double>(MemoryInfo.ThreadMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.TaskMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.ThreadPoolMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.TotalMemoryUsage) / 1024.0,
		         MemoryInfo.TotalThreads,
		         MemoryInfo.TotalTasks,
		         MemoryInfo.TotalThreadPools,
		         Scheduler.IsRunning() ? "Yes" : "No",
		         SchedulerStats.TotalCoroutines,
		         SchedulerStats.ActiveCoroutines,
		         SchedulerStats.SuspendedCoroutines,
		         SchedulerStats.CompletedCoroutines,
		         SchedulerStats.FailedCoroutines,
		         SchedulerStats.TotalSchedulingCycles,
		         SchedulerStats.AverageSchedulingTime.GetTotalMilliseconds(),
		         DEFAULT_COROUTINE_STACK_SIZE);

		return TString(Buffer);
	}
};

// === 全局默认线程池 ===

/**
 * @brief 获取全局默认线程池
 */
inline TSharedPtr<NThreadPool> GetDefaultThreadPool()
{
	return CAsync::GetDefaultThreadPool();
}

/**
 * @brief 设置全局默认线程池
 */
inline void SetDefaultThreadPool(TSharedPtr<NThreadPool> ThreadPool)
{
	CAsync::SetDefaultThreadPool(ThreadPool);
}

// === 全局协程管理 ===

/**
 * @brief 初始化协程系统
 */
inline bool InitializeCoroutineSystem(const SCoroutineSchedulerConfig& Config = SCoroutineSchedulerConfig{})
{
	return GetCoroutineScheduler().Initialize(Config);
}

/**
 * @brief 关闭协程系统
 */
inline void ShutdownCoroutineSystem()
{
	GetCoroutineScheduler().Shutdown();
}

/**
 * @brief 运行协程调度器
 */
inline void RunCoroutineScheduler(float TickInterval = 0.016f)
{
	GetCoroutineScheduler().RunUntilComplete(TickInterval);
}

/**
 * @brief 执行一轮协程调度
 */
inline void TickCoroutineScheduler(float DeltaTime = 0.0f)
{
	GetCoroutineScheduler().Tick(DeltaTime);
}

// === 便捷类型别名 ===
using ThreadingUtils = CThreadingUtils;

} // namespace NLib