#pragma once

#include "Containers/TArray.h"
#include "Containers/TString.h"
#include "Core/SmartPointers.h"
#include "Coroutine.h"
#include "CoroutineScheduler.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <functional>
#include <mutex>

namespace NLib
{
/**
 * @brief 协程同步原语 - 信号量
 */
class CCoroutineSemaphore
{
public:
	explicit CCoroutineSemaphore(uint32_t InitialCount = 1)
	    : Count(InitialCount),
	      MaxCount(InitialCount)
	{}

	/**
	 * @brief 获取信号量
	 */
	void Acquire()
	{
		CoroutineWaitFor(
		    [this]() {
			    std::lock_guard<std::mutex> Lock(Mutex);
			    if (Count > 0)
			    {
				    Count--;
				    return true;
			    }
			    return false;
		    },
		    TString("SemaphoreAcquire"));
	}

	/**
	 * @brief 尝试获取信号量
	 */
	bool TryAcquire()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		if (Count > 0)
		{
			Count--;
			return true;
		}
		return false;
	}

	/**
	 * @brief 释放信号量
	 */
	void Release()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		if (Count < MaxCount)
		{
			Count++;
		}
	}

	/**
	 * @brief 获取当前计数
	 */
	uint32_t GetCount() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		return Count;
	}

private:
	std::atomic<uint32_t> Count;
	uint32_t MaxCount;
	mutable std::mutex Mutex;
};

/**
 * @brief 协程同步原语 - 互斥锁
 */
class CCoroutineMutex
{
public:
	CCoroutineMutex()
	    : bIsLocked(false),
	      OwnerCoroutineId(INVALID_COROUTINE_ID)
	{}

	/**
	 * @brief 加锁
	 */
	void Lock()
	{
		auto CurrentCoroutine = GetCoroutineScheduler().GetCurrentCoroutine();
		FCoroutineId CurrentId = CurrentCoroutine.IsValid() ? CurrentCoroutine->GetCoroutineId() : INVALID_COROUTINE_ID;

		CoroutineWaitFor(
		    [this, CurrentId]() {
			    std::lock_guard<std::mutex> Lock(Mutex);
			    if (!bIsLocked)
			    {
				    bIsLocked = true;
				    OwnerCoroutineId = CurrentId;
				    return true;
			    }
			    return false;
		    },
		    TString("MutexLock"));
	}

	/**
	 * @brief 尝试加锁
	 */
	bool TryLock()
	{
		auto CurrentCoroutine = GetCoroutineScheduler().GetCurrentCoroutine();
		FCoroutineId CurrentId = CurrentCoroutine.IsValid() ? CurrentCoroutine->GetCoroutineId() : INVALID_COROUTINE_ID;

		std::lock_guard<std::mutex> Lock(Mutex);
		if (!bIsLocked)
		{
			bIsLocked = true;
			OwnerCoroutineId = CurrentId;
			return true;
		}
		return false;
	}

	/**
	 * @brief 解锁
	 */
	void Unlock()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		if (bIsLocked)
		{
			bIsLocked = false;
			OwnerCoroutineId = INVALID_COROUTINE_ID;
		}
	}

	/**
	 * @brief 检查是否已锁定
	 */
	bool IsLocked() const
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		return bIsLocked;
	}

private:
	std::atomic<bool> bIsLocked;
	std::atomic<FCoroutineId> OwnerCoroutineId;
	mutable std::mutex Mutex;
};

/**
 * @brief 协程同步原语 - 条件变量
 */
class CCoroutineConditionVariable
{
public:
	CCoroutineConditionVariable() = default;

	/**
	 * @brief 等待条件变量
	 */
	void Wait(CCoroutineMutex& Mutex)
	{
		Mutex.Unlock();

		CoroutineWaitFor(
		    [this]() {
			    std::lock_guard<std::mutex> Lock(NotifyMutex);
			    return bNotified;
		    },
		    TString("ConditionWait"));

		Mutex.Lock();
	}

	/**
	 * @brief 带超时的等待
	 */
	bool WaitFor(CCoroutineMutex& Mutex, const CTimespan& Timeout)
	{
		Mutex.Unlock();

		bool Result = false;
		auto CurrentCoroutine = GetCoroutineScheduler().GetCurrentCoroutine();
		if (CurrentCoroutine.IsValid())
		{
			auto WaitCondition = CreateConditionWait(
			    [this]() {
				    std::lock_guard<std::mutex> Lock(NotifyMutex);
				    return bNotified;
			    },
			    TString("ConditionWaitTimeout"));

			Result = CurrentCoroutine->WaitForCondition(WaitCondition, Timeout);
		}

		Mutex.Lock();
		return Result;
	}

	/**
	 * @brief 通知一个等待的协程
	 */
	void NotifyOne()
	{
		std::lock_guard<std::mutex> Lock(NotifyMutex);
		bNotified = true;
	}

	/**
	 * @brief 通知所有等待的协程
	 */
	void NotifyAll()
	{
		std::lock_guard<std::mutex> Lock(NotifyMutex);
		bNotified = true;
		bNotifyAll = true;
	}

	/**
	 * @brief 重置通知状态
	 */
	void Reset()
	{
		std::lock_guard<std::mutex> Lock(NotifyMutex);
		bNotified = false;
		bNotifyAll = false;
	}

private:
	std::atomic<bool> bNotified{false};
	std::atomic<bool> bNotifyAll{false};
	mutable std::mutex NotifyMutex;
};

/**
 * @brief 协程通道 - 用于协程间通信
 */
template <typename T>
class TCoroutineChannel
{
public:
	explicit TCoroutineChannel(uint32_t BufferSize = 0)
	    : MaxBufferSize(BufferSize),
	      bIsClosed(false)
	{
		if (MaxBufferSize > 0)
		{
			Buffer.Reserve(MaxBufferSize);
		}
	}

	/**
	 * @brief 发送数据
	 */
	bool Send(const T& Value)
	{
		if (bIsClosed.load())
		{
			return false;
		}

		// 如果有缓冲区，先尝试放入缓冲区
		if (MaxBufferSize > 0)
		{
			std::lock_guard<std::mutex> Lock(BufferMutex);
			if (Buffer.Size() < MaxBufferSize)
			{
				Buffer.Add(Value);
				return true;
			}
		}

		// 等待接收者准备好
		CoroutineWaitFor([this]() { return bIsClosed.load() || HasReceiver(); }, TString("ChannelSend"));

		if (bIsClosed.load())
		{
			return false;
		}

		// 直接传递给接收者
		std::lock_guard<std::mutex> Lock(TransferMutex);
		TransferValue = Value;
		bHasValue = true;

		return true;
	}

	/**
	 * @brief 发送数据（移动语义）
	 */
	bool Send(T&& Value)
	{
		if (bIsClosed.load())
		{
			return false;
		}

		// 如果有缓冲区，先尝试放入缓冲区
		if (MaxBufferSize > 0)
		{
			std::lock_guard<std::mutex> Lock(BufferMutex);
			if (Buffer.Size() < MaxBufferSize)
			{
				Buffer.Add(std::move(Value));
				return true;
			}
		}

		// 等待接收者准备好
		CoroutineWaitFor([this]() { return bIsClosed.load() || HasReceiver(); }, TString("ChannelSend"));

		if (bIsClosed.load())
		{
			return false;
		}

		// 直接传递给接收者
		std::lock_guard<std::mutex> Lock(TransferMutex);
		TransferValue = std::move(Value);
		bHasValue = true;

		return true;
	}

	/**
	 * @brief 接收数据
	 */
	bool Receive(T& OutValue)
	{
		// 先检查缓冲区
		if (MaxBufferSize > 0)
		{
			std::lock_guard<std::mutex> Lock(BufferMutex);
			if (!Buffer.IsEmpty())
			{
				OutValue = Buffer[0];
				Buffer.RemoveAt(0);
				return true;
			}
		}

		if (bIsClosed.load())
		{
			return false;
		}

		// 等待发送者
		CoroutineWaitFor([this]() { return bIsClosed.load() || HasValue(); }, TString("ChannelReceive"));

		if (bIsClosed.load())
		{
			return false;
		}

		// 获取值
		std::lock_guard<std::mutex> Lock(TransferMutex);
		if (bHasValue)
		{
			OutValue = std::move(TransferValue);
			bHasValue = false;
			return true;
		}

		return false;
	}

	/**
	 * @brief 尝试接收数据（非阻塞）
	 */
	bool TryReceive(T& OutValue)
	{
		// 先检查缓冲区
		if (MaxBufferSize > 0)
		{
			std::lock_guard<std::mutex> Lock(BufferMutex);
			if (!Buffer.IsEmpty())
			{
				OutValue = Buffer[0];
				Buffer.RemoveAt(0);
				return true;
			}
		}

		// 检查直接传递的值
		std::lock_guard<std::mutex> Lock(TransferMutex);
		if (bHasValue)
		{
			OutValue = std::move(TransferValue);
			bHasValue = false;
			return true;
		}

		return false;
	}

	/**
	 * @brief 关闭通道
	 */
	void Close()
	{
		bIsClosed = true;
	}

	/**
	 * @brief 检查通道是否关闭
	 */
	bool IsClosed() const
	{
		return bIsClosed.load();
	}

	/**
	 * @brief 检查缓冲区大小
	 */
	uint32_t GetBufferSize() const
	{
		if (MaxBufferSize > 0)
		{
			std::lock_guard<std::mutex> Lock(BufferMutex);
			return Buffer.Size();
		}
		return 0;
	}

private:
	bool HasValue() const
	{
		std::lock_guard<std::mutex> Lock(TransferMutex);
		return bHasValue;
	}

	bool HasReceiver() const
	{
		// 简化实现：假设总是有接收者准备好
		return true;
	}

private:
	uint32_t MaxBufferSize;
	std::atomic<bool> bIsClosed;

	// 缓冲区
	TArray<T, CMemoryManager> Buffer;
	mutable std::mutex BufferMutex;

	// 直接传递
	T TransferValue;
	std::atomic<bool> bHasValue{false};
	mutable std::mutex TransferMutex;
};

/**
 * @brief 协程池 - 管理一组协程
 */
class CCoroutinePool
{
public:
	explicit CCoroutinePool(uint32_t PoolSize = 8)
	    : MaxPoolSize(PoolSize),
	      bIsRunning(false)
	{
		WorkerCoroutines.Reserve(MaxPoolSize);
	}

	~CCoroutinePool()
	{
		Shutdown();
	}

	/**
	 * @brief 启动协程池
	 */
	bool Start()
	{
		if (bIsRunning)
		{
			return true;
		}

		bIsRunning = true;

		// 创建工作协程
		for (uint32_t i = 0; i < MaxPoolSize; ++i)
		{
			TString CoroutineName = TString("PoolWorker") + TString::FromInt(i);
			auto WorkerCoroutine = StartCoroutine(
			    [this]() { WorkerLoop(); }, CoroutineName, ECoroutinePriority::Normal);

			if (WorkerCoroutine.IsValid())
			{
				WorkerCoroutines.Add(WorkerCoroutine);
			}
		}

		NLOG_THREADING(Info, "CoroutinePool started with {} workers", WorkerCoroutines.Size());
		return true;
	}

	/**
	 * @brief 关闭协程池
	 */
	void Shutdown()
	{
		if (!bIsRunning)
		{
			return;
		}

		bIsRunning = false;
		TaskChannel.Close();

		// 等待所有工作协程完成
		for (auto& Worker : WorkerCoroutines)
		{
			if (Worker.IsValid())
			{
				Worker->Cancel();
			}
		}

		WorkerCoroutines.Empty();

		NLOG_THREADING(Info, "CoroutinePool shutdown complete");
	}

	/**
	 * @brief 提交任务
	 */
	template <typename TFunc>
	bool SubmitTask(TFunc&& Function, const TString& TaskName = TString("PoolTask"))
	{
		if (!bIsRunning)
		{
			return false;
		}

		PoolTask Task;
		Task.Function = [Function = std::forward<TFunc>(Function)]() { Function(); };
		Task.Name = TaskName;

		return TaskChannel.Send(std::move(Task));
	}

	/**
	 * @brief 检查是否正在运行
	 */
	bool IsRunning() const
	{
		return bIsRunning;
	}

	/**
	 * @brief 获取工作协程数量
	 */
	uint32_t GetWorkerCount() const
	{
		return WorkerCoroutines.Size();
	}

private:
	struct PoolTask
	{
		std::function<void()> Function;
		TString Name;
	};

	void WorkerLoop()
	{
		while (bIsRunning)
		{
			PoolTask Task;
			if (TaskChannel.Receive(Task))
			{
				try
				{
					NLOG_THREADING(Trace, "Executing pool task: {}", Task.Name.GetData());
					Task.Function();
				}
				catch (const std::exception& e)
				{
					NLOG_THREADING(Error, "Exception in pool task '{}': {}", Task.Name.GetData(), e.what());
				}
				catch (...)
				{
					NLOG_THREADING(Error, "Unknown exception in pool task '{}'", Task.Name.GetData());
				}
			}

			// 让出执行权
			CoroutineYield();
		}
	}

private:
	uint32_t MaxPoolSize;
	std::atomic<bool> bIsRunning;
	TArray<TSharedPtr<CCoroutine>, CMemoryManager> WorkerCoroutines;
	TCoroutineChannel<PoolTask> TaskChannel;
};

/**
 * @brief 协程工具类
 */
class CCoroutineUtils
{
public:
	// === 批量操作 ===

	/**
	 * @brief 等待所有协程完成
	 */
	static void WaitAll(const TArray<TSharedPtr<CCoroutine>, CMemoryManager>& Coroutines)
	{
		CoroutineWaitFor(
		    [&Coroutines]() {
			    for (const auto& Coroutine : Coroutines)
			    {
				    if (Coroutine.IsValid() && !Coroutine->IsCompleted())
				    {
					    return false;
				    }
			    }
			    return true;
		    },
		    TString("WaitAllCoroutines"));
	}

	/**
	 * @brief 等待任意一个协程完成
	 */
	static TSharedPtr<CCoroutine> WaitAny(const TArray<TSharedPtr<CCoroutine>, CMemoryManager>& Coroutines)
	{
		TSharedPtr<CCoroutine> CompletedCoroutine = nullptr;

		CoroutineWaitFor(
		    [&Coroutines, &CompletedCoroutine]() {
			    for (const auto& Coroutine : Coroutines)
			    {
				    if (Coroutine.IsValid() && Coroutine->IsCompleted())
				    {
					    CompletedCoroutine = Coroutine;
					    return true;
				    }
			    }
			    return false;
		    },
		    TString("WaitAnyCoroutine"));

		return CompletedCoroutine;
	}

	/**
	 * @brief 并行执行多个函数
	 */
	template <typename... TFuncs>
	static void ParallelRun(TFuncs&&... Functions)
	{
		TArray<TSharedPtr<CCoroutine>, CMemoryManager> Coroutines;
		int32_t Index = 0;

		auto CreateCoroutine = [&Coroutines, &Index](auto&& Function) {
			TString Name = TString("ParallelCoroutine") + TString::FromInt(Index++);
			auto Coroutine = StartCoroutine(std::forward<decltype(Function)>(Function), Name);
			if (Coroutine.IsValid())
			{
				Coroutines.Add(Coroutine);
			}
		};

		(CreateCoroutine(std::forward<TFuncs>(Functions)), ...);

		WaitAll(Coroutines);
	}

public:
	// === 性能测试 ===

	/**
	 * @brief 协程性能测试结果
	 */
	struct SCoroutinePerformanceTest
	{
		uint32_t CoroutineCount = 0;      // 协程数量
		uint32_t YieldsPerCoroutine = 0;  // 每协程让出次数
		CTimespan TotalExecutionTime;     // 总执行时间
		CTimespan AverageCoroutineTime;   // 平均协程时间
		double CoroutinesPerSecond = 0.0; // 每秒协程数
		double YieldsPerSecond = 0.0;     // 每秒让出次数
	};

	/**
	 * @brief 执行协程性能测试
	 */
	static SCoroutinePerformanceTest TestCoroutinePerformance(uint32_t CoroutineCount, uint32_t YieldsPerCoroutine)
	{
		SCoroutinePerformanceTest Result;
		Result.CoroutineCount = CoroutineCount;
		Result.YieldsPerCoroutine = YieldsPerCoroutine;

		CClock TestClock;
		TArray<TSharedPtr<CCoroutine>, CMemoryManager> TestCoroutines;
		TestCoroutines.Reserve(CoroutineCount);

		// 创建测试协程
		for (uint32_t i = 0; i < CoroutineCount; ++i)
		{
			TString CoroutineName = TString("TestCoroutine") + TString::FromInt(i);
			auto TestCoroutine = StartCoroutine(
			    [YieldsPerCoroutine]() {
				    for (uint32_t j = 0; j < YieldsPerCoroutine; ++j)
				    {
					    // 模拟一些工作
					    volatile int Dummy = j * 2;
					    (void)Dummy;

					    CoroutineYield();
				    }
			    },
			    CoroutineName);

			if (TestCoroutine.IsValid())
			{
				TestCoroutines.Add(TestCoroutine);
			}
		}

		// 等待所有协程完成
		WaitAll(TestCoroutines);

		Result.TotalExecutionTime = TestClock.GetElapsed();
		Result.AverageCoroutineTime = CTimespan::FromSeconds(Result.TotalExecutionTime.GetTotalSeconds() /
		                                                     CoroutineCount);
		Result.CoroutinesPerSecond = CoroutineCount / Result.TotalExecutionTime.GetTotalSeconds();
		Result.YieldsPerSecond = (CoroutineCount * YieldsPerCoroutine) / Result.TotalExecutionTime.GetTotalSeconds();

		return Result;
	}

public:
	// === 调试工具 ===

	/**
	 * @brief 生成协程系统综合报告
	 */
	static TString GenerateComprehensiveReport()
	{
		auto& Scheduler = GetCoroutineScheduler();
		auto SchedulerReport = Scheduler.GenerateReport();

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Coroutine System Comprehensive Report ===\n\n"
		         "%s\n\n"
		         "System Information:\n"
		         "  Default Stack Size: %zu bytes\n"
		         "  Min Stack Size: %zu bytes\n"
		         "  Max Stack Size: %zu bytes\n",
		         SchedulerReport.GetData(),
		         DEFAULT_COROUTINE_STACK_SIZE,
		         MIN_COROUTINE_STACK_SIZE,
		         MAX_COROUTINE_STACK_SIZE);

		return TString(Buffer);
	}
};

// === 便捷宏定义 ===

/**
 * @brief 定义协程函数的宏
 */
#define COROUTINE_FUNCTION(FunctionName) void FunctionName()

/**
 * @brief 启动协程的宏
 */
#define START_COROUTINE(Function, Name) NLib::StartCoroutine([this]() { Function(); }, Name)

/**
 * @brief 协程等待的宏
 */
#define COROUTINE_WAIT(Duration) NLib::CoroutineWait(Duration)

#define COROUTINE_WAIT_FOR(Condition) NLib::CoroutineWaitFor([&]() { return (Condition); }, #Condition)

#define COROUTINE_YIELD() NLib::CoroutineYield()

// === 类型别名 ===
using CoroutineUtils = CCoroutineUtils;
using CoroutineSemaphore = CCoroutineSemaphore;
using CoroutineMutex = CCoroutineMutex;
using CoroutineConditionVariable = CCoroutineConditionVariable;

template <typename T>
using CoroutineChannel = TCoroutineChannel<T>;

using CoroutinePool = CCoroutinePool;

} // namespace NLib