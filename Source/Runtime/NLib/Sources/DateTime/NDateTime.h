#pragma once

#include "Containers/CString.h"
#include "Core/CObject.h"

#include <chrono>
#include <ctime>

namespace NLib
{

/**
 * @brief 时间跨度类 - 表示时间间隔
 */
NCLASS()
class LIBNLIB_API NTimespan : public CObject
{
	GENERATED_BODY()

public:
	NTimespan();
	explicit NTimespan(int64_t Ticks);
	NTimespan(int32_t Hours, int32_t Minutes, int32_t Seconds);
	NTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds);
	NTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds);

	// 静态工厂方法
	static NTimespan FromDays(double Days);
	static NTimespan FromHours(double Hours);
	static NTimespan FromMinutes(double Minutes);
	static NTimespan FromSeconds(double Seconds);
	static NTimespan FromMilliseconds(double Milliseconds);
	static NTimespan FromMicroseconds(double Microseconds);
	static NTimespan FromTicks(int64_t Ticks);

	// 访问器
	int64_t GetTicks() const
	{
		return Ticks;
	}
	int32_t GetDays() const;
	int32_t GetHours() const;
	int32_t GetMinutes() const;
	int32_t GetSeconds() const;
	int32_t GetMilliseconds() const;

	double GetTotalDays() const;
	double GetTotalHours() const;
	double GetTotalMinutes() const;
	double GetTotalSeconds() const;
	double GetTotalMilliseconds() const;

	// 操作符
	NTimespan operator+(const NTimespan& Other) const;
	NTimespan operator-(const NTimespan& Other) const;
	NTimespan operator*(double Scalar) const;
	NTimespan operator/(double Scalar) const;

	NTimespan& operator+=(const NTimespan& Other);
	NTimespan& operator-=(const NTimespan& Other);
	NTimespan& operator*=(double Scalar);
	NTimespan& operator/=(double Scalar);

	bool operator==(const NTimespan& Other) const;
	bool operator!=(const NTimespan& Other) const;
	bool operator<(const NTimespan& Other) const;
	bool operator<=(const NTimespan& Other) const;
	bool operator>(const NTimespan& Other) const;
	bool operator>=(const NTimespan& Other) const;

	// 工具方法
	NTimespan GetDuration() const; // 绝对值
	NTimespan Negate() const;      // 取反
	bool IsZero() const;
	bool IsNegative() const;
	bool IsPositive() const;

	virtual CString ToString() const override;
	CString ToString(const CString& Format) const;

	// 常量
	static const NTimespan Zero;
	static const NTimespan MinValue;
	static const NTimespan MaxValue;

private:
	int64_t Ticks; // 以100纳秒为单位

	static constexpr int64_t TicksPerMicrosecond = 10;
	static constexpr int64_t TicksPerMillisecond = 10000;
	static constexpr int64_t TicksPerSecond = 10000000;
	static constexpr int64_t TicksPerMinute = 600000000;
	static constexpr int64_t TicksPerHour = 36000000000;
	static constexpr int64_t TicksPerDay = 864000000000;
};

/**
 * @brief 日期时间类 - 表示特定的时间点
 */
class LIBNLIB_API NDateTime : public CObject
{
	NCLASS()
class NDateTime : public NObject
{
    GENERATED_BODY()

public:
	enum class EDateTimeKind
	{
		Unspecified, // 未指定时区
		Utc,         // UTC时间
		Local        // 本地时间
	};

	// 构造函数
	NDateTime();
	explicit NDateTime(int64_t Ticks, EDateTimeKind Kind = EDateTimeKind::Unspecified);
	NDateTime(int32_t Year, int32_t Month, int32_t Day);
	NDateTime(int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Minute, int32_t Second);
	NDateTime(
	    int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Minute, int32_t Second, int32_t Millisecond);

	// 静态工厂方法
	static NDateTime Now();
	static NDateTime UtcNow();
	static NDateTime Today();
	static NDateTime FromUnixTimestamp(int64_t UnixTimestamp);
	static NDateTime FromFileTime(int64_t FileTime);

	// 访问器
	int64_t GetTicks() const
	{
		return Ticks;
	}
	EDateTimeKind GetKind() const
	{
		return Kind;
	}

	int32_t GetYear() const;
	int32_t GetMonth() const;
	int32_t GetDay() const;
	int32_t GetHour() const;
	int32_t GetMinute() const;
	int32_t GetSecond() const;
	int32_t GetMillisecond() const;
	int32_t GetDayOfWeek() const; // 0=Sunday, 1=Monday, etc.
	int32_t GetDayOfYear() const;

	NDateTime GetDate() const;      // 只保留日期部分
	NTimespan GetTimeOfDay() const; // 只保留时间部分

	// 转换
	int64_t ToUnixTimestamp() const;
	int64_t ToFileTime() const;
	NDateTime ToLocalTime() const;
	NDateTime ToUniversalTime() const;

	// 操作符
	NDateTime operator+(const NTimespan& Timespan) const;
	NDateTime operator-(const NTimespan& Timespan) const;
	NTimespan operator-(const NDateTime& Other) const;

	NDateTime& operator+=(const NTimespan& Timespan);
	NDateTime& operator-=(const NTimespan& Timespan);

	bool operator==(const NDateTime& Other) const;
	bool operator!=(const NDateTime& Other) const;
	bool operator<(const NDateTime& Other) const;
	bool operator<=(const NDateTime& Other) const;
	bool operator>(const NDateTime& Other) const;
	bool operator>=(const NDateTime& Other) const;

	// 工具方法
	NDateTime AddDays(double Days) const;
	NDateTime AddHours(double Hours) const;
	NDateTime AddMinutes(double Minutes) const;
	NDateTime AddSeconds(double Seconds) const;
	NDateTime AddMilliseconds(double Milliseconds) const;
	NDateTime AddMonths(int32_t Months) const;
	NDateTime AddYears(int32_t Years) const;

	bool IsLeapYear() const;
	static bool IsLeapYear(int32_t Year);
	static int32_t DaysInMonth(int32_t Year, int32_t Month);

	virtual CString ToString() const override;
	CString ToString(const CString& Format) const;
	static NDateTime Parse(const CString& DateTimeString);
	static bool TryParse(const CString& DateTimeString, NDateTime& OutDateTime);

	// 常量
	static const NDateTime MinValue;
	static const NDateTime MaxValue;
	static const NDateTime UnixEpoch;

private:
	int64_t Ticks; // 以100纳秒为单位，从公元1年1月1日开始
	EDateTimeKind Kind;

	void ValidateRange() const;
	static int64_t DateToTicks(int32_t Year, int32_t Month, int32_t Day);
	static int64_t TimeToTicks(int32_t Hour, int32_t Minute, int32_t Second, int32_t Millisecond = 0);

	static constexpr int64_t TicksPerMillisecond = 10000;
	static constexpr int64_t TicksPerSecond = 10000000;
	static constexpr int64_t TicksPerMinute = 600000000;
	static constexpr int64_t TicksPerHour = 36000000000;
	static constexpr int64_t TicksPerDay = 864000000000;

	static constexpr int64_t MinTicks = 0;
	static constexpr int64_t MaxTicks = 3155378975999999999;      // 9999年12月31日 23:59:59.9999999
	static constexpr int64_t UnixEpochTicks = 621355968000000000; // 1970年1月1日
};

/**
 * @brief 高精度计时器 - 用于性能测量
 */
class LIBNLIB_API NStopwatch : public CObject
{
	NCLASS()
class NStopwatch : public NObject
{
    GENERATED_BODY()

public:
	NStopwatch();

	// 控制方法
	void Start();
	void Stop();
	void Reset();
	void Restart(); // Reset + Start

	// 状态查询
	bool IsRunning() const
	{
		return bIsRunning;
	}

	// 时间获取
	NTimespan GetElapsed() const;
	int64_t GetElapsedTicks() const;
	int64_t GetElapsedMilliseconds() const;
	int64_t GetElapsedMicroseconds() const;

	// 静态工厂方法
	static NStopwatch StartNew();

	// 系统频率查询
	static int64_t GetFrequency();
	static bool IsHighResolution();

	virtual CString ToString() const override;

private:
	std::chrono::high_resolution_clock::time_point StartTime;
	std::chrono::high_resolution_clock::time_point StopTime;
	bool bIsRunning;

	int64_t GetCurrentTicks() const;
};

} // namespace NLib
