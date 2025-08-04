#pragma once

#include "Containers/TArray.h"
#include "Containers/TQueue.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "Task.h"
#include "Thread.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "ThreadPool.generate.h"

namespace NLib
{
/**
 * @brief 线程池状态枚举
 */
enum class EThreadPoolState : uint8_t
{
	Stopped,  // 已停止
	Starting, // 启动中
	Running,  // 运行中
	Stopping, // 停止中
	Paused    // 暂停中
};

/**
 * @brief 线程池统计信息
 */
struct SThreadPoolStats
{
	uint32_t WorkerThreadCount = 0;   // 工作线程数
	uint32_t ActiveThreadCount = 0;   // 活跃线程数
	uint32_t QueuedTaskCount = 0;     // 队列中的任务数
	uint32_t CompletedTaskCount = 0;  // 已完成任务数
	uint32_t FailedTaskCount = 0;     // 失败任务数
	uint32_t TotalTasksProcessed = 0; // 总处理任务数
	CTimespan TotalProcessingTime;    // 总处理时间
	CDateTime LastTaskTime;           // 最后任务时间

	void Reset()
	{
		QueuedTaskCount = 0;
		TotalProcessingTime = CTimespan::Zero();
	}
};

/**
 * @brief 线程池配置
 */
struct SThreadPoolConfig
{
	uint32_t MinThreads = 2;                                   // 最小线程数
	uint32_t MaxThreads = 8;                                   // 最大线程数
	uint32_t MaxQueueSize = 1000;                              // 最大队列大小
	CTimespan ThreadIdleTimeout = CTimespan::FromSeconds(60);  // 线程空闲超时
	EThreadPriority DefaultPriority = EThreadPriority::Normal; // 默认线程优先级
	bool bAutoScale = true;                                    // 是否自动伸缩
	bool bPrestart = false;                                    // 是否预启动线程

	// 验证配置
	bool IsValid() const
	{
		return MinThreads > 0 && MaxThreads >= MinThreads && MaxQueueSize > 0;
	}
};

/**
 * @brief 工作线程包装器
 */
class CWorkerThread : public IRunnable
{
public:
	CWorkerThread(class NThreadPool* InOwner, uint32_t InWorkerId)
	    : Owner(InOwner),
	      WorkerId(InWorkerId),
	      bShouldStop(false)
	{}

	bool Initialize() override;
	uint32_t Run() override;
	void Stop() override;
	void Cleanup() override;
	const char* GetRunnableName() const override
	{
		return "WorkerThread";
	}

	uint32_t GetWorkerId() const
	{
		return WorkerId;
	}
	bool ShouldStop() const
	{
		return bShouldStop.load();
	}

private:
	class NThreadPool* Owner;
	uint32_t WorkerId;
	std::atomic<bool> bShouldStop;
};

/**
 * @brief 线程池类
 *
 * 提供高性能的线程池功能：
 * - 动态线程管理
 * - 任务队列管理
 * - 自动负载均衡
 * - 性能统计监控
 */
NCLASS()
class NThreadPool : public NObject
{
	GENERATED_BODY()

	friend class CWorkerThread;

public:
	// === 委托定义 ===
	DECLARE_DELEGATE(FOnThreadPoolStarted);
	DECLARE_DELEGATE(FOnThreadPoolStopped);
	DECLARE_DELEGATE(FOnTaskCompleted, uint64_t);
	DECLARE_DELEGATE(FOnTaskFailed, uint64_t, const TString&);

public:
	// === 构造函数 ===

	/**
	 * @brief 构造函数
	 */
	explicit NThreadPool(const SThreadPoolConfig& InConfig = SThreadPoolConfig{})
	    : Config(InConfig),
	      State(EThreadPoolState::Stopped),
	      bIsInitialized(false),
	      NextTaskId(1)
	{
		if (!Config.IsValid())
		{
			NLOG_THREADING(Error, "Invalid thread pool configuration");
			Config = SThreadPoolConfig{}; // 使用默认配置
		}
	}

	/**
	 * @brief 析构函数
	 */
	virtual ~NThreadPool()
	{
		Shutdown();
	}

public:
	// === 生命周期管理 ===

	/**
	 * @brief 初始化线程池
	 */
	bool Initialize()
	{
		if (bIsInitialized)
		{
			NLOG_THREADING(Warning, "ThreadPool already initialized");
			return true;
		}

		if (!Config.IsValid())
		{
			NLOG_THREADING(Error, "Cannot initialize ThreadPool with invalid config");
			return false;
		}

		State = EThreadPoolState::Starting;
		Stats = SThreadPoolStats{};

		// 预分配容器
		WorkerThreads.Reserve(Config.MaxThreads);
		TaskQueue.Reserve(Config.MaxQueueSize);

		// 预启动线程
		if (Config.bPrestart)
		{
			for (uint32_t i = 0; i < Config.MinThreads; ++i)
			{
				CreateWorkerThread();
			}
		}

		State = EThreadPoolState::Running;
		bIsInitialized = true;

		NLOG_THREADING(
		    Info, "ThreadPool initialized with {} min threads, {} max threads", Config.MinThreads, Config.MaxThreads);

		OnThreadPoolStarted.ExecuteIfBound();
		return true;
	}

	/**
	 * @brief 关闭线程池
	 */
	void Shutdown()
	{
		if (!bIsInitialized || State == EThreadPoolState::Stopped)
		{
			return;
		}

		NLOG_THREADING(Info, "ThreadPool shutting down...");

		State = EThreadPoolState::Stopping;

		// 停止所有工作线程
		StopAllWorkerThreads();

		// 清空任务队列
		ClearTaskQueue();

		State = EThreadPoolState::Stopped;
		bIsInitialized = false;

		NLOG_THREADING(Info, "ThreadPool shutdown complete. Stats: {} tasks processed", Stats.TotalTasksProcessed);

		OnThreadPoolStopped.ExecuteIfBound();
	}

public:
	// === 任务提交 ===

	/**
	 * @brief 提交函数任务
	 */
	template <typename TFunc>
	auto SubmitTask(TFunc&& Function, const TString& TaskName = TString("PoolTask"))
	    -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		if (!IsRunning())
		{
			NLOG_THREADING(Error, "Cannot submit task to stopped ThreadPool");
			return TFuture<ResultType>();
		}

		auto Task = CreateTask(std::forward<TFunc>(Function), TaskName);
		auto Future = TFuture<ResultType>(Task);

		if (EnqueueTask(Task))
		{
			return Future;
		}

		return TFuture<ResultType>();
	}

	/**
	 * @brief 提交对象成员函数任务
	 */
	template <typename TObject, typename TFunc>
	auto SubmitTask(TObject* Object, TFunc&& Function, const TString& TaskName = TString("PoolTask"))
	    -> TFuture<std::invoke_result_t<TFunc, TObject*>>
	{
		using ResultType = std::invoke_result_t<TFunc, TObject*>;

		if (!Object)
		{
			NLOG_THREADING(Error, "Cannot submit task with null object");
			return TFuture<ResultType>();
		}

		if (!IsRunning())
		{
			NLOG_THREADING(Error, "Cannot submit task to stopped ThreadPool");
			return TFuture<ResultType>();
		}

		auto Task = CreateTask(
		    [Object, Function]() -> ResultType {
			    if constexpr (std::is_void_v<ResultType>)
			    {
				    (Object->*Function)();
			    }
			    else
			    {
				    return (Object->*Function)();
			    }
		    },
		    TaskName);

		auto Future = TFuture<ResultType>(Task);

		if (EnqueueTask(Task))
		{
			return Future;
		}

		return TFuture<ResultType>();
	}

	/**
	 * @brief 提交智能指针对象任务
	 */
	template <typename TObject, typename TFunc>
	auto SubmitTask(TSharedPtr<TObject> Object, TFunc&& Function, const TString& TaskName = TString("PoolTask"))
	    -> TFuture<std::invoke_result_t<TFunc, TObject*>>
	{
		using ResultType = std::invoke_result_t<TFunc, TObject*>;

		if (!Object.IsValid())
		{
			NLOG_THREADING(Error, "Cannot submit task with invalid shared pointer");
			return TFuture<ResultType>();
		}

		if (!IsRunning())
		{
			NLOG_THREADING(Error, "Cannot submit task to stopped ThreadPool");
			return TFuture<ResultType>();
		}

		auto Task = CreateTask(
		    [Object, Function]() -> ResultType {
			    if constexpr (std::is_void_v<ResultType>)
			    {
				    (Object.Get()->*Function)();
			    }
			    else
			    {
				    return (Object.Get()->*Function)();
			    }
		    },
		    TaskName);

		auto Future = TFuture<ResultType>(Task);

		if (EnqueueTask(Task))
		{
			return Future;
		}

		return TFuture<ResultType>();
	}

	/**
	 * @brief 提交已创建的任务
	 */
	template <typename TResult>
	bool SubmitTask(TSharedPtr<TTask<TResult>> Task)
	{
		if (!Task.IsValid())
		{
			NLOG_THREADING(Error, "Cannot submit invalid task");
			return false;
		}

		return EnqueueTask(Task);
	}

public:
	// === 控制操作 ===

	/**
	 * @brief 暂停线程池
	 */
	void Pause()
	{
		if (State == EThreadPoolState::Running)
		{
			State = EThreadPoolState::Paused;
			TaskCondition.notify_all();
			NLOG_THREADING(Debug, "ThreadPool paused");
		}
	}

	/**
	 * @brief 恢复线程池
	 */
	void Resume()
	{
		if (State == EThreadPoolState::Paused)
		{
			State = EThreadPoolState::Running;
			TaskCondition.notify_all();
			NLOG_THREADING(Debug, "ThreadPool resumed");
		}
	}

	/**
	 * @brief 等待所有任务完成
	 */
	void WaitForAll(const CTimespan& Timeout = CTimespan::Zero())
	{
		if (!IsRunning())
		{
			return;
		}

		std::unique_lock<std::mutex> Lock(TaskMutex);

		if (Timeout.IsZero())
		{
			TaskCompletionCondition.wait(Lock,
			                             [this]() { return TaskQueue.IsEmpty() && Stats.ActiveThreadCount == 0; });
		}
		else
		{
			auto TimeoutDuration = std::chrono::nanoseconds(Timeout.GetTicks() * 100);
			TaskCompletionCondition.wait_for(
			    Lock, TimeoutDuration, [this]() { return TaskQueue.IsEmpty() && Stats.ActiveThreadCount == 0; });
		}
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查线程池是否正在运行
	 */
	bool IsRunning() const
	{
		return State == EThreadPoolState::Running;
	}

	/**
	 * @brief 检查线程池是否暂停
	 */
	bool IsPaused() const
	{
		return State == EThreadPoolState::Paused;
	}

	/**
	 * @brief 获取线程池状态
	 */
	EThreadPoolState GetState() const
	{
		return State.load();
	}

	/**
	 * @brief 获取配置信息
	 */
	const SThreadPoolConfig& GetConfig() const
	{
		return Config;
	}

	/**
	 * @brief 获取统计信息
	 */
	const SThreadPoolStats& GetStats() const
	{
		return Stats;
	}

	/**
	 * @brief 获取工作线程数量
	 */
	uint32_t GetWorkerThreadCount() const
	{
		std::lock_guard<std::mutex> Lock(ThreadsMutex);
		return WorkerThreads.Size();
	}

	/**
	 * @brief 获取队列中的任务数量
	 */
	uint32_t GetQueuedTaskCount() const
	{
		std::lock_guard<std::mutex> Lock(TaskMutex);
		return TaskQueue.Size();
	}

public:
	// === 委托事件 ===

	FOnThreadPoolStarted OnThreadPoolStarted;
	FOnThreadPoolStopped OnThreadPoolStopped;
	FOnTaskCompleted OnTaskCompleted;
	FOnTaskFailed OnTaskFailed;

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成线程池报告
	 */
	TString GenerateReport() const
	{
		std::lock_guard<std::mutex> Lock1(ThreadsMutex);
		std::lock_guard<std::mutex> Lock2(TaskMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== ThreadPool Report ===\n"
		         "State: %s\n"
		         "Worker Threads: %u\n"
		         "Active Threads: %u\n"
		         "Queued Tasks: %u\n"
		         "Completed Tasks: %u\n"
		         "Failed Tasks: %u\n"
		         "Total Processed: %u\n"
		         "Total Processing Time: %.2f ms\n"
		         "Config - Min: %u, Max: %u, Queue Size: %u",
		         GetStateString().GetData(),
		         Stats.WorkerThreadCount,
		         Stats.ActiveThreadCount,
		         Stats.QueuedTaskCount,
		         Stats.CompletedTaskCount,
		         Stats.FailedTaskCount,
		         Stats.TotalTasksProcessed,
		         Stats.TotalProcessingTime.GetTotalMilliseconds(),
		         Config.MinThreads,
		         Config.MaxThreads,
		         Config.MaxQueueSize);

		return TString(Buffer);
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 创建任务对象
	 */
	template <typename TFunc>
	auto CreateTask(TFunc&& Function, const TString& TaskName)
	{
		using ResultType = std::invoke_result_t<TFunc>;

		uint64_t TaskId = NextTaskId.fetch_add(1);

		auto Task = MakeShared<TTask<ResultType>>(std::forward<TFunc>(Function), TaskName, ETaskPriority::Normal);

		// 绑定完成回调
		Task->OnTaskCompleted.BindLambda([this, TaskId](uint64_t CompletedTaskId, const auto& Result) {
			OnTaskCompleted.ExecuteIfBound(CompletedTaskId);
			Stats.CompletedTaskCount++;
			TaskCompletionCondition.notify_all();
		});

		// 绑定失败回调
		Task->OnTaskFailed.BindLambda([this, TaskId](uint64_t FailedTaskId, const TString& Error) {
			OnTaskFailed.ExecuteIfBound(FailedTaskId, Error);
			Stats.FailedTaskCount++;
			TaskCompletionCondition.notify_all();
		});

		return Task;
	}

	/**
	 * @brief 入队任务
	 */
	template <typename TResult>
	bool EnqueueTask(TSharedPtr<TTask<TResult>> Task)
	{
		std::lock_guard<std::mutex> Lock(TaskMutex);

		if (TaskQueue.Size() >= Config.MaxQueueSize)
		{
			NLOG_THREADING(Warning, "ThreadPool task queue is full, dropping task");
			return false;
		}

		// 类型擦除存储
		TaskQueue.Enqueue(TSharedPtr<ITaskBase>(Task));
		Stats.QueuedTaskCount++;

		NLOG_THREADING(Trace, "Task enqueued, queue size: {}", TaskQueue.Size());

		// 检查是否需要创建新线程
		if (Config.bAutoScale && ShouldCreateNewThread())
		{
			CreateWorkerThread();
		}

		// 唤醒等待的工作线程
		TaskCondition.notify_one();

		return true;
	}

	/**
	 * @brief 出队任务
	 */
	TSharedPtr<ITaskBase> DequeueTask()
	{
		std::unique_lock<std::mutex> Lock(TaskMutex);

		// 等待任务或停止信号
		TaskCondition.wait(Lock, [this]() { return !TaskQueue.IsEmpty() || State != EThreadPoolState::Running; });

		if (State != EThreadPoolState::Running)
		{
			return nullptr;
		}

		if (TaskQueue.IsEmpty())
		{
			return nullptr;
		}

		auto Task = TaskQueue.Dequeue();
		Stats.QueuedTaskCount--;

		return Task;
	}

	/**
	 * @brief 创建工作线程
	 */
	bool CreateWorkerThread()
	{
		std::lock_guard<std::mutex> Lock(ThreadsMutex);

		if (WorkerThreads.Size() >= Config.MaxThreads)
		{
			return false;
		}

		uint32_t WorkerId = WorkerThreads.Size();
		auto Worker = MakeShared<CWorkerThread>(this, WorkerId);
		auto Thread = MakeShared<NThread>("ThreadPoolWorker");

		if (!Thread->Start(Worker, Config.DefaultPriority))
		{
			NLOG_THREADING(Error, "Failed to start worker thread {}", WorkerId);
			return false;
		}

		WorkerThreads.Add(Thread);
		Stats.WorkerThreadCount++;

		NLOG_THREADING(Debug, "Created worker thread {}, total threads: {}", WorkerId, WorkerThreads.Size());

		return true;
	}

	/**
	 * @brief 停止所有工作线程
	 */
	void StopAllWorkerThreads()
	{
		{
			std::lock_guard<std::mutex> Lock(ThreadsMutex);

			// 通知所有线程停止
			for (auto& Thread : WorkerThreads)
			{
				if (Thread.IsValid())
				{
					Thread->Stop();
				}
			}
		}

		// 唤醒所有等待的线程
		TaskCondition.notify_all();

		// 等待所有线程结束
		{
			std::lock_guard<std::mutex> Lock(ThreadsMutex);

			for (auto& Thread : WorkerThreads)
			{
				if (Thread.IsValid())
				{
					Thread->Join();
				}
			}

			WorkerThreads.Empty();
			Stats.WorkerThreadCount = 0;
			Stats.ActiveThreadCount = 0;
		}
	}

	/**
	 * @brief 清空任务队列
	 */
	void ClearTaskQueue()
	{
		std::lock_guard<std::mutex> Lock(TaskMutex);

		int32_t Count = TaskQueue.Size();
		TaskQueue.Empty();
		Stats.QueuedTaskCount = 0;

		if (Count > 0)
		{
			NLOG_THREADING(Warning, "Cleared {} pending tasks from queue", Count);
		}
	}

	/**
	 * @brief 检查是否应该创建新线程
	 */
	bool ShouldCreateNewThread() const
	{
		return WorkerThreads.Size() < Config.MaxThreads && TaskQueue.Size() > Stats.ActiveThreadCount &&
		       Stats.ActiveThreadCount == WorkerThreads.Size();
	}

	/**
	 * @brief 获取状态字符串
	 */
	TString GetStateString() const
	{
		switch (State.load())
		{
		case EThreadPoolState::Stopped:
			return TString("Stopped");
		case EThreadPoolState::Starting:
			return TString("Starting");
		case EThreadPoolState::Running:
			return TString("Running");
		case EThreadPoolState::Stopping:
			return TString("Stopping");
		case EThreadPoolState::Paused:
			return TString("Paused");
		default:
			return TString("Unknown");
		}
	}

	/**
	 * @brief 工作线程执行任务
	 */
	void ExecuteTask(TSharedPtr<ITaskBase> Task)
	{
		if (!Task.IsValid())
		{
			return;
		}

		CClock ExecutionClock;
		Stats.ActiveThreadCount++;

		try
		{
			Task->Execute();
			Stats.TotalTasksProcessed++;
		}
		catch (...)
		{
			NLOG_THREADING(Error, "Exception in task execution");
			Stats.FailedTaskCount++;
		}

		Stats.ActiveThreadCount--;
		Stats.TotalProcessingTime += ExecutionClock.GetElapsed();
		Stats.LastTaskTime = CDateTime::Now();
	}

private:
	// === 成员变量 ===

	SThreadPoolConfig Config;            // 线程池配置
	std::atomic<EThreadPoolState> State; // 线程池状态
	std::atomic<bool> bIsInitialized;    // 是否已初始化
	std::atomic<uint64_t> NextTaskId;    // 下一个任务ID

	// 线程管理
	TArray<TSharedPtr<NThread>, CMemoryManager> WorkerThreads; // 工作线程数组
	mutable std::mutex ThreadsMutex;                           // 线程互斥锁

	// 任务队列
	TQueue<TSharedPtr<ITaskBase>, CMemoryManager> TaskQueue; // 任务队列
	mutable std::mutex TaskMutex;                            // 任务互斥锁
	std::condition_variable TaskCondition;                   // 任务条件变量
	std::condition_variable TaskCompletionCondition;         // 任务完成条件变量

	// 统计信息
	SThreadPoolStats Stats; // 统计信息
};

// === 工作线程实现 ===

inline bool CWorkerThread::Initialize()
{
	NLOG_THREADING(Trace, "Worker thread {} initializing", WorkerId);
	return true;
}

inline uint32_t CWorkerThread::Run()
{
	NLOG_THREADING(Trace, "Worker thread {} started", WorkerId);

	while (!ShouldStop() && Owner->IsRunning())
	{
		auto Task = Owner->DequeueTask();
		if (Task.IsValid())
		{
			Owner->ExecuteTask(Task);
		}

		// 避免忙等待
		if (!Task.IsValid())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	NLOG_THREADING(Trace, "Worker thread {} finished", WorkerId);
	return 0;
}

inline void CWorkerThread::Stop()
{
	bShouldStop = true;
}

inline void CWorkerThread::Cleanup()
{
	NLOG_THREADING(Trace, "Worker thread {} cleanup", WorkerId);
}

// === 类型别名 ===
using FThreadPool = NThreadPool;

// === 便捷函数 ===

/**
 * @brief 创建线程池
 */
inline TSharedPtr<NThreadPool> CreateThreadPool(const SThreadPoolConfig& Config = SThreadPoolConfig{})
{
	auto Pool = MakeShared<NThreadPool>(Config);
	if (Pool->Initialize())
	{
		return Pool;
	}
	return nullptr;
}

} // namespace NLib