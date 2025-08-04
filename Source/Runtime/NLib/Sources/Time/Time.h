#pragma once

/**
 * @file Time.h
 * @brief NLib时间管理库主头文件
 *
 * 提供完整的时间管理功能：
 * - 时间类型和时间跨度
 * - 高精度计时器和时钟
 * - 游戏时间管理和帧率控制
 * - 秒表和性能测量工具
 */

// 基础时间类型
#include "TimeTypes.h"

// 计时器和时钟
#include "Timer.h"

// 游戏时间管理
#include "GameTime.h"

#include <thread>

namespace NLib
{
/**
 * @brief 时间工具类 - 提供便捷的时间操作函数
 */
class CTimeUtils
{
public:
	// === 时间格式化 ===

	/**
	 * @brief 格式化时间跨度为可读字符串
	 * @param timespan 时间跨度
	 * @param bIncludeMilliseconds 是否包含毫秒
	 */
	static CString FormatTimespan(const CTimespan& timespan, bool bIncludeMilliseconds = false)
	{
		char Buffer[128];

		if (timespan.GetDays() > 0)
		{
			if (bIncludeMilliseconds)
			{
				snprintf(Buffer,
				         sizeof(Buffer),
				         "%dd %02dh %02dm %02ds %03dms",
				         timespan.GetDays(),
				         timespan.GetHours(),
				         timespan.GetMinutes(),
				         timespan.GetSeconds(),
				         timespan.GetMilliseconds());
			}
			else
			{
				snprintf(Buffer,
				         sizeof(Buffer),
				         "%dd %02dh %02dm %02ds",
				         timespan.GetDays(),
				         timespan.GetHours(),
				         timespan.GetMinutes(),
				         timespan.GetSeconds());
			}
		}
		else if (timespan.GetHours() > 0)
		{
			if (bIncludeMilliseconds)
			{
				snprintf(Buffer,
				         sizeof(Buffer),
				         "%02dh %02dm %02ds %03dms",
				         timespan.GetHours(),
				         timespan.GetMinutes(),
				         timespan.GetSeconds(),
				         timespan.GetMilliseconds());
			}
			else
			{
				snprintf(Buffer,
				         sizeof(Buffer),
				         "%02dh %02dm %02ds",
				         timespan.GetHours(),
				         timespan.GetMinutes(),
				         timespan.GetSeconds());
			}
		}
		else if (timespan.GetMinutes() > 0)
		{
			if (bIncludeMilliseconds)
			{
				snprintf(Buffer,
				         sizeof(Buffer),
				         "%02dm %02ds %03dms",
				         timespan.GetMinutes(),
				         timespan.GetSeconds(),
				         timespan.GetMilliseconds());
			}
			else
			{
				snprintf(Buffer, sizeof(Buffer), "%02dm %02ds", timespan.GetMinutes(), timespan.GetSeconds());
			}
		}
		else
		{
			if (bIncludeMilliseconds)
			{
				snprintf(Buffer, sizeof(Buffer), "%02ds %03dms", timespan.GetSeconds(), timespan.GetMilliseconds());
			}
			else
			{
				snprintf(Buffer, sizeof(Buffer), "%02ds", timespan.GetSeconds());
			}
		}

		return CString(Buffer);
	}

	/**
	 * @brief 格式化日期时间为标准格式
	 */
	static CString FormatDateTime(const CDateTime& dateTime, bool bIncludeTime = true)
	{
		if (bIncludeTime)
		{
			return dateTime.ToString();
		}
		else
		{
			return dateTime.ToDateString();
		}
	}

	/**
	 * @brief 格式化为ISO 8601格式
	 */
	static CString FormatISO8601(const CDateTime& dateTime)
	{
		char Buffer[32];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
		         dateTime.GetYear(),
		         dateTime.GetMonth(),
		         dateTime.GetDay(),
		         dateTime.GetHour(),
		         dateTime.GetMinute(),
		         dateTime.GetSecond(),
		         dateTime.GetMillisecond());
		return CString(Buffer);
	}

public:
	// === 时间转换 ===

	/**
	 * @brief 将秒转换为帧数
	 */
	static int32_t SecondsToFrames(float seconds, float frameRate = 60.0f)
	{
		return static_cast<int32_t>(seconds * frameRate);
	}

	/**
	 * @brief 将帧数转换为秒
	 */
	static float FramesToSeconds(int32_t frames, float frameRate = 60.0f)
	{
		return static_cast<float>(frames) / frameRate;
	}

	/**
	 * @brief 将毫秒转换为tick
	 */
	static int64_t MillisecondsToTicks(double milliseconds)
	{
		return static_cast<int64_t>(milliseconds * CTimespan::TicksPerMillisecond);
	}

	/**
	 * @brief 将tick转换为毫秒
	 */
	static double TicksToMilliseconds(int64_t ticks)
	{
		return static_cast<double>(ticks) / CTimespan::TicksPerMillisecond;
	}

public:
	// === 时间计算 ===

	/**
	 * @brief 计算两个时间之间的差值
	 */
	static CTimespan TimeDifference(const CDateTime& start, const CDateTime& end)
	{
		return end - start;
	}

	/**
	 * @brief 检查时间是否在指定范围内
	 */
	static bool IsTimeInRange(const CDateTime& time, const CDateTime& start, const CDateTime& end)
	{
		return time >= start && time <= end;
	}

	/**
	 * @brief 获取一天的开始时间
	 */
	static CDateTime GetStartOfDay(const CDateTime& dateTime)
	{
		return CDateTime(dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
	}

	/**
	 * @brief 获取一天的结束时间
	 */
	static CDateTime GetEndOfDay(const CDateTime& dateTime)
	{
		return GetStartOfDay(dateTime) + CTimespan(0, 23, 59, 59, 999);
	}

public:
	// === 性能测量 ===

	/**
	 * @brief 测量函数执行时间
	 */
	template <typename TFunc>
	static CTimespan MeasureExecutionTime(TFunc&& func)
	{
		CClock clock;
		func();
		return clock.GetElapsed();
	}

	/**
	 * @brief 测量函数执行时间（毫秒）
	 */
	template <typename TFunc>
	static double MeasureExecutionTimeMs(TFunc&& func)
	{
		return MeasureExecutionTime(func).GetTotalMilliseconds();
	}

public:
	// === 延迟和等待 ===

	/**
	 * @brief 延迟指定时间（阻塞）
	 */
	static void Sleep(const CTimespan& duration)
	{
		auto nanos = std::chrono::nanoseconds(static_cast<int64_t>(duration.GetTicks() * 100));
		std::this_thread::sleep_for(nanos);
	}

	/**
	 * @brief 延迟指定毫秒（阻塞）
	 */
	static void SleepMs(int32_t milliseconds)
	{
		Sleep(CTimespan::FromMilliseconds(static_cast<double>(milliseconds)));
	}

	/**
	 * @brief 高精度延迟（忙等待）
	 */
	static void BusySleep(const CTimespan& duration)
	{
		CClock clock;
		while (clock.GetElapsed() < duration)
		{
			// 忙等待
		}
	}

public:
	// === 帧率工具 ===

	/**
	 * @brief 计算平均帧率
	 */
	static float CalculateAverageFPS(const CTimespan& totalTime, uint64_t frameCount)
	{
		if (totalTime.IsZero())
		{
			return 0.0f;
		}
		return static_cast<float>(frameCount) / static_cast<float>(totalTime.GetTotalSeconds());
	}

	/**
	 * @brief 将帧率转换为帧时间
	 */
	static CTimespan FPSToFrameTime(float fps)
	{
		if (fps <= 0.0f)
		{
			return CTimespan::Zero;
		}
		return CTimespan::FromSeconds(1.0 / fps);
	}

	/**
	 * @brief 将帧时间转换为帧率
	 */
	static float FrameTimeToFPS(const CTimespan& frameTime)
	{
		if (frameTime.IsZero())
		{
			return 0.0f;
		}
		return static_cast<float>(1.0 / frameTime.GetTotalSeconds());
	}
};

/**
 * @brief 性能监视器类 - 用于监控代码段性能
 */
class CPerformanceMonitor
{
public:
	// === 构造函数 ===

	CPerformanceMonitor(const char* name)
	    : Name(name),
	      StartTime(CClock::GetCurrentTimestampUs())
	{
		NLOG_PERF(Debug, "Performance monitor '{}' started", Name);
	}

	~CPerformanceMonitor()
	{
		int64_t endTime = CClock::GetCurrentTimestampUs();
		double elapsedMs = static_cast<double>(endTime - StartTime) / 1000.0;
		NLOG_PERF(Debug, "Performance monitor '{}' completed in {:.3f}ms", Name, elapsedMs);
	}

private:
	const char* Name;
	int64_t StartTime;
};

// === 便捷宏定义 ===

/**
 * @brief 性能监控宏
 */
#define PROFILE_SCOPE(name) NLib::CPerformanceMonitor _prof(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

// === 类型别名 ===
using TimeUtils = CTimeUtils;
using PerformanceMonitor = CPerformanceMonitor;

} // namespace NLib