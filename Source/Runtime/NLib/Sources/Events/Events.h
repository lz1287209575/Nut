#pragma once

/**
 * @file Events.h
 * @brief NLib事件系统库主头文件
 *
 * 提供完整的事件驱动编程功能：
 * - 委托系统（单播和多播）
 * - 事件系统（类型安全的事件分发）
 * - 定时器管理（统一的定时器管理）
 * - 异步事件处理
 */

// 委托系统
#include "Delegate.h"

// 事件系统
#include "Event.h"

// 定时器管理
#include "Time/Timer.h"
#include "TimerManager.h"

#include <chrono>
#include <thread>

namespace NLib
{
/**
 * @brief 事件系统工具类
 */
class CEventUtils
{
public:
	// === 事件性能测试 ===

	/**
	 * @brief 测试事件分发性能
	 */
	struct SEventPerformanceTest
	{
		uint32_t EventCount = 0;       // 事件数量
		uint32_t ListenerCount = 0;    // 监听器数量
		CTimespan TotalDispatchTime;   // 总分发时间
		CTimespan AverageDispatchTime; // 平均分发时间
		double EventsPerSecond = 0.0;  // 每秒事件数
	};

	/**
	 * @brief 执行事件性能测试
	 */
	template <typename TEventType>
	static SEventPerformanceTest TestEventPerformance(uint32_t EventCount, uint32_t ListenerCount)
	{
		SEventPerformanceTest Result;
		Result.EventCount = EventCount;
		Result.ListenerCount = ListenerCount;

		auto& EventManager = CEventManager::GetInstance();
		TArray<FDelegateHandle, CMemoryManager> Handles;

		// 添加测试监听器
		for (uint32_t i = 0; i < ListenerCount; ++i)
		{
			auto Handle = EventManager.AddEventListener<TEventType>([i](const TEventType& Event) {
				// 简单的测试操作
				volatile int dummy = i * 2;
				(void)dummy;
			});
			Handles.Add(Handle);
		}

		// 执行性能测试
		CClock TestClock;

		for (uint32_t i = 0; i < EventCount; ++i)
		{
			TEventType TestEvent;
			EventManager.DispatchEvent(TestEvent);
		}

		Result.TotalDispatchTime = TestClock.GetElapsed();
		Result.AverageDispatchTime = CTimespan::FromSeconds(Result.TotalDispatchTime.GetTotalSeconds() / EventCount);
		Result.EventsPerSecond = EventCount / Result.TotalDispatchTime.GetTotalSeconds();

		// 清理测试监听器
		for (FDelegateHandle Handle : Handles)
		{
			EventManager.RemoveEventListener<TEventType>(Handle);
		}

		return Result;
	}

public:
	// === 委托性能测试 ===

	/**
	 * @brief 测试委托调用性能
	 */
	struct SDelegatePerformanceTest
	{
		uint32_t CallCount = 0;      // 调用次数
		CTimespan TotalCallTime;     // 总调用时间
		CTimespan AverageCallTime;   // 平均调用时间
		double CallsPerSecond = 0.0; // 每秒调用数
	};

	/**
	 * @brief 执行单播委托性能测试
	 */
	template <typename TSignature>
	static SDelegatePerformanceTest TestDelegatePerformance(uint32_t CallCount)
	{
		SDelegatePerformanceTest Result;
		Result.CallCount = CallCount;

		TDelegate<TSignature> TestDelegate;

		if constexpr (std::is_same_v<TSignature, void()>)
		{
			TestDelegate.BindLambda([]() {
				volatile int dummy = 42;
				(void)dummy;
			});
		}

		CClock TestClock;

		for (uint32_t i = 0; i < CallCount; ++i)
		{
			if constexpr (std::is_same_v<TSignature, void()>)
			{
				TestDelegate.Execute();
			}
		}

		Result.TotalCallTime = TestClock.GetElapsed();
		Result.AverageCallTime = CTimespan::FromSeconds(Result.TotalCallTime.GetTotalSeconds() / CallCount);
		Result.CallsPerSecond = CallCount / Result.TotalCallTime.GetTotalSeconds();

		return Result;
	}

	/**
	 * @brief 执行多播委托性能测试
	 */
	static SDelegatePerformanceTest TestMulticastDelegatePerformance(uint32_t CallCount, uint32_t ListenerCount)
	{
		SDelegatePerformanceTest Result;
		Result.CallCount = CallCount;

		TMulticastDelegate<void()> TestDelegate;

		// 添加监听器
		for (uint32_t i = 0; i < ListenerCount; ++i)
		{
			TestDelegate.AddLambda([i]() {
				volatile int dummy = i * 2;
				(void)dummy;
			});
		}

		CClock TestClock;

		for (uint32_t i = 0; i < CallCount; ++i)
		{
			TestDelegate.Broadcast();
		}

		Result.TotalCallTime = TestClock.GetElapsed();
		Result.AverageCallTime = CTimespan::FromSeconds(Result.TotalCallTime.GetTotalSeconds() / CallCount);
		Result.CallsPerSecond = CallCount / Result.TotalCallTime.GetTotalSeconds();

		return Result;
	}

public:
	// === 定时器性能测试 ===

	/**
	 * @brief 定时器性能测试结果
	 */
	struct STimerPerformanceTest
	{
		uint32_t TimerCount = 0;       // 定时器数量
		uint32_t ExecutedTimers = 0;   // 执行的定时器数
		CTimespan TestDuration;        // 测试持续时间
		CTimespan TotalUpdateTime;     // 总更新时间
		double UpdateEfficiency = 0.0; // 更新效率 (实际更新时间/总时间)
	};

	/**
	 * @brief 执行定时器性能测试
	 */
	static STimerPerformanceTest TestTimerPerformance(uint32_t TimerCount, float TestDurationSeconds)
	{
		STimerPerformanceTest Result;
		Result.TimerCount = TimerCount;

		auto& TimerManager = CTimerManager::GetInstance();
		TArray<FTimerHandle, CMemoryManager> TimerHandles;
		std::atomic<uint32_t> ExecutedCount{0};

		// 创建测试定时器
		for (uint32_t i = 0; i < TimerCount; ++i)
		{
			float Delay = 0.1f + (i % 100) * 0.01f; // 0.1s到1.1s的随机延迟
			auto Handle = TimerManager.SetTimer(
			    [&ExecutedCount]() { ExecutedCount.fetch_add(1); }, Delay, false, "PerfTest");

			TimerHandles.Add(Handle);
		}

		// 执行测试
		CClock TestClock;
		CClock UpdateClock;
		CTimespan TotalUpdateTime = CTimespan::Zero;

		while (TestClock.GetElapsedSeconds() < TestDurationSeconds)
		{
			UpdateClock.Reset();
			TimerManager.Tick(0.016f); // 模拟60FPS
			TotalUpdateTime += UpdateClock.GetElapsed();

			// 模拟帧间隔
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}

		Result.TestDuration = TestClock.GetElapsed();
		Result.TotalUpdateTime = TotalUpdateTime;
		Result.ExecutedTimers = ExecutedCount.load();
		Result.UpdateEfficiency = TotalUpdateTime.GetTotalSeconds() / Result.TestDuration.GetTotalSeconds();

		// 清理测试定时器
		for (FTimerHandle Handle : TimerHandles)
		{
			TimerManager.ClearTimer(Handle);
		}

		return Result;
	}

public:
	// === 内存使用分析 ===

	/**
	 * @brief 事件系统内存使用信息
	 */
	struct SEventSystemMemoryInfo
	{
		size_t DelegateMemoryUsage = 0; // 委托内存使用
		size_t EventMemoryUsage = 0;    // 事件内存使用
		size_t TimerMemoryUsage = 0;    // 定时器内存使用
		size_t TotalMemoryUsage = 0;    // 总内存使用
		uint32_t TotalEventTypes = 0;   // 事件类型数量
		uint32_t TotalListeners = 0;    // 监听器数量
		uint32_t TotalTimers = 0;       // 定时器数量
	};

	/**
	 * @brief 获取事件系统内存使用信息
	 */
	static SEventSystemMemoryInfo GetMemoryInfo()
	{
		SEventSystemMemoryInfo Info;

		auto& EventManager = CEventManager::GetInstance();
		auto& TimerManager = CTimerManager::GetInstance();

		// 获取事件管理器信息
		Info.TotalEventTypes = EventManager.GetEventTypeCount();
		// 注意：获取总监听器数量需要遍历所有事件类型，这里简化实现
		Info.TotalListeners = 0; // 简化实现

		// 获取定时器管理器信息
		Info.TotalTimers = TimerManager.GetTotalTimerCount();

		// 估算内存使用（简化实现）
		Info.DelegateMemoryUsage = Info.TotalListeners * sizeof(void*) * 4; // 粗略估算
		Info.EventMemoryUsage = Info.TotalEventTypes * 256;                 // 粗略估算
		Info.TimerMemoryUsage = Info.TotalTimers * sizeof(NTimer);
		Info.TotalMemoryUsage = Info.DelegateMemoryUsage + Info.EventMemoryUsage + Info.TimerMemoryUsage;

		return Info;
	}

public:
	// === 综合报告生成 ===

	/**
	 * @brief 生成事件系统综合报告
	 */
	static CString GenerateComprehensiveReport()
	{
		auto& EventManager = CEventManager::GetInstance();
		auto& TimerManager = CTimerManager::GetInstance();
		auto MemoryInfo = GetMemoryInfo();

		char Buffer[2048];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Event System Comprehensive Report ===\n\n"
		         "Event Manager:\n%s\n\n"
		         "Timer Manager:\n%s\n\n"
		         "Memory Usage:\n"
		         "  Delegate Memory: %.2f KB\n"
		         "  Event Memory: %.2f KB\n"
		         "  Timer Memory: %.2f KB\n"
		         "  Total Memory: %.2f KB\n\n"
		         "Summary:\n"
		         "  Event Types: %u\n"
		         "  Total Listeners: %u\n"
		         "  Total Timers: %u\n",
		         EventManager.GenerateEventReport().GetData(),
		         TimerManager.GenerateReport().GetData(),
		         static_cast<double>(MemoryInfo.DelegateMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.EventMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.TimerMemoryUsage) / 1024.0,
		         static_cast<double>(MemoryInfo.TotalMemoryUsage) / 1024.0,
		         MemoryInfo.TotalEventTypes,
		         MemoryInfo.TotalListeners,
		         MemoryInfo.TotalTimers);

		return CString(Buffer);
	}
};

// === 常用事件类型定义 ===

/**
 * @brief 应用程序生命周期事件
 */
class CApplicationStartEvent : public TEvent<CApplicationStartEvent>
{
public:
	CApplicationStartEvent()
	    : TEvent(EEventPriority::Highest)
	{}
	static const char* GetStaticEventName()
	{
		return "ApplicationStart";
	}
};

class CApplicationShutdownEvent : public TEvent<CApplicationShutdownEvent>
{
public:
	CApplicationShutdownEvent()
	    : TEvent(EEventPriority::Highest)
	{}
	static const char* GetStaticEventName()
	{
		return "ApplicationShutdown";
	}
};

/**
 * @brief 帧更新事件
 */
class CFrameUpdateEvent : public TEvent<CFrameUpdateEvent>
{
public:
	float DeltaTime;
	uint64_t FrameNumber;

	CFrameUpdateEvent(float InDeltaTime, uint64_t InFrameNumber)
	    : TEvent(EEventPriority::Normal),
	      DeltaTime(InDeltaTime),
	      FrameNumber(InFrameNumber)
	{}

	static const char* GetStaticEventName()
	{
		return "FrameUpdate";
	}
};

// === 便捷宏定义 ===

/**
 * @brief 快速事件声明宏
 */
#define DECLARE_SIMPLE_EVENT(EventName)                                                                                \
	class EventName : public NLib::TEvent<EventName>                                                                   \
	{                                                                                                                  \
	public:                                                                                                            \
		EventName()                                                                                                    \
		    : TEvent()                                                                                                 \
		{}                                                                                                             \
		static const char* GetStaticEventName()                                                                        \
		{                                                                                                              \
			return #EventName;                                                                                         \
		}                                                                                                              \
	};

/**
 * @brief 带参数的事件声明宏
 */
#define DECLARE_EVENT_WITH_PARAMS(EventName, ...)                                                                      \
	class EventName : public NLib::TEvent<EventName>                                                                   \
	{                                                                                                                  \
	public:                                                                                                            \
		EventName(__VA_ARGS__)                                                                                         \
		    : TEvent()                                                                                                 \
		{}                                                                                                             \
		static const char* GetStaticEventName()                                                                        \
		{                                                                                                              \
			return #EventName;                                                                                         \
		}                                                                                                              \
	};

// === 类型别名 ===
using EventUtils = CEventUtils;

} // namespace NLib