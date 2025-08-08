#pragma once

#include "TimeTypes.h"

#include <chrono>

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

} // namespace NLib