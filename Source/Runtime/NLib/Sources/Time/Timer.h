#pragma once

#include "Core/Object.h"
#include "Logging/LogCategory.h"
#include "TimeTypes.h"

#include <chrono>
#include <functional>

#include "Timer.generate.h"

namespace NLib
{
/**
 * @brief 高精度时钟类
 *
 * 提供高精度时间测量功能，用于性能分析和精确计时
 */
class CClock
{
public:
	// === 构造函数 ===

	/**
	 * @brief 构造函数，自动记录起始时间
	 */
	CClock()
	{
		Reset();
	}

public:
	// === 时间测量 ===

	/**
	 * @brief 重置时钟
	 */
	void Reset()
	{
		StartTime = std::chrono::high_resolution_clock::now();
	}

	/**
	 * @brief 获取经过的时间（秒）
	 */
	double GetElapsedSeconds() const
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - StartTime);
		return static_cast<double>(duration.count()) / 1000000000.0;
	}

	/**
	 * @brief 获取经过的时间（毫秒）
	 */
	double GetElapsedMilliseconds() const
	{
		return GetElapsedSeconds() * 1000.0;
	}

	/**
	 * @brief 获取经过的时间（微秒）
	 */
	double GetElapsedMicroseconds() const
	{
		return GetElapsedSeconds() * 1000000.0;
	}

	/**
	 * @brief 获取经过的时间跨度
	 */
	CTimespan GetElapsed() const
	{
		return CTimespan::FromSeconds(GetElapsedSeconds());
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 测量并重置时钟
	 * @return 上次重置以来经过的时间
	 */
	CTimespan Lap()
	{
		CTimespan elapsed = GetElapsed();
		Reset();
		return elapsed;
	}

	/**
	 * @brief 获取当前时间戳（毫秒）
	 */
	static int64_t GetCurrentTimestampMs()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	}

	/**
	 * @brief 获取当前时间戳（微秒）
	 */
	static int64_t GetCurrentTimestampUs()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	}

private:
	std::chrono::high_resolution_clock::time_point StartTime;
};

/**
 * @brief 计时器状态枚举
 */
enum class ETimerStatus : uint8_t
{
	Stopped, // 已停止
	Running, // 运行中
	Paused   // 已暂停
};

/**
 * @brief 计时器类
 *
 * 提供可暂停/恢复的计时器功能，支持回调函数
 */
NCLASS()
class NTimer : public NObject
{
	GENERATED_BODY()

public:
	// === 类型定义 ===
	using FTimerCallback = std::function<void()>;

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数
	 */
	NTimer()
	    : Duration(CTimespan::Zero),
	      ElapsedTime(CTimespan::Zero),
	      Status(ETimerStatus::Stopped),
	      bRepeating(false),
	      RepeatCount(0),
	      MaxRepeats(-1)
	{}

	/**
	 * @brief 构造函数
	 * @param InDuration 计时器持续时间
	 * @param InCallback 回调函数
	 * @param bInRepeating 是否重复
	 */
	NTimer(const CTimespan& InDuration, FTimerCallback InCallback = nullptr, bool bInRepeating = false)
	    : Duration(InDuration),
	      ElapsedTime(CTimespan::Zero),
	      Status(ETimerStatus::Stopped),
	      bRepeating(bInRepeating),
	      RepeatCount(0),
	      MaxRepeats(-1),
	      Callback(InCallback)
	{}

public:
	// === 计时器控制 ===

	/**
	 * @brief 启动计时器
	 */
	void Start()
	{
		if (Status == ETimerStatus::Stopped)
		{
			ElapsedTime = CTimespan::Zero;
			RepeatCount = 0;
		}

		Status = ETimerStatus::Running;
		LastUpdateTime = CClock::GetCurrentTimestampUs();

		NLOG_CORE(Debug, "Timer started with duration: {}", Duration.ToString().GetData());
	}

	/**
	 * @brief 停止计时器
	 */
	void Stop()
	{
		Status = ETimerStatus::Stopped;
		ElapsedTime = CTimespan::Zero;
		RepeatCount = 0;

		NLOG_CORE(Debug, "Timer stopped");
	}

	/**
	 * @brief 暂停计时器
	 */
	void Pause()
	{
		if (Status == ETimerStatus::Running)
		{
			Status = ETimerStatus::Paused;
			NLOG_CORE(Debug, "Timer paused");
		}
	}

	/**
	 * @brief 恢复计时器
	 */
	void Resume()
	{
		if (Status == ETimerStatus::Paused)
		{
			Status = ETimerStatus::Running;
			LastUpdateTime = CClock::GetCurrentTimestampUs();
			NLOG_CORE(Debug, "Timer resumed");
		}
	}

	/**
	 * @brief 重置计时器
	 */
	void Reset()
	{
		ElapsedTime = CTimespan::Zero;
		RepeatCount = 0;
		LastUpdateTime = CClock::GetCurrentTimestampUs();

		NLOG_CORE(Debug, "Timer reset");
	}

public:
	// === 计时器更新 ===

	/**
	 * @brief 更新计时器（需要定期调用）
	 * @param DeltaTime 时间增量
	 */
	void Update(const CTimespan& DeltaTime)
	{
		if (Status != ETimerStatus::Running)
		{
			return;
		}

		ElapsedTime += DeltaTime;

		// 检查是否到时间
		if (ElapsedTime >= Duration)
		{
			// 触发回调
			if (Callback)
			{
				try
				{
					Callback();
				}
				catch (...)
				{
					NLOG_CORE(Error, "Timer callback threw an exception");
				}
			}

			RepeatCount++;

			// 检查是否需要重复
			if (bRepeating && (MaxRepeats < 0 || RepeatCount < MaxRepeats))
			{
				// 重复计时器，重置经过时间
				ElapsedTime = CTimespan::Zero;
				NLOG_CORE(Trace, "Timer repeated, count: {}", RepeatCount);
			}
			else
			{
				// 停止计时器
				Status = ETimerStatus::Stopped;
				NLOG_CORE(Debug, "Timer completed after {} repeats", RepeatCount);
			}
		}
	}

	/**
	 * @brief 自动更新（基于系统时间）
	 */
	void AutoUpdate()
	{
		if (Status != ETimerStatus::Running)
		{
			return;
		}

		int64_t currentTime = CClock::GetCurrentTimestampUs();
		int64_t deltaUs = currentTime - LastUpdateTime;
		LastUpdateTime = currentTime;

		CTimespan deltaTime = CTimespan::FromMicroseconds(static_cast<double>(deltaUs));
		Update(deltaTime);
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查是否正在运行
	 */
	bool IsRunning() const
	{
		return Status == ETimerStatus::Running;
	}

	/**
	 * @brief 检查是否已停止
	 */
	bool IsStopped() const
	{
		return Status == ETimerStatus::Stopped;
	}

	/**
	 * @brief 检查是否已暂停
	 */
	bool IsPaused() const
	{
		return Status == ETimerStatus::Paused;
	}

	/**
	 * @brief 检查是否已完成
	 */
	bool IsCompleted() const
	{
		return Status == ETimerStatus::Stopped && ElapsedTime >= Duration;
	}

	/**
	 * @brief 获取进度（0.0 - 1.0）
	 */
	float GetProgress() const
	{
		if (Duration.IsZero())
		{
			return 1.0f;
		}
		return static_cast<float>(ElapsedTime.GetTotalSeconds() / Duration.GetTotalSeconds());
	}

	/**
	 * @brief 获取剩余时间
	 */
	CTimespan GetRemainingTime() const
	{
		if (ElapsedTime >= Duration)
		{
			return CTimespan::Zero;
		}
		return Duration - ElapsedTime;
	}

public:
	// === 属性访问 ===

	/**
	 * @brief 设置持续时间
	 */
	void SetDuration(const CTimespan& NewDuration)
	{
		Duration = NewDuration;
	}

	/**
	 * @brief 获取持续时间
	 */
	const CTimespan& GetDuration() const
	{
		return Duration;
	}

	/**
	 * @brief 获取已经过时间
	 */
	const CTimespan& GetElapsedTime() const
	{
		return ElapsedTime;
	}

	/**
	 * @brief 设置回调函数
	 */
	void SetCallback(FTimerCallback NewCallback)
	{
		Callback = NewCallback;
	}

	/**
	 * @brief 设置是否重复
	 */
	void SetRepeating(bool bNewRepeating)
	{
		bRepeating = bNewRepeating;
	}

	/**
	 * @brief 是否重复
	 */
	bool IsRepeating() const
	{
		return bRepeating;
	}

	/**
	 * @brief 设置最大重复次数
	 */
	void SetMaxRepeats(int32_t NewMaxRepeats)
	{
		MaxRepeats = NewMaxRepeats;
	}

	/**
	 * @brief 获取重复次数
	 */
	int32_t GetRepeatCount() const
	{
		return RepeatCount;
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 转换为字符串
	 */
	CString ToString() const override
	{
		char Buffer[256];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "Timer(Duration: %s, Elapsed: %s, Status: %s, Repeating: %s, Repeats: %d)",
		         Duration.ToString().GetData(),
		         ElapsedTime.ToString().GetData(),
		         GetStatusString().GetData(),
		         bRepeating ? "true" : "false",
		         RepeatCount);
		return CString(Buffer);
	}

private:
	// === 内部函数 ===

	CString GetStatusString() const
	{
		switch (Status)
		{
		case ETimerStatus::Stopped:
			return "Stopped";
		case ETimerStatus::Running:
			return "Running";
		case ETimerStatus::Paused:
			return "Paused";
		default:
			return "Unknown";
		}
	}

private:
	// === 成员变量 ===
	CTimespan Duration;      // 计时器持续时间
	CTimespan ElapsedTime;   // 已经过时间
	ETimerStatus Status;     // 计时器状态
	bool bRepeating;         // 是否重复
	int32_t RepeatCount;     // 重复次数
	int32_t MaxRepeats;      // 最大重复次数（-1表示无限）
	FTimerCallback Callback; // 回调函数
	int64_t LastUpdateTime;  // 上次更新时间（微秒）
};

/**
 * @brief 秒表类
 *
 * 提供秒表功能，可以测量累计时间
 */
class CStopwatch
{
public:
	// === 构造函数 ===

	CStopwatch()
	    : TotalElapsed(CTimespan::Zero),
	      bIsRunning(false)
	{}

public:
	// === 秒表控制 ===

	/**
	 * @brief 启动秒表
	 */
	void Start()
	{
		if (!bIsRunning)
		{
			bIsRunning = true;
			Clock.Reset();
		}
	}

	/**
	 * @brief 停止秒表
	 */
	void Stop()
	{
		if (bIsRunning)
		{
			TotalElapsed += Clock.GetElapsed();
			bIsRunning = false;
		}
	}

	/**
	 * @brief 重置秒表
	 */
	void Reset()
	{
		TotalElapsed = CTimespan::Zero;
		bIsRunning = false;
	}

	/**
	 * @brief 重新开始（重置并启动）
	 */
	void Restart()
	{
		Reset();
		Start();
	}

public:
	// === 时间获取 ===

	/**
	 * @brief 获取总经过时间
	 */
	CTimespan GetElapsed() const
	{
		CTimespan total = TotalElapsed;
		if (bIsRunning)
		{
			total += Clock.GetElapsed();
		}
		return total;
	}

	/**
	 * @brief 获取经过的秒数
	 */
	double GetElapsedSeconds() const
	{
		return GetElapsed().GetTotalSeconds();
	}

	/**
	 * @brief 获取经过的毫秒数
	 */
	double GetElapsedMilliseconds() const
	{
		return GetElapsed().GetTotalMilliseconds();
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查是否正在运行
	 */
	bool IsRunning() const
	{
		return bIsRunning;
	}

public:
	// === 静态工厂函数 ===

	/**
	 * @brief 创建并启动新的秒表
	 */
	static CStopwatch StartNew()
	{
		CStopwatch stopwatch;
		stopwatch.Start();
		return stopwatch;
	}

private:
	CClock Clock;           // 内部时钟
	CTimespan TotalElapsed; // 总经过时间
	bool bIsRunning;        // 是否正在运行
};

// === 类型别名 ===
using FTimer = NTimer;
using FClock = CClock;
using FStopwatch = CStopwatch;

} // namespace NLib