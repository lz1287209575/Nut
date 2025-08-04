#pragma once

#include "Containers/TString.h"

#include <chrono>
#include <cstdint>
#include <ctime>

namespace NLib
{
/**
 * @brief 时间单位枚举
 */
enum class ETimeUnit : uint8_t
{
	Nanoseconds,  // 纳秒
	Microseconds, // 微秒
	Milliseconds, // 毫秒
	Seconds,      // 秒
	Minutes,      // 分钟
	Hours,        // 小时
	Days          // 天
};

/**
 * @brief 时间跨度类
 *
 * 表示一段时间长度，提供各种时间单位的转换和运算
 */
class CTimespan
{
public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（零时间）
	 */
	CTimespan()
	    : Ticks(0)
	{}

	/**
	 * @brief 从tick数构造
	 * @param InTicks tick数（100纳秒为单位）
	 */
	explicit CTimespan(int64_t InTicks)
	    : Ticks(InTicks)
	{}

	/**
	 * @brief 从时分秒构造
	 */
	CTimespan(int32_t Hours, int32_t Minutes, int32_t Seconds)
	    : Ticks(Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond)
	{}

	/**
	 * @brief 从天时分秒构造
	 */
	CTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds)
	    : Ticks(Days * TicksPerDay + Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond)
	{}

	/**
	 * @brief 从天时分秒毫秒构造
	 */
	CTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds)
	    : Ticks(Days * TicksPerDay + Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond +
	            Milliseconds * TicksPerMillisecond)
	{}

public:
	// === 时间常量（100纳秒tick为单位）===

	static constexpr int64_t TicksPerMicrosecond = 10LL;
	static constexpr int64_t TicksPerMillisecond = 10000LL;
	static constexpr int64_t TicksPerSecond = 10000000LL;
	static constexpr int64_t TicksPerMinute = 600000000LL;
	static constexpr int64_t TicksPerHour = 36000000000LL;
	static constexpr int64_t TicksPerDay = 864000000000LL;

public:
	// === 工厂函数 ===

	/**
	 * @brief 从天数创建时间跨度
	 */
	static CTimespan FromDays(double Days)
	{
		return CTimespan(static_cast<int64_t>(Days * TicksPerDay));
	}

	/**
	 * @brief 从小时创建时间跨度
	 */
	static CTimespan FromHours(double Hours)
	{
		return CTimespan(static_cast<int64_t>(Hours * TicksPerHour));
	}

	/**
	 * @brief 从分钟创建时间跨度
	 */
	static CTimespan FromMinutes(double Minutes)
	{
		return CTimespan(static_cast<int64_t>(Minutes * TicksPerMinute));
	}

	/**
	 * @brief 从秒数创建时间跨度
	 */
	static CTimespan FromSeconds(double Seconds)
	{
		return CTimespan(static_cast<int64_t>(Seconds * TicksPerSecond));
	}

	/**
	 * @brief 从毫秒创建时间跨度
	 */
	static CTimespan FromMilliseconds(double Milliseconds)
	{
		return CTimespan(static_cast<int64_t>(Milliseconds * TicksPerMillisecond));
	}

	/**
	 * @brief 从微秒创建时间跨度
	 */
	static CTimespan FromMicroseconds(double Microseconds)
	{
		return CTimespan(static_cast<int64_t>(Microseconds * TicksPerMicrosecond));
	}

public:
	// === 获取时间分量 ===

	/**
	 * @brief 获取总的tick数
	 */
	int64_t GetTicks() const
	{
		return Ticks;
	}

	/**
	 * @brief 获取天数部分
	 */
	int32_t GetDays() const
	{
		return static_cast<int32_t>(Ticks / TicksPerDay);
	}

	/**
	 * @brief 获取小时部分（0-23）
	 */
	int32_t GetHours() const
	{
		return static_cast<int32_t>((Ticks / TicksPerHour) % 24);
	}

	/**
	 * @brief 获取分钟部分（0-59）
	 */
	int32_t GetMinutes() const
	{
		return static_cast<int32_t>((Ticks / TicksPerMinute) % 60);
	}

	/**
	 * @brief 获取秒数部分（0-59）
	 */
	int32_t GetSeconds() const
	{
		return static_cast<int32_t>((Ticks / TicksPerSecond) % 60);
	}

	/**
	 * @brief 获取毫秒部分（0-999）
	 */
	int32_t GetMilliseconds() const
	{
		return static_cast<int32_t>((Ticks / TicksPerMillisecond) % 1000);
	}

public:
	// === 获取总时间量 ===

	/**
	 * @brief 获取总天数（浮点数）
	 */
	double GetTotalDays() const
	{
		return static_cast<double>(Ticks) / TicksPerDay;
	}

	/**
	 * @brief 获取总小时数（浮点数）
	 */
	double GetTotalHours() const
	{
		return static_cast<double>(Ticks) / TicksPerHour;
	}

	/**
	 * @brief 获取总分钟数（浮点数）
	 */
	double GetTotalMinutes() const
	{
		return static_cast<double>(Ticks) / TicksPerMinute;
	}

	/**
	 * @brief 获取总秒数（浮点数）
	 */
	double GetTotalSeconds() const
	{
		return static_cast<double>(Ticks) / TicksPerSecond;
	}

	/**
	 * @brief 获取总毫秒数（浮点数）
	 */
	double GetTotalMilliseconds() const
	{
		return static_cast<double>(Ticks) / TicksPerMillisecond;
	}

	/**
	 * @brief 获取总微秒数（浮点数）
	 */
	double GetTotalMicroseconds() const
	{
		return static_cast<double>(Ticks) / TicksPerMicrosecond;
	}

public:
	// === 运算符重载 ===

	CTimespan operator+(const CTimespan& Other) const
	{
		return CTimespan(Ticks + Other.Ticks);
	}

	CTimespan operator-(const CTimespan& Other) const
	{
		return CTimespan(Ticks - Other.Ticks);
	}

	CTimespan operator*(double Multiplier) const
	{
		return CTimespan(static_cast<int64_t>(Ticks * Multiplier));
	}

	CTimespan operator/(double Divisor) const
	{
		return CTimespan(static_cast<int64_t>(Ticks / Divisor));
	}

	CTimespan& operator+=(const CTimespan& Other)
	{
		Ticks += Other.Ticks;
		return *this;
	}

	CTimespan& operator-=(const CTimespan& Other)
	{
		Ticks -= Other.Ticks;
		return *this;
	}

	CTimespan& operator*=(double Multiplier)
	{
		Ticks = static_cast<int64_t>(Ticks * Multiplier);
		return *this;
	}

	CTimespan& operator/=(double Divisor)
	{
		Ticks = static_cast<int64_t>(Ticks / Divisor);
		return *this;
	}

	CTimespan operator-() const
	{
		return CTimespan(-Ticks);
	}

public:
	// === 比较运算符 ===

	bool operator==(const CTimespan& Other) const
	{
		return Ticks == Other.Ticks;
	}
	bool operator!=(const CTimespan& Other) const
	{
		return Ticks != Other.Ticks;
	}
	bool operator<(const CTimespan& Other) const
	{
		return Ticks < Other.Ticks;
	}
	bool operator<=(const CTimespan& Other) const
	{
		return Ticks <= Other.Ticks;
	}
	bool operator>(const CTimespan& Other) const
	{
		return Ticks > Other.Ticks;
	}
	bool operator>=(const CTimespan& Other) const
	{
		return Ticks >= Other.Ticks;
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 获取绝对值
	 */
	CTimespan GetAbs() const
	{
		return CTimespan(Ticks >= 0 ? Ticks : -Ticks);
	}

	/**
	 * @brief 检查是否为零
	 */
	bool IsZero() const
	{
		return Ticks == 0;
	}

	/**
	 * @brief 检查是否为负数
	 */
	bool IsNegative() const
	{
		return Ticks < 0;
	}

	/**
	 * @brief 检查是否为正数
	 */
	bool IsPositive() const
	{
		return Ticks > 0;
	}

	/**
	 * @brief 转换为字符串
	 */
	CString ToString() const
	{
		char Buffer[64];
		if (GetDays() > 0)
		{
			snprintf(Buffer,
			         sizeof(Buffer),
			         "%d.%02d:%02d:%02d.%03d",
			         GetDays(),
			         GetHours(),
			         GetMinutes(),
			         GetSeconds(),
			         GetMilliseconds());
		}
		else
		{
			snprintf(Buffer,
			         sizeof(Buffer),
			         "%02d:%02d:%02d.%03d",
			         GetHours(),
			         GetMinutes(),
			         GetSeconds(),
			         GetMilliseconds());
		}
		return CString(Buffer);
	}

public:
	// === 常用时间跨度常量 ===

	static const CTimespan Zero;     // 零时间
	static const CTimespan MinValue; // 最小时间跨度
	static const CTimespan MaxValue; // 最大时间跨度

private:
	int64_t Ticks; // 时间tick数（100纳秒为单位）
};

// === 常量定义 ===
inline const CTimespan CTimespan::Zero(0);
inline const CTimespan CTimespan::MinValue(INT64_MIN);
inline const CTimespan CTimespan::MaxValue(INT64_MAX);

/**
 * @brief 日期时间类
 *
 * 表示具体的日期和时间点
 */
class CDateTime
{
public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（Unix纪元）
	 */
	CDateTime()
	    : Ticks(0)
	{}

	/**
	 * @brief 从tick数构造
	 */
	explicit CDateTime(int64_t InTicks)
	    : Ticks(InTicks)
	{}

	/**
	 * @brief 从年月日构造
	 */
	CDateTime(int32_t Year, int32_t Month, int32_t Day)
	{
		Ticks = DateToTicks(Year, Month, Day);
	}

	/**
	 * @brief 从年月日时分秒构造
	 */
	CDateTime(int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Minute, int32_t Second)
	{
		Ticks = DateToTicks(Year, Month, Day) + TimeToTicks(Hour, Minute, Second);
	}

public:
	// === 静态工厂函数 ===

	/**
	 * @brief 获取当前时间
	 */
	static CDateTime Now()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() / 100; // 转换为100纳秒tick
		return CDateTime(ticks);
	}

	/**
	 * @brief 获取当前UTC时间
	 */
	static CDateTime UtcNow()
	{
		// 简化实现，实际应用中需要考虑时区转换
		return Now();
	}

	/**
	 * @brief 获取今天的日期（时间为00:00:00）
	 */
	static CDateTime Today()
	{
		CDateTime now = Now();
		return CDateTime(now.GetYear(), now.GetMonth(), now.GetDay());
	}

	/**
	 * @brief 从time_t创建CDateTime
	 */
	static CDateTime FromTimeT(time_t TimeValue)
	{
		// time_t是秒数，需要转换为100纳秒tick
		return CDateTime(static_cast<int64_t>(TimeValue) * CTimespan::TicksPerSecond);
	}

public:
	// === 日期时间分量获取 ===

	/**
	 * @brief 获取年份
	 */
	int32_t GetYear() const
	{
		int32_t year, month, day;
		TicksToDate(Ticks, year, month, day);
		return year;
	}

	/**
	 * @brief 获取月份（1-12）
	 */
	int32_t GetMonth() const
	{
		int32_t year, month, day;
		TicksToDate(Ticks, year, month, day);
		return month;
	}

	/**
	 * @brief 获取日期（1-31）
	 */
	int32_t GetDay() const
	{
		int32_t year, month, day;
		TicksToDate(Ticks, year, month, day);
		return day;
	}

	/**
	 * @brief 获取小时（0-23）
	 */
	int32_t GetHour() const
	{
		return static_cast<int32_t>((Ticks / CTimespan::TicksPerHour) % 24);
	}

	/**
	 * @brief 获取分钟（0-59）
	 */
	int32_t GetMinute() const
	{
		return static_cast<int32_t>((Ticks / CTimespan::TicksPerMinute) % 60);
	}

	/**
	 * @brief 获取秒数（0-59）
	 */
	int32_t GetSecond() const
	{
		return static_cast<int32_t>((Ticks / CTimespan::TicksPerSecond) % 60);
	}

	/**
	 * @brief 获取毫秒（0-999）
	 */
	int32_t GetMillisecond() const
	{
		return static_cast<int32_t>((Ticks / CTimespan::TicksPerMillisecond) % 1000);
	}

public:
	// === 运算符重载 ===

	CDateTime operator+(const CTimespan& timespan) const
	{
		return CDateTime(Ticks + timespan.GetTicks());
	}

	CDateTime operator-(const CTimespan& timespan) const
	{
		return CDateTime(Ticks - timespan.GetTicks());
	}

	CTimespan operator-(const CDateTime& other) const
	{
		return CTimespan(Ticks - other.Ticks);
	}

	CDateTime& operator+=(const CTimespan& timespan)
	{
		Ticks += timespan.GetTicks();
		return *this;
	}

	CDateTime& operator-=(const CTimespan& timespan)
	{
		Ticks -= timespan.GetTicks();
		return *this;
	}

public:
	// === 比较运算符 ===

	bool operator==(const CDateTime& Other) const
	{
		return Ticks == Other.Ticks;
	}
	bool operator!=(const CDateTime& Other) const
	{
		return Ticks != Other.Ticks;
	}
	bool operator<(const CDateTime& Other) const
	{
		return Ticks < Other.Ticks;
	}
	bool operator<=(const CDateTime& Other) const
	{
		return Ticks <= Other.Ticks;
	}
	bool operator>(const CDateTime& Other) const
	{
		return Ticks > Other.Ticks;
	}
	bool operator>=(const CDateTime& Other) const
	{
		return Ticks >= Other.Ticks;
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 获取tick数
	 */
	int64_t GetTicks() const
	{
		return Ticks;
	}

	/**
	 * @brief 获取日期部分
	 */
	CDateTime GetDate() const
	{
		return CDateTime(GetYear(), GetMonth(), GetDay());
	}

	/**
	 * @brief 获取时间部分
	 */
	CTimespan GetTimeOfDay() const
	{
		return CTimespan(Ticks % CTimespan::TicksPerDay);
	}

	/**
	 * @brief 转换为字符串
	 */
	CString ToString() const
	{
		char Buffer[64];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "%04d-%02d-%02d %02d:%02d:%02d.%03d",
		         GetYear(),
		         GetMonth(),
		         GetDay(),
		         GetHour(),
		         GetMinute(),
		         GetSecond(),
		         GetMillisecond());
		return CString(Buffer);
	}

	/**
	 * @brief 转换为日期字符串
	 */
	CString ToDateString() const
	{
		char Buffer[32];
		snprintf(Buffer, sizeof(Buffer), "%04d-%02d-%02d", GetYear(), GetMonth(), GetDay());
		return CString(Buffer);
	}

	/**
	 * @brief 转换为时间字符串
	 */
	CString ToTimeString() const
	{
		char Buffer[32];
		snprintf(Buffer, sizeof(Buffer), "%02d:%02d:%02d", GetHour(), GetMinute(), GetSecond());
		return CString(Buffer);
	}

	/**
	 * @brief 转换为time_t
	 */
	time_t ToTimeT() const
	{
		// 将100纳秒tick转换为秒数
		return static_cast<time_t>(Ticks / CTimespan::TicksPerSecond);
	}

private:
	// === 内部辅助函数 ===

	static int64_t DateToTicks(int32_t Year, int32_t Month, int32_t Day)
	{
		// 简化的日期转换实现，实际应用中需要更精确的算法
		int64_t days = Year * 365 + Month * 30 + Day; // 非常简化的实现
		return days * CTimespan::TicksPerDay;
	}

	static int64_t TimeToTicks(int32_t Hour, int32_t Minute, int32_t Second)
	{
		return Hour * CTimespan::TicksPerHour + Minute * CTimespan::TicksPerMinute + Second * CTimespan::TicksPerSecond;
	}

	static void TicksToDate(int64_t ticks, int32_t& Year, int32_t& Month, int32_t& Day)
	{
		// 简化的tick转日期实现
		int64_t days = ticks / CTimespan::TicksPerDay;
		Year = static_cast<int32_t>(days / 365);
		Month = static_cast<int32_t>((days % 365) / 30) + 1;
		Day = static_cast<int32_t>((days % 365) % 30) + 1;
	}

private:
	int64_t Ticks; // 时间tick数
};

// === 类型别名 ===
using FTimespan = CTimespan;
using FDateTime = CDateTime;

} // namespace NLib