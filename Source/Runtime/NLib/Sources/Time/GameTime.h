#pragma once

#include "Core/Object.h"
#include "Logging/LogCategory.h"
#include "Math/MathTypes.h"
#include "TimeTypes.h"
#include "Timer.h"

#include <atomic>
#include <thread>

#include "GameTime.generate.h"

namespace NLib
{
/**
 * @brief 游戏时间管理器
 *
 * 管理游戏的时间系统，包括：
 * - 帧率控制
 * - 时间缩放
 * - 暂停/恢复
 * - 帧时间统计
 */
NCLASS()
class NGameTime : public NObject
{
	GENERATED_BODY()

public:
	// === 单例模式 ===

	/**
	 * @brief 获取游戏时间管理器实例
	 */
	static NGameTime& GetInstance()
	{
		static NGameTime Instance;
		return Instance;
	}

private:
	// === 构造函数（私有） ===

	NGameTime()
	    : TotalGameTime(CTimespan::Zero),
	      DeltaTime(CTimespan::Zero),
	      UnscaledDeltaTime(CTimespan::Zero),
	      TimeScale(1.0f),
	      TargetFrameRate(60.0f),
	      bIsPaused(false),
	      bIsInitialized(false),
	      FrameCount(0),
	      bVSyncEnabled(true),
	      MaxDeltaTime(CTimespan::FromMilliseconds(100.0)) // 最大100ms帧时间
	{
		// 初始化统计数据
		for (int32_t i = 0; i < FrameTimeSampleCount; ++i)
		{
			FrameTimeSamples[i] = 0.0;
		}
		FrameTimeSampleIndex = 0;
		AverageFrameTime = 0.0;
		MinFrameTime = 0.0;
		MaxFrameTime = 0.0;
	}

public:
	// 禁用拷贝
	NGameTime(const NGameTime&) = delete;
	NGameTime& operator=(const NGameTime&) = delete;

public:
	// === 初始化和更新 ===

	/**
	 * @brief 初始化游戏时间系统
	 */
	void Initialize()
	{
		if (bIsInitialized)
		{
			NLOG_CORE(Warning, "GameTime already initialized");
			return;
		}

		GameClock.Reset();
		LastFrameTime = CClock::GetCurrentTimestampUs();
		bIsInitialized = true;

		NLOG_CORE(Info, "GameTime initialized with target framerate: {} FPS", TargetFrameRate);
	}

	/**
	 * @brief 更新游戏时间（每帧调用）
	 */
	void Update()
	{
		if (!bIsInitialized)
		{
			NLOG_CORE(Error, "GameTime not initialized");
			return;
		}

		// 计算帧时间
		int64_t currentTime = CClock::GetCurrentTimestampUs();
		int64_t deltaUs = currentTime - LastFrameTime;
		LastFrameTime = currentTime;

		// 转换为时间跨度
		UnscaledDeltaTime = CTimespan::FromMicroseconds(static_cast<double>(deltaUs));

		// 限制最大帧时间，防止时间跳跃
		if (UnscaledDeltaTime > MaxDeltaTime)
		{
			NLOG_CORE(Warning,
			          "Frame time clamped from {} to {}",
			          UnscaledDeltaTime.ToString().GetData(),
			          MaxDeltaTime.ToString().GetData());
			UnscaledDeltaTime = MaxDeltaTime;
		}

		// 应用时间缩放和暂停
		if (bIsPaused)
		{
			DeltaTime = CTimespan::Zero;
		}
		else
		{
			DeltaTime = UnscaledDeltaTime * TimeScale;
			TotalGameTime += DeltaTime;
		}

		// 更新帧计数
		FrameCount++;

		// 更新统计信息
		UpdateFrameStatistics();

		// 记录详细信息（仅在Trace级别）
		NLOG_CORE(Trace,
		          "Frame {}: DeltaTime={:.2f}ms, FPS={:.1f}",
		          FrameCount,
		          DeltaTime.GetTotalMilliseconds(),
		          GetCurrentFPS());
	}

	/**
	 * @brief 关闭游戏时间系统
	 */
	void Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}

		NLOG_CORE(Info,
		          "GameTime shutdown. Total runtime: {}, Total frames: {}",
		          TotalGameTime.ToString().GetData(),
		          FrameCount);

		bIsInitialized = false;
	}

public:
	// === 时间访问 ===

	/**
	 * @brief 获取总游戏时间
	 */
	const CTimespan& GetTotalGameTime() const
	{
		return TotalGameTime;
	}

	/**
	 * @brief 获取帧时间间隔
	 */
	const CTimespan& GetDeltaTime() const
	{
		return DeltaTime;
	}

	/**
	 * @brief 获取未缩放的帧时间间隔
	 */
	const CTimespan& GetUnscaledDeltaTime() const
	{
		return UnscaledDeltaTime;
	}

	/**
	 * @brief 获取帧时间间隔（秒）
	 */
	float GetDeltaSeconds() const
	{
		return static_cast<float>(DeltaTime.GetTotalSeconds());
	}

	/**
	 * @brief 获取未缩放帧时间间隔（秒）
	 */
	float GetUnscaledDeltaSeconds() const
	{
		return static_cast<float>(UnscaledDeltaTime.GetTotalSeconds());
	}

	/**
	 * @brief 获取总游戏时间（秒）
	 */
	float GetTotalGameTimeSeconds() const
	{
		return static_cast<float>(TotalGameTime.GetTotalSeconds());
	}

public:
	// === 时间控制 ===

	/**
	 * @brief 设置时间缩放
	 */
	void SetTimeScale(float NewTimeScale)
	{
		TimeScale = CMath::Max(0.0f, NewTimeScale);
		NLOG_CORE(Debug, "Time scale set to: {}", TimeScale);
	}

	/**
	 * @brief 获取时间缩放
	 */
	float GetTimeScale() const
	{
		return TimeScale;
	}

	/**
	 * @brief 暂停游戏时间
	 */
	void Pause()
	{
		if (!bIsPaused)
		{
			bIsPaused = true;
			NLOG_CORE(Debug, "Game time paused");
		}
	}

	/**
	 * @brief 恢复游戏时间
	 */
	void Resume()
	{
		if (bIsPaused)
		{
			bIsPaused = false;
			// 重置时钟以避免时间跳跃
			LastFrameTime = CClock::GetCurrentTimestampUs();
			NLOG_CORE(Debug, "Game time resumed");
		}
	}

	/**
	 * @brief 检查是否暂停
	 */
	bool IsPaused() const
	{
		return bIsPaused;
	}

public:
	// === 帧率控制 ===

	/**
	 * @brief 设置目标帧率
	 */
	void SetTargetFrameRate(float NewFrameRate)
	{
		TargetFrameRate = CMath::Max(1.0f, NewFrameRate);
		NLOG_CORE(Debug, "Target frame rate set to: {} FPS", TargetFrameRate);
	}

	/**
	 * @brief 获取目标帧率
	 */
	float GetTargetFrameRate() const
	{
		return TargetFrameRate;
	}

	/**
	 * @brief 获取当前帧率
	 */
	float GetCurrentFPS() const
	{
		if (UnscaledDeltaTime.IsZero())
		{
			return 0.0f;
		}
		return static_cast<float>(1.0 / UnscaledDeltaTime.GetTotalSeconds());
	}

	/**
	 * @brief 获取平均帧率
	 */
	float GetAverageFPS() const
	{
		if (AverageFrameTime <= 0.0)
		{
			return 0.0f;
		}
		return static_cast<float>(1.0 / AverageFrameTime);
	}

	/**
	 * @brief 设置垂直同步
	 */
	void SetVSyncEnabled(bool bEnabled)
	{
		bVSyncEnabled = bEnabled;
		NLOG_CORE(Debug, "VSync {}", bEnabled ? "enabled" : "disabled");
	}

	/**
	 * @brief 检查垂直同步是否启用
	 */
	bool IsVSyncEnabled() const
	{
		return bVSyncEnabled;
	}

public:
	// === 帧统计 ===

	/**
	 * @brief 获取总帧数
	 */
	uint64_t GetFrameCount() const
	{
		return FrameCount;
	}

	/**
	 * @brief 获取平均帧时间（秒）
	 */
	double GetAverageFrameTime() const
	{
		return AverageFrameTime;
	}

	/**
	 * @brief 获取最小帧时间（秒）
	 */
	double GetMinFrameTime() const
	{
		return MinFrameTime;
	}

	/**
	 * @brief 获取最大帧时间（秒）
	 */
	double GetMaxFrameTime() const
	{
		return MaxFrameTime;
	}

	/**
	 * @brief 重置帧统计
	 */
	void ResetFrameStatistics()
	{
		for (int32_t i = 0; i < FrameTimeSampleCount; ++i)
		{
			FrameTimeSamples[i] = 0.0;
		}
		FrameTimeSampleIndex = 0;
		AverageFrameTime = 0.0;
		MinFrameTime = 0.0;
		MaxFrameTime = 0.0;

		NLOG_CORE(Debug, "Frame statistics reset");
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 等待下一帧（帧率限制）
	 */
	void WaitForNextFrame()
	{
		if (bVSyncEnabled || TargetFrameRate <= 0.0f)
		{
			return; // VSync控制或无限制帧率
		}

		double targetFrameTime = 1.0 / TargetFrameRate;
		double currentFrameTime = UnscaledDeltaTime.GetTotalSeconds();

		if (currentFrameTime < targetFrameTime)
		{
			double sleepTime = targetFrameTime - currentFrameTime;
			// 使用高精度睡眠（实际实现可能需要平台特定代码）
			std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
		}
	}

	/**
	 * @brief 设置最大帧时间
	 */
	void SetMaxDeltaTime(const CTimespan& NewMaxDeltaTime)
	{
		MaxDeltaTime = NewMaxDeltaTime;
		NLOG_CORE(Debug, "Max delta time set to: {}", MaxDeltaTime.ToString().GetData());
	}

	/**
	 * @brief 获取性能报告
	 */
	CString GetPerformanceReport() const
	{
		char Buffer[512];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "GameTime Performance Report:\n"
		         "  Total Runtime: %s\n"
		         "  Total Frames: %llu\n"
		         "  Current FPS: %.1f\n"
		         "  Average FPS: %.1f\n"
		         "  Average Frame Time: %.2f ms\n"
		         "  Min Frame Time: %.2f ms\n"
		         "  Max Frame Time: %.2f ms\n"
		         "  Time Scale: %.2f\n"
		         "  Is Paused: %s",
		         TotalGameTime.ToString().GetData(),
		         FrameCount,
		         GetCurrentFPS(),
		         GetAverageFPS(),
		         AverageFrameTime * 1000.0,
		         MinFrameTime * 1000.0,
		         MaxFrameTime * 1000.0,
		         TimeScale,
		         bIsPaused ? "Yes" : "No");
		return CString(Buffer);
	}

private:
	// === 内部函数 ===

	/**
	 * @brief 更新帧时间统计
	 */
	void UpdateFrameStatistics()
	{
		double frameTime = UnscaledDeltaTime.GetTotalSeconds();

		// 添加新样本
		FrameTimeSamples[FrameTimeSampleIndex] = frameTime;
		FrameTimeSampleIndex = (FrameTimeSampleIndex + 1) % FrameTimeSampleCount;

		// 计算统计信息
		double total = 0.0;
		double min = frameTime;
		double max = frameTime;

		for (int32_t i = 0; i < FrameTimeSampleCount; ++i)
		{
			double sample = FrameTimeSamples[i];
			if (sample > 0.0)
			{
				total += sample;
				min = CMath::Min(min, sample);
				max = CMath::Max(max, sample);
			}
		}

		AverageFrameTime = total / FrameTimeSampleCount;
		MinFrameTime = min;
		MaxFrameTime = max;
	}

private:
	// === 成员变量 ===

	// 时间状态
	CTimespan TotalGameTime;     // 总游戏时间
	CTimespan DeltaTime;         // 帧时间间隔（缩放后）
	CTimespan UnscaledDeltaTime; // 帧时间间隔（未缩放）
	CTimespan MaxDeltaTime;      // 最大帧时间限制
	float TimeScale;             // 时间缩放
	bool bIsPaused;              // 是否暂停
	bool bIsInitialized;         // 是否已初始化

	// 帧率控制
	float TargetFrameRate; // 目标帧率
	bool bVSyncEnabled;    // 垂直同步

	// 计时器
	CClock GameClock;      // 游戏时钟
	int64_t LastFrameTime; // 上一帧时间（微秒）

	// 统计信息
	uint64_t FrameCount;                                // 总帧数
	static constexpr int32_t FrameTimeSampleCount = 60; // 帧时间样本数
	double FrameTimeSamples[FrameTimeSampleCount];      // 帧时间样本
	int32_t FrameTimeSampleIndex;                       // 当前样本索引
	double AverageFrameTime;                            // 平均帧时间
	double MinFrameTime;                                // 最小帧时间
	double MaxFrameTime;                                // 最大帧时间
};

// === 全局访问函数 ===

/**
 * @brief 获取游戏时间管理器
 */
inline NGameTime& GetGameTime()
{
	return NGameTime::GetInstance();
}

/**
 * @brief 获取帧时间间隔（秒）
 */
inline float GetDeltaTime()
{
	return GetGameTime().GetDeltaSeconds();
}

/**
 * @brief 获取未缩放帧时间间隔（秒）
 */
inline float GetUnscaledDeltaTime()
{
	return GetGameTime().GetUnscaledDeltaSeconds();
}

/**
 * @brief 获取总游戏时间（秒）
 */
inline float GetGameTimeSeconds()
{
	return GetGameTime().GetTotalGameTimeSeconds();
}

// === 类型别名 ===
using FGameTime = NGameTime;

} // namespace NLib