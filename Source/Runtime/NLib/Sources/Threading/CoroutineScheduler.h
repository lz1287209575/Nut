#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Containers/TQueue.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Coroutine.h"
#include "Events/Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <mutex>
#include <setjmp.h>

#include "CoroutineScheduler.generate.h"

namespace NLib
{
/**
 * @brief 调度器策略枚举
 */
enum class ECoroutineSchedulingPolicy : uint8_t
{
	RoundRobin, // 轮询调度
	Priority,   // 优先级调度
	Fair,       // 公平调度
	Custom      // 自定义调度
};

/**
 * @brief 协程优先级
 */
enum class ECoroutinePriority : uint8_t
{
	Lowest = 0,
	Low = 1,
	Normal = 2,
	High = 3,
	Highest = 4,
	Critical = 5
};

/**
 * @brief 调度器配置
 */
struct SCoroutineSchedulerConfig
{
	ECoroutineSchedulingPolicy Policy = ECoroutineSchedulingPolicy::RoundRobin;
	uint32_t MaxCoroutines = 1000;                         // 最大协程数
	uint32_t TimeSliceMs = 10;                             // 时间片长度（毫秒）
	bool bEnablePreemption = false;                        // 是否启用抢占式调度
	bool bAutoCleanup = true;                              // 是否自动清理已完成的协程
	CTimespan CleanupInterval = CTimespan::FromSeconds(5); // 清理间隔

	bool IsValid() const
	{
		return MaxCoroutines > 0 && TimeSliceMs > 0;
	}
};

/**
 * @brief 调度器统计信息
 */
struct SCoroutineSchedulerStats
{
	uint32_t TotalCoroutines = 0;       // 总协程数
	uint32_t ActiveCoroutines = 0;      // 活跃协程数
	uint32_t SuspendedCoroutines = 0;   // 挂起协程数
	uint32_t CompletedCoroutines = 0;   // 已完成协程数
	uint32_t FailedCoroutines = 0;      // 失败协程数
	uint32_t TotalSchedulingCycles = 0; // 总调度周期数
	CTimespan TotalSchedulingTime;      // 总调度时间
	CTimespan AverageSchedulingTime;    // 平均调度时间
	CDateTime LastSchedulingTime;       // 最后调度时间

	void Reset()
	{
		TotalSchedulingCycles = 0;
		TotalSchedulingTime = CTimespan::Zero;
		AverageSchedulingTime = CTimespan::Zero;
	}

	void UpdateSchedulingTime(const CTimespan& SchedulingTime)
	{
		TotalSchedulingTime += SchedulingTime;
		TotalSchedulingCycles++;

		if (TotalSchedulingCycles > 0)
		{
			AverageSchedulingTime = CTimespan::FromSeconds(TotalSchedulingTime.GetTotalSeconds() /
			                                               TotalSchedulingCycles);
		}
	}
};

/**
 * @brief 协程调度项
 */
struct SCoroutineScheduleItem
{
	TSharedPtr<NCoroutine> Coroutine;                         // 协程对象
	ECoroutinePriority Priority = ECoroutinePriority::Normal; // 优先级
	CDateTime LastRunTime;                                    // 最后运行时间
	CTimespan TotalRunTime;                                   // 总运行时间
	uint32_t RunCount = 0;                                    // 运行次数
	bool bIsScheduled = false;                                // 是否已调度

	SCoroutineScheduleItem() = default;

	SCoroutineScheduleItem(TSharedPtr<NCoroutine> InCoroutine,
	                       ECoroutinePriority InPriority = ECoroutinePriority::Normal)
	    : Coroutine(InCoroutine),
	      Priority(InPriority),
	      LastRunTime(CDateTime::Now())
	{}

	bool IsValid() const
	{
		return Coroutine->IsValid() && !Coroutine->IsCompleted();
	}

	bool CanRun() const
	{
		return IsValid() && Coroutine->CanResume();
	}
};

/**
 * @brief 协程调度器
 *
 * 提供协程的调度和管理功能：
 * - 多种调度策略
 * - 协程生命周期管理
 * - 性能监控和统计
 * - 自动清理机制
 */
NCLASS()
class NCoroutineScheduler : public NObject
{
	GENERATED_BODY()

	friend class NCoroutine;

public:
	// === 委托定义 ===
	DECLARE_DELEGATE(FOnSchedulerStarted);
	DECLARE_DELEGATE(FOnSchedulerStopped);
	DECLARE_DELEGATE(FOnCoroutineScheduled, FCoroutineId);
	DECLARE_DELEGATE(FOnSchedulingCycleCompleted, uint32_t);

	using CustomSchedulerFunc =
	    std::function<TSharedPtr<NCoroutine>(const TArray<SCoroutineScheduleItem, CMemoryManager>&)>;

public:
	// === 单例模式 ===

	static NCoroutineScheduler& GetInstance()
	{
		static NCoroutineScheduler Instance;
		return Instance;
	}

private:
	// === 构造函数（私有） ===

	NCoroutineScheduler()
	    : Config(),
	      bIsRunning(false),
	      bIsInitialized(false),
	      CurrentCoroutine(nullptr),
	      MainCoroutine(nullptr),
	      CustomScheduler(nullptr)
	{}

public:
	// 禁用拷贝
	NCoroutineScheduler(const NCoroutineScheduler&) = delete;
	NCoroutineScheduler& operator=(const NCoroutineScheduler&) = delete;

	~NCoroutineScheduler()
	{
		Shutdown();
	}

public:
	// === 初始化和关闭 ===

	/**
	 * @brief 初始化调度器
	 */
	bool Initialize(const SCoroutineSchedulerConfig& InConfig = SCoroutineSchedulerConfig{})
	{
		if (bIsInitialized)
		{
			NLOG_THREADING(Warning, "CoroutineScheduler already initialized");
			return true;
		}

		if (!InConfig.IsValid())
		{
			NLOG_THREADING(Error, "Invalid coroutine scheduler configuration");
			return false;
		}

		Config = InConfig;
		Stats = SCoroutineSchedulerStats{};

		// 创建主协程
		MainCoroutine = MakeShared<NCoroutine>(CString("MainCoroutine"));
		CurrentCoroutine = MainCoroutine;

		// 预分配容器
		ScheduleItems.Reserve(Config.MaxCoroutines);
		ReadyQueue.Reserve(Config.MaxCoroutines);

		// 设置调度器上下文
		if (setjmp(SchedulerContext) == 0)
		{
			bIsInitialized = true;
			bIsRunning = true;

			NLOG_THREADING(Info,
			               "CoroutineScheduler initialized with policy: {}, max coroutines: {}",
			               static_cast<int>(Config.Policy),
			               Config.MaxCoroutines);

			OnSchedulerStarted.ExecuteIfBound();
			return true;
		}

		return false;
	}

	/**
	 * @brief 关闭调度器
	 */
	void Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}

		bIsRunning = false;

		// 取消所有协程
		CancelAllCoroutines();

		// 清理资源
		ClearAllCoroutines();

		NLOG_THREADING(Info,
		               "CoroutineScheduler shutdown. Stats: {} total coroutines, {} completed",
		               Stats.TotalCoroutines,
		               Stats.CompletedCoroutines);

		OnSchedulerStopped.ExecuteIfBound();

		bIsInitialized = false;
	}

public:
	// === 协程管理 ===

	/**
	 * @brief 创建并启动协程
	 */
	template <typename TFunc>
	TSharedPtr<NCoroutine> StartCoroutine(TFunc&& Function,
	                                      const CString& Name = CString("Coroutine"),
	                                      ECoroutinePriority Priority = ECoroutinePriority::Normal,
	                                      size_t StackSize = DEFAULT_COROUTINE_STACK_SIZE)
	{
		if (!bIsInitialized || !bIsRunning)
		{
			NLOG_THREADING(Error, "Cannot start coroutine when scheduler is not running");
			return nullptr;
		}

		if (ScheduleItems.Size() >= Config.MaxCoroutines)
		{
			NLOG_THREADING(Error, "Cannot start coroutine: maximum coroutines reached ({})", Config.MaxCoroutines);
			return nullptr;
		}

		// 创建协程
		auto Coroutine = MakeShared<NCoroutine>(
		    [Function = std::forward<TFunc>(Function)]() { Function(); }, Name, StackSize);

		if (!Coroutine->Initialize())
		{
			NLOG_THREADING(Error, "Failed to initialize coroutine '{}'", Name.GetData());
			return nullptr;
		}

		// 添加到调度列表
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		SCoroutineScheduleItem Item(Coroutine, Priority);
		ScheduleItems.Add(Item);

		// 添加到就绪队列
		if (Coroutine->GetState() == ECoroutineState::Ready)
		{
			ReadyQueue.Enqueue(Coroutine);
		}

		Stats.TotalCoroutines++;
		Stats.ActiveCoroutines++;

		NLOG_THREADING(Debug,
		               "Coroutine '{}' (ID: {}) started with priority: {}",
		               Name.GetData(),
		               Coroutine->GetCoroutineId(),
		               static_cast<int>(Priority));

		return Coroutine;
	}

	/**
	 * @brief 停止协程
	 */
	bool StopCoroutine(FCoroutineId CoroutineId)
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		for (auto& Item : ScheduleItems)
		{
			if (Item.Coroutine.IsValid() && Item.Coroutine->GetCoroutineId() == CoroutineId)
			{
				Item.Coroutine->Cancel();
				Stats.ActiveCoroutines--;

				NLOG_THREADING(Debug, "Coroutine ID: {} stopped", CoroutineId);
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief 停止协程
	 */
	bool StopCoroutine(TSharedPtr<NCoroutine> Coroutine)
	{
		if (!Coroutine.IsValid())
		{
			return false;
		}

		return StopCoroutine(Coroutine->GetCoroutineId());
	}

	/**
	 * @brief 取消所有协程
	 */
	void CancelAllCoroutines()
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		for (auto& Item : ScheduleItems)
		{
			if (Item.Coroutine.IsValid() && !Item.Coroutine->IsCompleted())
			{
				Item.Coroutine->Cancel();
			}
		}

		Stats.ActiveCoroutines = 0;

		NLOG_THREADING(Info, "All coroutines cancelled");
	}

public:
	// === 调度执行 ===

	/**
	 * @brief 执行一轮调度
	 */
	void Tick(float DeltaTime = 0.0f)
	{
		if (!bIsInitialized || !bIsRunning)
		{
			return;
		}

		CClock SchedulingClock;

		// 清理已完成的协程
		if (Config.bAutoCleanup)
		{
			CleanupCompletedCoroutines();
		}

		// 更新就绪队列
		UpdateReadyQueue();

		// 选择下一个要执行的协程
		auto NextCoroutine = SelectNextCoroutine();
		if (!NextCoroutine.IsValid())
		{
			return; // 没有可执行的协程
		}

		// 切换到选中的协程
		SwitchToCoroutine(NextCoroutine);

		// 更新统计信息
		Stats.UpdateSchedulingTime(SchedulingClock.GetElapsed());
		Stats.LastSchedulingTime = CDateTime::Now();

		OnSchedulingCycleCompleted.ExecuteIfBound(Stats.TotalSchedulingCycles);
	}

	/**
	 * @brief 运行调度器直到所有协程完成
	 */
	void RunUntilComplete(float TickInterval = 0.016f) // 默认60FPS
	{
		while (bIsRunning && HasActiveCoroutines())
		{
			Tick(TickInterval);

			// 模拟帧间隔
			if (TickInterval > 0.0f)
			{
				CTimespan SleepTime = CTimespan::FromSeconds(static_cast<double>(TickInterval));
				auto SleepDuration = std::chrono::nanoseconds(SleepTime.GetTicks() * 100);
				std::this_thread::sleep_for(SleepDuration);
			}
		}
	}

	/**
	 * @brief 让出当前协程的执行权
	 */
	void YieldCurrentCoroutine()
	{
		if (CurrentCoroutine.IsValid() && CurrentCoroutine != MainCoroutine)
		{
			CurrentCoroutine->Yield();
		}
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查调度器是否正在运行
	 */
	bool IsRunning() const
	{
		return bIsRunning;
	}

	/**
	 * @brief 检查是否有活跃的协程
	 */
	bool HasActiveCoroutines() const
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);
		return Stats.ActiveCoroutines > 0;
	}

	/**
	 * @brief 获取当前运行的协程
	 */
	TSharedPtr<NCoroutine> GetCurrentCoroutine() const
	{
		return CurrentCoroutine;
	}

	/**
	 * @brief 获取主协程
	 */
	TSharedPtr<NCoroutine> GetMainCoroutine() const
	{
		return MainCoroutine;
	}

	/**
	 * @brief 获取配置信息
	 */
	const SCoroutineSchedulerConfig& GetConfig() const
	{
		return Config;
	}

	/**
	 * @brief 获取统计信息
	 */
	const SCoroutineSchedulerStats& GetStats() const
	{
		return Stats;
	}

	/**
	 * @brief 查找协程
	 */
	TSharedPtr<NCoroutine> FindCoroutine(FCoroutineId CoroutineId) const
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		for (const auto& Item : ScheduleItems)
		{
			if (Item.Coroutine.IsValid() && Item.Coroutine->GetCoroutineId() == CoroutineId)
			{
				return Item.Coroutine;
			}
		}

		return nullptr;
	}

public:
	// === 调度策略配置 ===

	/**
	 * @brief 设置自定义调度器
	 */
	void SetCustomScheduler(CustomSchedulerFunc Scheduler)
	{
		if (Config.Policy == ECoroutineSchedulingPolicy::Custom)
		{
			CustomScheduler = std::move(Scheduler);
			NLOG_THREADING(Debug, "Custom scheduler set");
		}
	}

	/**
	 * @brief 设置协程优先级
	 */
	bool SetCoroutinePriority(FCoroutineId CoroutineId, ECoroutinePriority Priority)
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		for (auto& Item : ScheduleItems)
		{
			if (Item.Coroutine.IsValid() && Item.Coroutine->GetCoroutineId() == CoroutineId)
			{
				Item.Priority = Priority;
				NLOG_THREADING(Debug, "Coroutine ID: {} priority set to: {}", CoroutineId, static_cast<int>(Priority));
				return true;
			}
		}

		return false;
	}

public:
	// === 委托事件 ===

	FOnSchedulerStarted OnSchedulerStarted;
	FOnSchedulerStopped OnSchedulerStopped;
	FOnCoroutineScheduled OnCoroutineScheduled;
	FOnSchedulingCycleCompleted OnSchedulingCycleCompleted;

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成调度器报告
	 */
	CString GenerateReport() const
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Coroutine Scheduler Report ===\n"
		         "State: %s\n"
		         "Policy: %s\n"
		         "Total Coroutines: %u\n"
		         "Active Coroutines: %u\n"
		         "Suspended Coroutines: %u\n"
		         "Completed Coroutines: %u\n"
		         "Failed Coroutines: %u\n"
		         "Scheduling Cycles: %u\n"
		         "Average Scheduling Time: %.3f ms\n"
		         "Total Scheduling Time: %.3f ms",
		         bIsRunning ? "Running" : "Stopped",
		         GetPolicyString().GetData(),
		         Stats.TotalCoroutines,
		         Stats.ActiveCoroutines,
		         Stats.SuspendedCoroutines,
		         Stats.CompletedCoroutines,
		         Stats.FailedCoroutines,
		         Stats.TotalSchedulingCycles,
		         Stats.AverageSchedulingTime.GetTotalMilliseconds(),
		         Stats.TotalSchedulingTime.GetTotalMilliseconds());

		return CString(Buffer);
	}

	/**
	 * @brief 获取所有协程的调试信息
	 */
	TArray<CString, CMemoryManager> GetCoroutineDebugInfo() const
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		TArray<CString, CMemoryManager> DebugInfo;
		DebugInfo.Reserve(ScheduleItems.Size());

		for (const auto& Item : ScheduleItems)
		{
			if (Item.Coroutine.IsValid())
			{
				char Buffer[256];
				snprintf(Buffer,
				         sizeof(Buffer),
				         "ID: %llu, Name: '%s', State: %s, Priority: %d, Runs: %u",
				         Item.Coroutine->GetCoroutineId(),
				         Item.Coroutine->GetName().GetData(),
				         GetCoroutineStateString(Item.Coroutine->GetState()).GetData(),
				         static_cast<int>(Item.Priority),
				         Item.RunCount);

				DebugInfo.Add(CString(Buffer));
			}
		}

		return DebugInfo;
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 更新就绪队列
	 */
	void UpdateReadyQueue()
	{
		for (auto& Item : ScheduleItems)
		{
			if (Item.CanRun() && !Item.bIsScheduled)
			{
				ReadyQueue.Enqueue(Item.Coroutine);
				Item.bIsScheduled = true;
			}
		}
	}

	/**
	 * @brief 选择下一个要执行的协程
	 */
	TSharedPtr<NCoroutine> SelectNextCoroutine()
	{
		switch (Config.Policy)
		{
		case ECoroutineSchedulingPolicy::RoundRobin:
			return SelectRoundRobin();

		case ECoroutineSchedulingPolicy::Priority:
			return SelectByPriority();

		case ECoroutineSchedulingPolicy::Fair:
			return SelectFair();

		case ECoroutineSchedulingPolicy::Custom:
			return SelectCustom();

		default:
			return SelectRoundRobin();
		}
	}

	/**
	 * @brief 轮询调度
	 */
	TSharedPtr<NCoroutine> SelectRoundRobin()
	{
		TSharedPtr<NCoroutine> Selected;

		if (ReadyQueue.TryDequeue(Selected))
		{
			// 标记为未调度，以便下次可以重新加入队列
			for (auto& Item : ScheduleItems)
			{
				if (Item.Coroutine == Selected)
				{
					Item.bIsScheduled = false;
					break;
				}
			}

			return Selected;
		}

		return nullptr;
	}

	/**
	 * @brief 优先级调度
	 */
	TSharedPtr<NCoroutine> SelectByPriority()
	{
		TSharedPtr<NCoroutine> HighestPriorityCoroutine = nullptr;
		ECoroutinePriority HighestPriority = ECoroutinePriority::Lowest;

		for (auto& Item : ScheduleItems)
		{
			if (Item.CanRun() && Item.Priority >= HighestPriority)
			{
				HighestPriority = Item.Priority;
				HighestPriorityCoroutine = Item.Coroutine;
			}
		}

		return HighestPriorityCoroutine;
	}

	/**
	 * @brief 公平调度
	 */
	TSharedPtr<NCoroutine> SelectFair()
	{
		TSharedPtr<NCoroutine> LeastRunCoroutine = nullptr;
		uint32_t LeastRunCount = UINT32_MAX;

		for (auto& Item : ScheduleItems)
		{
			if (Item.CanRun() && Item.RunCount < LeastRunCount)
			{
				LeastRunCount = Item.RunCount;
				LeastRunCoroutine = Item.Coroutine;
			}
		}

		return LeastRunCoroutine;
	}

	/**
	 * @brief 自定义调度
	 */
	TSharedPtr<NCoroutine> SelectCustom()
	{
		if (CustomScheduler)
		{
			return CustomScheduler(ScheduleItems);
		}

		return SelectRoundRobin(); // 回退到轮询调度
	}

	/**
	 * @brief 切换到指定协程
	 */
	void SwitchToCoroutine(TSharedPtr<NCoroutine> Coroutine)
	{
		if (!Coroutine.IsValid() || Coroutine == CurrentCoroutine)
		{
			return;
		}

		TSharedPtr<NCoroutine> PreviousCoroutine = CurrentCoroutine;
		CurrentCoroutine = Coroutine;

		// 更新调度项统计
		for (auto& Item : ScheduleItems)
		{
			if (Item.Coroutine == Coroutine)
			{
				Item.LastRunTime = CDateTime::Now();
				Item.RunCount++;
				break;
			}
		}

		OnCoroutineScheduled.ExecuteIfBound(Coroutine->GetCoroutineId());

		NLOG_THREADING(
		    Trace, "Switching to coroutine '{}' (ID: {})", Coroutine->GetName().GetData(), Coroutine->GetCoroutineId());

		// 启动或恢复协程
		if (Coroutine->GetState() == ECoroutineState::Ready)
		{
			if (Coroutine->Start())
			{
				// 执行协程函数
				Coroutine->Execute();
			}
		}
		else if (Coroutine->GetState() == ECoroutineState::Suspended)
		{
			Coroutine->Resume();
		}

		// 协程执行完毕后返回调度器
		CurrentCoroutine = PreviousCoroutine;
	}

	/**
	 * @brief 清理已完成的协程
	 */
	void CleanupCompletedCoroutines()
	{
		static CDateTime LastCleanupTime = CDateTime::Now();
		CDateTime Now = CDateTime::Now();

		if ((Now - LastCleanupTime) < Config.CleanupInterval)
		{
			return;
		}

		int32_t CleanedCount = 0;

		for (int32_t i = ScheduleItems.Size() - 1; i >= 0; --i)
		{
			if (ScheduleItems[i].Coroutine.IsValid() && ScheduleItems[i].Coroutine->IsCompleted())
			{
				ECoroutineState State = ScheduleItems[i].Coroutine->GetState();

				if (State == ECoroutineState::Completed)
				{
					Stats.CompletedCoroutines++;
				}
				else if (State == ECoroutineState::Failed)
				{
					Stats.FailedCoroutines++;
				}

				ScheduleItems.RemoveAt(i);
				CleanedCount++;

				if (Stats.ActiveCoroutines > 0)
				{
					Stats.ActiveCoroutines--;
				}
			}
		}

		if (CleanedCount > 0)
		{
			NLOG_THREADING(Debug, "Cleaned up {} completed coroutines", CleanedCount);
		}

		LastCleanupTime = Now;
	}

	/**
	 * @brief 清理所有协程
	 */
	void ClearAllCoroutines()
	{
		std::lock_guard<std::mutex> Lock(ScheduleMutex);

		ScheduleItems.Empty();
		ReadyQueue.Empty();

		Stats.ActiveCoroutines = 0;
		Stats.SuspendedCoroutines = 0;
	}

	/**
	 * @brief 获取策略字符串
	 */
	CString GetPolicyString() const
	{
		switch (Config.Policy)
		{
		case ECoroutineSchedulingPolicy::RoundRobin:
			return CString("RoundRobin");
		case ECoroutineSchedulingPolicy::Priority:
			return CString("Priority");
		case ECoroutineSchedulingPolicy::Fair:
			return CString("Fair");
		case ECoroutineSchedulingPolicy::Custom:
			return CString("Custom");
		default:
			return CString("Unknown");
		}
	}

	/**
	 * @brief 获取协程状态字符串
	 */
	static CString GetCoroutineStateString(ECoroutineState State)
	{
		switch (State)
		{
		case ECoroutineState::Created:
			return CString("Created");
		case ECoroutineState::Ready:
			return CString("Ready");
		case ECoroutineState::Running:
			return CString("Running");
		case ECoroutineState::Suspended:
			return CString("Suspended");
		case ECoroutineState::Completed:
			return CString("Completed");
		case ECoroutineState::Failed:
			return CString("Failed");
		case ECoroutineState::Cancelled:
			return CString("Cancelled");
		default:
			return CString("Unknown");
		}
	}

private:
	// === 成员变量 ===

	SCoroutineSchedulerConfig Config; // 调度器配置
	std::atomic<bool> bIsRunning;     // 是否正在运行
	std::atomic<bool> bIsInitialized; // 是否已初始化

	TSharedPtr<NCoroutine> CurrentCoroutine; // 当前运行的协程
	TSharedPtr<NCoroutine> MainCoroutine;    // 主协程

	// 调度数据
	TArray<SCoroutineScheduleItem, CMemoryManager> ScheduleItems; // 调度项列表
	TQueue<TSharedPtr<NCoroutine>, CMemoryManager> ReadyQueue;    // 就绪队列
	mutable std::mutex ScheduleMutex;                             // 调度互斥锁

	// 统计信息
	SCoroutineSchedulerStats Stats; // 统计信息

	// 调度器上下文
	jmp_buf SchedulerContext; // 调度器上下文

	// 自定义调度器
	CustomSchedulerFunc CustomScheduler; // 自定义调度函数
};

// === NCoroutine中调度器上下文的实现 ===

inline jmp_buf& NCoroutine::GetSchedulerContext()
{
	return NCoroutineScheduler::GetInstance().SchedulerContext;
}

// === 全局访问函数 ===

/**
 * @brief 获取协程调度器
 */
inline NCoroutineScheduler& GetCoroutineScheduler()
{
	return NCoroutineScheduler::GetInstance();
}

/**
 * @brief 启动协程的便捷函数
 */
template <typename TFunc>
TSharedPtr<NCoroutine> StartCoroutine(TFunc&& Function,
                                      const CString& Name = CString("Coroutine"),
                                      ECoroutinePriority Priority = ECoroutinePriority::Normal,
                                      size_t StackSize = DEFAULT_COROUTINE_STACK_SIZE)
{
	return GetCoroutineScheduler().StartCoroutine(std::forward<TFunc>(Function), Name, Priority, StackSize);
}

/**
 * @brief 让出当前协程执行权的便捷函数
 */
inline void CoroutineYield()
{
	GetCoroutineScheduler().YieldCurrentCoroutine();
}

/**
 * @brief 等待指定时间的便捷函数
 */
inline void CoroutineWait(const CTimespan& Duration)
{
	auto CurrentCoroutine = GetCoroutineScheduler().GetCurrentCoroutine();
	if (CurrentCoroutine.IsValid())
	{
		CurrentCoroutine->WaitFor(Duration);
	}
}

/**
 * @brief 等待条件满足的便捷函数
 */
template <typename TFunc>
void CoroutineWaitFor(TFunc&& Condition, const CString& Description = CString("Condition"))
{
	auto CurrentCoroutine = GetCoroutineScheduler().GetCurrentCoroutine();
	if (CurrentCoroutine.IsValid())
	{
		auto WaitCondition = CreateConditionWait(std::forward<TFunc>(Condition), Description);
		CurrentCoroutine->WaitForCondition(WaitCondition);
	}
}

// === 类型别名 ===
using FCoroutineScheduler = NCoroutineScheduler;

} // namespace NLib
