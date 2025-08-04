#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/SmartPointers.h"
#include "Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/GameTime.h"
#include "Time/Timer.h"

#include <atomic>
#include <mutex>

namespace NLib
{
/**
 * @brief 定时器句柄类型
 */
using FTimerHandle = uint64_t;
constexpr FTimerHandle INVALID_TIMER_HANDLE = 0;

/**
 * @brief 定时器状态枚举
 */
enum class ETimerManagerState : uint8_t
{
	Inactive, // 未激活
	Active,   // 激活状态
	Paused    // 暂停状态
};

/**
 * @brief 管理定时器信息
 */
struct SManagedTimer
{
	TSharedPtr<NTimer> Timer;                   // 定时器对象
	FTimerHandle Handle = INVALID_TIMER_HANDLE; // 定时器句柄
	CString DebugName;                          // 调试名称
	bool bIsPendingKill = false;                // 是否待删除
	CDateTime CreationTime;                     // 创建时间

	SManagedTimer() = default;

	SManagedTimer(TSharedPtr<NTimer> InTimer, FTimerHandle InHandle, const char* InDebugName = "")
	    : Timer(InTimer),
	      Handle(InHandle),
	      DebugName(InDebugName ? InDebugName : ""),
	      CreationTime(CDateTime::Now())
	{}

	bool IsValid() const
	{
		return Timer.IsValid() && Handle != INVALID_TIMER_HANDLE && !bIsPendingKill;
	}

	void MarkForDeletion()
	{
		bIsPendingKill = true;
		if (Timer.IsValid())
		{
			Timer->Stop();
		}
	}
};

/**
 * @brief 定时器统计信息
 */
struct STimerManagerStats
{
	uint32_t ActiveTimers = 0;            // 活跃定时器数
	uint32_t PausedTimers = 0;            // 暂停定时器数
	uint32_t TotalTimersCreated = 0;      // 总创建定时器数
	uint32_t TotalTimersDestroyed = 0;    // 总销毁定时器数
	uint32_t TimersExecutedThisFrame = 0; // 本帧执行的定时器数
	CTimespan TotalUpdateTime;            // 总更新时间
	CDateTime LastUpdateTime;             // 最后更新时间

	void Reset()
	{
		TimersExecutedThisFrame = 0;
		TotalUpdateTime = CTimespan::Zero;
	}
};

/**
 * @brief 定时器管理器
 *
 * 提供统一的定时器管理功能：
 * - 定时器创建和销毁
 * - 定时器暂停和恢复
 * - 自动垃圾回收
 * - 性能统计和监控
 */
class CTimerManager
{
public:
	// === 单例模式 ===

	static CTimerManager& GetInstance()
	{
		static CTimerManager Instance;
		return Instance;
	}

private:
	// === 构造函数（私有） ===

	CTimerManager()
	    : State(ETimerManagerState::Inactive),
	      bIsInitialized(false),
	      NextTimerHandle(1)
	{}

public:
	// 禁用拷贝
	CTimerManager(const CTimerManager&) = delete;
	CTimerManager& operator=(const CTimerManager&) = delete;

	~CTimerManager()
	{
		Shutdown();
	}

public:
	// === 初始化和关闭 ===

	/**
	 * @brief 初始化定时器管理器
	 */
	void Initialize()
	{
		if (bIsInitialized)
		{
			NLOG_EVENTS(Warning, "TimerManager already initialized");
			return;
		}

		State = ETimerManagerState::Active;
		Stats = STimerManagerStats{};

		// 预分配定时器容器
		Timers.Reserve(256);

		bIsInitialized = true;

		NLOG_EVENTS(Info, "TimerManager initialized");
	}

	/**
	 * @brief 关闭定时器管理器
	 */
	void Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}

		State = ETimerManagerState::Inactive;

		// 停止并清理所有定时器
		ClearAllTimers();

		NLOG_EVENTS(Info,
		            "TimerManager shutdown. Stats: {} timers created, {} destroyed",
		            Stats.TotalTimersCreated,
		            Stats.TotalTimersDestroyed);

		bIsInitialized = false;
	}

public:
	// === 定时器创建 ===

	/**
	 * @brief 设置定时器（函数版本）
	 */
	template <typename TFunc>
	FTimerHandle SetTimer(TFunc&& Function, float Delay, bool bLooping = false, const char* DebugName = "")
	{
		if (!bIsInitialized || State == ETimerManagerState::Inactive)
		{
			NLOG_EVENTS(Error, "TimerManager not initialized or inactive");
			return INVALID_TIMER_HANDLE;
		}

		// 创建委托
		FSimpleDelegate Delegate;
		Delegate.BindLambda(std::forward<TFunc>(Function));

		return SetTimerInternal(std::move(Delegate), Delay, bLooping, DebugName);
	}

	/**
	 * @brief 设置定时器（对象成员函数版本）
	 */
	template <typename TObject, typename TFunc>
	FTimerHandle SetTimer(
	    TObject* Object, TFunc&& Function, float Delay, bool bLooping = false, const char* DebugName = "")
	{
		if (!Object)
		{
			NLOG_EVENTS(Error, "Cannot set timer with null object");
			return INVALID_TIMER_HANDLE;
		}

		if (!bIsInitialized || State == ETimerManagerState::Inactive)
		{
			NLOG_EVENTS(Error, "TimerManager not initialized or inactive");
			return INVALID_TIMER_HANDLE;
		}

		// 创建委托
		FSimpleDelegate Delegate;
		Delegate.BindUObject(Object, std::forward<TFunc>(Function));

		return SetTimerInternal(std::move(Delegate), Delay, bLooping, DebugName);
	}

	/**
	 * @brief 设置定时器（智能指针版本）
	 */
	template <typename TObject, typename TFunc>
	FTimerHandle SetTimer(
	    TSharedPtr<TObject> Object, TFunc&& Function, float Delay, bool bLooping = false, const char* DebugName = "")
	{
		if (!Object.IsValid())
		{
			NLOG_EVENTS(Error, "Cannot set timer with invalid shared pointer");
			return INVALID_TIMER_HANDLE;
		}

		if (!bIsInitialized || State == ETimerManagerState::Inactive)
		{
			NLOG_EVENTS(Error, "TimerManager not initialized or inactive");
			return INVALID_TIMER_HANDLE;
		}

		// 创建委托
		FSimpleDelegate Delegate;
		Delegate.BindSP(Object, std::forward<TFunc>(Function));

		return SetTimerInternal(std::move(Delegate), Delay, bLooping, DebugName);
	}

	/**
	 * @brief 设置定时器（委托版本）
	 */
	FTimerHandle SetTimer(FSimpleDelegate&& Delegate, float Delay, bool bLooping = false, const char* DebugName = "")
	{
		if (!bIsInitialized || State == ETimerManagerState::Inactive)
		{
			NLOG_EVENTS(Error, "TimerManager not initialized or inactive");
			return INVALID_TIMER_HANDLE;
		}

		return SetTimerInternal(std::move(Delegate), Delay, bLooping, DebugName);
	}

public:
	// === 定时器控制 ===

	/**
	 * @brief 清除定时器
	 */
	bool ClearTimer(FTimerHandle& TimerHandle)
	{
		if (TimerHandle == INVALID_TIMER_HANDLE)
		{
			return false;
		}

		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		if (TimerPtr && TimerPtr->IsValid())
		{
			TimerPtr->MarkForDeletion();
			Stats.TotalTimersDestroyed++;

			NLOG_EVENTS(Trace, "Timer cleared, handle: {}, name: '{}'", TimerHandle, TimerPtr->DebugName.GetData());

			TimerHandle = INVALID_TIMER_HANDLE;
			return true;
		}

		TimerHandle = INVALID_TIMER_HANDLE;
		return false;
	}

	/**
	 * @brief 暂停定时器
	 */
	bool PauseTimer(FTimerHandle TimerHandle)
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		if (TimerPtr && TimerPtr->IsValid())
		{
			TimerPtr->Timer->Pause();
			NLOG_EVENTS(Trace, "Timer paused, handle: {}", TimerHandle);
			return true;
		}

		return false;
	}

	/**
	 * @brief 恢复定时器
	 */
	bool UnPauseTimer(FTimerHandle TimerHandle)
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		if (TimerPtr && TimerPtr->IsValid())
		{
			TimerPtr->Timer->Resume();
			NLOG_EVENTS(Trace, "Timer resumed, handle: {}", TimerHandle);
			return true;
		}

		return false;
	}

	/**
	 * @brief 检查定时器是否存在且有效
	 */
	bool IsTimerActive(FTimerHandle TimerHandle) const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		return TimerPtr && TimerPtr->IsValid() && TimerPtr->Timer->IsRunning();
	}

	/**
	 * @brief 检查定时器是否暂停
	 */
	bool IsTimerPaused(FTimerHandle TimerHandle) const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		return TimerPtr && TimerPtr->IsValid() && TimerPtr->Timer->IsPaused();
	}

	/**
	 * @brief 获取定时器剩余时间
	 */
	float GetTimerRemaining(FTimerHandle TimerHandle) const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		if (TimerPtr && TimerPtr->IsValid())
		{
			return static_cast<float>(TimerPtr->Timer->GetRemainingTime().GetTotalSeconds());
		}

		return -1.0f;
	}

	/**
	 * @brief 获取定时器经过时间
	 */
	float GetTimerElapsed(FTimerHandle TimerHandle) const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		auto* TimerPtr = Timers.Find(TimerHandle);
		if (TimerPtr && TimerPtr->IsValid())
		{
			return static_cast<float>(TimerPtr->Timer->GetElapsedTime().GetTotalSeconds());
		}

		return -1.0f;
	}

public:
	// === 管理器控制 ===

	/**
	 * @brief 暂停所有定时器
	 */
	void PauseAllTimers()
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		if (State == ETimerManagerState::Active)
		{
			State = ETimerManagerState::Paused;

			for (auto& Pair : Timers)
			{
				if (Pair.Value.IsValid())
				{
					Pair.Value.Timer->Pause();
				}
			}

			NLOG_EVENTS(Debug, "All timers paused");
		}
	}

	/**
	 * @brief 恢复所有定时器
	 */
	void UnPauseAllTimers()
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		if (State == ETimerManagerState::Paused)
		{
			State = ETimerManagerState::Active;

			for (auto& Pair : Timers)
			{
				if (Pair.Value.IsValid())
				{
					Pair.Value.Timer->Resume();
				}
			}

			NLOG_EVENTS(Debug, "All timers resumed");
		}
	}

	/**
	 * @brief 清除所有定时器
	 */
	void ClearAllTimers()
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		int32_t Count = Timers.Size();

		for (auto& Pair : Timers)
		{
			if (Pair.Value.IsValid())
			{
				Pair.Value.MarkForDeletion();
			}
		}

		Timers.Empty();
		Stats.TotalTimersDestroyed += Count;

		NLOG_EVENTS(Info, "All timers cleared, count: {}", Count);
	}

public:
	// === 更新和维护 ===

	/**
	 * @brief 更新定时器管理器（每帧调用）
	 */
	void Tick(float DeltaTime)
	{
		if (!bIsInitialized || State == ETimerManagerState::Inactive)
		{
			return;
		}

		CClock UpdateClock;
		Stats.TimersExecutedThisFrame = 0;

		std::lock_guard<std::mutex> Lock(TimersMutex);

		// 更新活跃定时器
		CTimespan DeltaTimespan = CTimespan::FromSeconds(static_cast<double>(DeltaTime));

		for (auto& Pair : Timers)
		{
			auto& ManagedTimer = Pair.Value;

			if (ManagedTimer.bIsPendingKill)
			{
				continue;
			}

			if (ManagedTimer.IsValid() && State == ETimerManagerState::Active)
			{
				ManagedTimer.Timer->Update(DeltaTimespan);

				// 检查定时器是否完成
				if (ManagedTimer.Timer->IsCompleted())
				{
					Stats.TimersExecutedThisFrame++;

					// 如果不是重复定时器，标记为删除
					if (!ManagedTimer.Timer->IsRepeating())
					{
						ManagedTimer.MarkForDeletion();
					}
				}
			}
		}

		// 清理待删除的定时器
		CleanupPendingTimers();

		// 更新统计信息
		Stats.TotalUpdateTime = UpdateClock.GetElapsed();
		Stats.LastUpdateTime = CDateTime::Now();

		// 更新计数器
		UpdateTimerCounts();
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 获取管理器状态
	 */
	ETimerManagerState GetState() const
	{
		return State;
	}

	/**
	 * @brief 检查是否已初始化
	 */
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

	/**
	 * @brief 获取统计信息
	 */
	const STimerManagerStats& GetStats() const
	{
		return Stats;
	}

	/**
	 * @brief 获取活跃定时器数量
	 */
	int32_t GetActiveTimerCount() const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);
		return Stats.ActiveTimers;
	}

	/**
	 * @brief 获取总定时器数量
	 */
	int32_t GetTotalTimerCount() const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);
		return Timers.Size();
	}

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成定时器管理器报告
	 */
	CString GenerateReport() const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Timer Manager Report ===\n"
		         "State: %s\n"
		         "Total Timers: %d\n"
		         "Active Timers: %u\n"
		         "Paused Timers: %u\n"
		         "Timers Created: %u\n"
		         "Timers Destroyed: %u\n"
		         "Executed This Frame: %u\n"
		         "Last Update Time: %.3f ms\n"
		         "Last Update: %s",
		         GetStateString().GetData(),
		         Timers.Size(),
		         Stats.ActiveTimers,
		         Stats.PausedTimers,
		         Stats.TotalTimersCreated,
		         Stats.TotalTimersDestroyed,
		         Stats.TimersExecutedThisFrame,
		         Stats.TotalUpdateTime.GetTotalMilliseconds(),
		         Stats.LastUpdateTime.ToString().GetData());

		return CString(Buffer);
	}

	/**
	 * @brief 获取所有定时器的调试信息
	 */
	TArray<CString, CMemoryManager> GetTimerDebugInfo() const
	{
		std::lock_guard<std::mutex> Lock(TimersMutex);

		TArray<CString, CMemoryManager> DebugInfo;
		DebugInfo.Reserve(Timers.Size());

		for (const auto& Pair : Timers)
		{
			const auto& ManagedTimer = Pair.Value;
			if (ManagedTimer.IsValid())
			{
				char Buffer[256];
				snprintf(Buffer,
				         sizeof(Buffer),
				         "Handle: %llu, Name: '%s', Remaining: %.2fs, State: %s",
				         Pair.Key,
				         ManagedTimer.DebugName.GetData(),
				         ManagedTimer.Timer->GetRemainingTime().GetTotalSeconds(),
				         ManagedTimer.Timer->IsRunning() ? "Running"
				                                         : (ManagedTimer.Timer->IsPaused() ? "Paused" : "Stopped"));

				DebugInfo.Add(CString(Buffer));
			}
		}

		return DebugInfo;
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 内部设置定时器实现
	 */
	FTimerHandle SetTimerInternal(FSimpleDelegate&& Delegate, float Delay, bool bLooping, const char* DebugName)
	{
		if (Delay <= 0.0f)
		{
			NLOG_EVENTS(Warning, "Timer delay must be positive, got: {}", Delay);
			return INVALID_TIMER_HANDLE;
		}

		std::lock_guard<std::mutex> Lock(TimersMutex);

		// 创建定时器
		CTimespan Duration = CTimespan::FromSeconds(static_cast<double>(Delay));
		auto Timer = MakeShared<NTimer>(Duration, nullptr, bLooping);

		// 设置回调
		Timer->SetCallback([Delegate = std::move(Delegate)]() mutable { Delegate.ExecuteIfBound(); });

		// 生成句柄
		FTimerHandle Handle = NextTimerHandle.fetch_add(1);

		// 创建管理定时器
		SManagedTimer ManagedTimer(Timer, Handle, DebugName);

		// 启动定时器
		Timer->Start();

		// 添加到管理器
		Timers.Add(Handle, ManagedTimer);
		Stats.TotalTimersCreated++;

		NLOG_EVENTS(Debug,
		            "Timer created, handle: {}, delay: {}s, looping: {}, name: '{}'",
		            Handle,
		            Delay,
		            bLooping,
		            DebugName);

		return Handle;
	}

	/**
	 * @brief 清理待删除的定时器
	 */
	void CleanupPendingTimers()
	{
		TArray<FTimerHandle, CMemoryManager> TimersToRemove;

		for (const auto& Pair : Timers)
		{
			if (Pair.Value.bIsPendingKill)
			{
				TimersToRemove.Add(Pair.Key);
			}
		}

		for (FTimerHandle Handle : TimersToRemove)
		{
			Timers.Remove(Handle);
		}
	}

	/**
	 * @brief 更新定时器计数
	 */
	void UpdateTimerCounts()
	{
		Stats.ActiveTimers = 0;
		Stats.PausedTimers = 0;

		for (const auto& Pair : Timers)
		{
			if (Pair.Value.IsValid())
			{
				if (Pair.Value.Timer->IsRunning())
				{
					Stats.ActiveTimers++;
				}
				else if (Pair.Value.Timer->IsPaused())
				{
					Stats.PausedTimers++;
				}
			}
		}
	}

	/**
	 * @brief 获取状态字符串
	 */
	CString GetStateString() const
	{
		switch (State)
		{
		case ETimerManagerState::Inactive:
			return "Inactive";
		case ETimerManagerState::Active:
			return "Active";
		case ETimerManagerState::Paused:
			return "Paused";
		default:
			return "Unknown";
		}
	}

private:
	// === 成员变量 ===

	ETimerManagerState State;                  // 管理器状态
	std::atomic<bool> bIsInitialized;          // 是否已初始化
	std::atomic<FTimerHandle> NextTimerHandle; // 下一个定时器句柄

	THashMap<FTimerHandle, SManagedTimer, CMemoryManager> Timers; // 定时器映射
	STimerManagerStats Stats;                                     // 统计信息

	mutable std::mutex TimersMutex; // 定时器互斥锁
};

// === 全局访问函数 ===

/**
 * @brief 获取定时器管理器
 */
inline CTimerManager& GetTimerManager()
{
	return CTimerManager::GetInstance();
}

/**
 * @brief 设置定时器的便捷函数
 */
template <typename TFunc>
FTimerHandle SetTimer(TFunc&& Function, float Delay, bool bLooping = false, const char* DebugName = "")
{
	return GetTimerManager().SetTimer(std::forward<TFunc>(Function), Delay, bLooping, DebugName);
}

template <typename TObject, typename TFunc>
FTimerHandle SetTimer(TObject* Object, TFunc&& Function, float Delay, bool bLooping = false, const char* DebugName = "")
{
	return GetTimerManager().SetTimer(Object, std::forward<TFunc>(Function), Delay, bLooping, DebugName);
}

/**
 * @brief 清除定时器的便捷函数
 */
inline bool ClearTimer(FTimerHandle& TimerHandle)
{
	return GetTimerManager().ClearTimer(TimerHandle);
}

} // namespace NLib