#include "NDateTime.h"
#include "NDateTime.generate.h"

#include "Containers/CString.h"
#include "Logging/CLogger.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace NLib
{

// === NTimespan 实现 ===

// 静态常量定义
const NTimespan NTimespan::Zero = NTimespan(0);
const NTimespan NTimespan::MinValue = NTimespan(INT64_MIN);
const NTimespan NTimespan::MaxValue = NTimespan(INT64_MAX);

NTimespan::NTimespan()
    : Ticks(0)
{}

NTimespan::NTimespan(int64_t InTicks)
    : Ticks(InTicks)
{}

NTimespan::NTimespan(int32_t Hours, int32_t Minutes, int32_t Seconds)
    : Ticks(Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond)
{}

NTimespan::NTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds)
    : Ticks(Days * TicksPerDay + Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond)
{}

NTimespan::NTimespan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds)
    : Ticks(Days * TicksPerDay + Hours * TicksPerHour + Minutes * TicksPerMinute + Seconds * TicksPerSecond +
            Milliseconds * TicksPerMillisecond)
{}

// 静态工厂方法
NTimespan NTimespan::FromDays(double Days)
{
	return NTimespan(static_cast<int64_t>(Days * TicksPerDay));
}

NTimespan NTimespan::FromHours(double Hours)
{
	return NTimespan(static_cast<int64_t>(Hours * TicksPerHour));
}

NTimespan NTimespan::FromMinutes(double Minutes)
{
	return NTimespan(static_cast<int64_t>(Minutes * TicksPerMinute));
}

NTimespan NTimespan::FromSeconds(double Seconds)
{
	return NTimespan(static_cast<int64_t>(Seconds * TicksPerSecond));
}

NTimespan NTimespan::FromMilliseconds(double Milliseconds)
{
	return NTimespan(static_cast<int64_t>(Milliseconds * TicksPerMillisecond));
}

NTimespan NTimespan::FromMicroseconds(double Microseconds)
{
	return NTimespan(static_cast<int64_t>(Microseconds * TicksPerMicrosecond));
}

NTimespan NTimespan::FromTicks(int64_t InTicks)
{
	return NTimespan(InTicks);
}

// 访问器
int32_t NTimespan::GetDays() const
{
	return static_cast<int32_t>(Ticks / TicksPerDay);
}

int32_t NTimespan::GetHours() const
{
	return static_cast<int32_t>((Ticks / TicksPerHour) % 24);
}

int32_t NTimespan::GetMinutes() const
{
	return static_cast<int32_t>((Ticks / TicksPerMinute) % 60);
}

int32_t NTimespan::GetSeconds() const
{
	return static_cast<int32_t>((Ticks / TicksPerSecond) % 60);
}

int32_t NTimespan::GetMilliseconds() const
{
	return static_cast<int32_t>((Ticks / TicksPerMillisecond) % 1000);
}

double NTimespan::GetTotalDays() const
{
	return static_cast<double>(Ticks) / TicksPerDay;
}

double NTimespan::GetTotalHours() const
{
	return static_cast<double>(Ticks) / TicksPerHour;
}

double NTimespan::GetTotalMinutes() const
{
	return static_cast<double>(Ticks) / TicksPerMinute;
}

double NTimespan::GetTotalSeconds() const
{
	return static_cast<double>(Ticks) / TicksPerSecond;
}

double NTimespan::GetTotalMilliseconds() const
{
	return static_cast<double>(Ticks) / TicksPerMillisecond;
}

// 操作符
NTimespan NTimespan::operator+(const NTimespan& Other) const
{
	return NTimespan(Ticks + Other.Ticks);
}

NTimespan NTimespan::operator-(const NTimespan& Other) const
{
	return NTimespan(Ticks - Other.Ticks);
}

NTimespan NTimespan::operator*(double Scalar) const
{
	return NTimespan(static_cast<int64_t>(Ticks * Scalar));
}

NTimespan NTimespan::operator/(double Scalar) const
{
	if (Scalar == 0.0)
	{
		CLogger::Error("NTimespan::operator/: Division by zero");
		return Zero;
	}
	return NTimespan(static_cast<int64_t>(Ticks / Scalar));
}

NTimespan& NTimespan::operator+=(const NTimespan& Other)
{
	Ticks += Other.Ticks;
	return *this;
}

NTimespan& NTimespan::operator-=(const NTimespan& Other)
{
	Ticks -= Other.Ticks;
	return *this;
}

NTimespan& NTimespan::operator*=(double Scalar)
{
	Ticks = static_cast<int64_t>(Ticks * Scalar);
	return *this;
}

NTimespan& NTimespan::operator/=(double Scalar)
{
	if (Scalar == 0.0)
	{
		CLogger::Error("NTimespan::operator/=: Division by zero");
		return *this;
	}
	Ticks = static_cast<int64_t>(Ticks / Scalar);
	return *this;
}

bool NTimespan::operator==(const NTimespan& Other) const
{
	return Ticks == Other.Ticks;
}

bool NTimespan::operator!=(const NTimespan& Other) const
{
	return Ticks != Other.Ticks;
}

bool NTimespan::operator<(const NTimespan& Other) const
{
	return Ticks < Other.Ticks;
}

bool NTimespan::operator<=(const NTimespan& Other) const
{
	return Ticks <= Other.Ticks;
}

bool NTimespan::operator>(const NTimespan& Other) const
{
	return Ticks > Other.Ticks;
}

bool NTimespan::operator>=(const NTimespan& Other) const
{
	return Ticks >= Other.Ticks;
}

// 工具方法
NTimespan NTimespan::GetDuration() const
{
	return NTimespan(Ticks < 0 ? -Ticks : Ticks);
}

NTimespan NTimespan::Negate() const
{
	return NTimespan(-Ticks);
}

bool NTimespan::IsZero() const
{
	return Ticks == 0;
}

bool NTimespan::IsNegative() const
{
	return Ticks < 0;
}

bool NTimespan::IsPositive() const
{
	return Ticks > 0;
}

CString NTimespan::ToString() const
{
	if (IsZero())
	{
		return CString("00:00:00");
	}

	bool bNegative = IsNegative();
	int64_t AbsTicks = bNegative ? -Ticks : Ticks;

	int32_t Days = static_cast<int32_t>(AbsTicks / TicksPerDay);
	int32_t Hours = static_cast<int32_t>((AbsTicks / TicksPerHour) % 24);
	int32_t Minutes = static_cast<int32_t>((AbsTicks / TicksPerMinute) % 60);
	int32_t Seconds = static_cast<int32_t>((AbsTicks / TicksPerSecond) % 60);
	int32_t Milliseconds = static_cast<int32_t>((AbsTicks / TicksPerMillisecond) % 1000);

	std::ostringstream OSS;
	if (bNegative)
	{
		OSS << "-";
	}

	if (Days > 0)
	{
		OSS << Days << ".";
	}

	OSS << std::setfill('0') << std::setw(2) << Hours << ":" << std::setfill('0') << std::setw(2) << Minutes << ":"
	    << std::setfill('0') << std::setw(2) << Seconds;

	if (Milliseconds > 0)
	{
		OSS << "." << std::setfill('0') << std::setw(3) << Milliseconds;
	}

	return CString(OSS.str().c_str());
}

CString NTimespan::ToString(const CString& Format) const
{
	// 简化格式支持，可以后续扩展
	if (Format == CString("c"))
	{
		return ToString(); // 默认格式
	}
	else if (Format == CString("g"))
	{
		// 简短格式
		return ToString();
	}

	return ToString();
}

// === NDateTime 实现 ===

// 静态常量定义
const NDateTime NDateTime::MinValue = NDateTime(MinTicks, EDateTimeKind::Unspecified);
const NDateTime NDateTime::MaxValue = NDateTime(MaxTicks, EDateTimeKind::Unspecified);
const NDateTime NDateTime::UnixEpoch = NDateTime(UnixEpochTicks, EDateTimeKind::Utc);

NDateTime::NDateTime()
    : Ticks(0),
      Kind(EDateTimeKind::Unspecified)
{}

NDateTime::NDateTime(int64_t InTicks, EDateTimeKind InKind)
    : Ticks(InTicks),
      Kind(InKind)
{
	ValidateRange();
}

NDateTime::NDateTime(int32_t Year, int32_t Month, int32_t Day)
    : Ticks(DateToTicks(Year, Month, Day)),
      Kind(EDateTimeKind::Unspecified)
{
	ValidateRange();
}

NDateTime::NDateTime(int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Minute, int32_t Second)
    : Ticks(DateToTicks(Year, Month, Day) + TimeToTicks(Hour, Minute, Second)),
      Kind(EDateTimeKind::Unspecified)
{
	ValidateRange();
}

NDateTime::NDateTime(
    int32_t Year, int32_t Month, int32_t Day, int32_t Hour, int32_t Minute, int32_t Second, int32_t Millisecond)
    : Ticks(DateToTicks(Year, Month, Day) + TimeToTicks(Hour, Minute, Second, Millisecond)),
      Kind(EDateTimeKind::Unspecified)
{
	ValidateRange();
}

// 静态工厂方法
NDateTime NDateTime::Now()
{
	auto Now = std::chrono::system_clock::now();
	auto Duration = Now.time_since_epoch();
	auto UnixTicks = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration).count() / 100; // 转换为100纳秒单位

	return NDateTime(UnixEpochTicks + UnixTicks, EDateTimeKind::Local);
}

NDateTime NDateTime::UtcNow()
{
	auto Now = std::chrono::system_clock::now();
	auto Duration = Now.time_since_epoch();
	auto UnixTicks = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration).count() / 100;

	return NDateTime(UnixEpochTicks + UnixTicks, EDateTimeKind::Utc);
}

NDateTime NDateTime::Today()
{
	NDateTime Now = NDateTime::Now();
	return Now.GetDate();
}

NDateTime NDateTime::FromUnixTimestamp(int64_t UnixTimestamp)
{
	return NDateTime(UnixEpochTicks + UnixTimestamp * TicksPerSecond, EDateTimeKind::Utc);
}

NDateTime NDateTime::FromFileTime(int64_t FileTime)
{
	// Windows FILETIME is 100-nanosecond intervals since January 1, 1601
	constexpr int64_t FileTimeEpochTicks = 504911232000000000; // 1601年1月1日到1年1月1日的Ticks差
	return NDateTime(FileTime + FileTimeEpochTicks, EDateTimeKind::Utc);
}

// 访问器实现（简化版本，实际需要更复杂的日期计算）
int32_t NDateTime::GetYear() const
{
	// 简化实现，实际需要更精确的算法
	auto TimePoint = std::chrono::system_clock::from_time_t((Ticks - UnixEpochTicks) / TicksPerSecond);
	auto TimeT = std::chrono::system_clock::to_time_t(TimePoint);
	auto* TM = std::gmtime(&TimeT);
	return TM ? TM->tm_year + 1900 : 1970;
}

int32_t NDateTime::GetMonth() const
{
	auto TimePoint = std::chrono::system_clock::from_time_t((Ticks - UnixEpochTicks) / TicksPerSecond);
	auto TimeT = std::chrono::system_clock::to_time_t(TimePoint);
	auto* TM = std::gmtime(&TimeT);
	return TM ? TM->tm_mon + 1 : 1;
}

int32_t NDateTime::GetDay() const
{
	auto TimePoint = std::chrono::system_clock::from_time_t((Ticks - UnixEpochTicks) / TicksPerSecond);
	auto TimeT = std::chrono::system_clock::to_time_t(TimePoint);
	auto* TM = std::gmtime(&TimeT);
	return TM ? TM->tm_mday : 1;
}

int32_t NDateTime::GetHour() const
{
	return static_cast<int32_t>((Ticks / TicksPerHour) % 24);
}

int32_t NDateTime::GetMinute() const
{
	return static_cast<int32_t>((Ticks / TicksPerMinute) % 60);
}

int32_t NDateTime::GetSecond() const
{
	return static_cast<int32_t>((Ticks / TicksPerSecond) % 60);
}

int32_t NDateTime::GetMillisecond() const
{
	return static_cast<int32_t>((Ticks / TicksPerMillisecond) % 1000);
}

int32_t NDateTime::GetDayOfWeek() const
{
	auto TimePoint = std::chrono::system_clock::from_time_t((Ticks - UnixEpochTicks) / TicksPerSecond);
	auto TimeT = std::chrono::system_clock::to_time_t(TimePoint);
	auto* TM = std::gmtime(&TimeT);
	return TM ? TM->tm_wday : 0;
}

int32_t NDateTime::GetDayOfYear() const
{
	auto TimePoint = std::chrono::system_clock::from_time_t((Ticks - UnixEpochTicks) / TicksPerSecond);
	auto TimeT = std::chrono::system_clock::to_time_t(TimePoint);
	auto* TM = std::gmtime(&TimeT);
	return TM ? TM->tm_yday + 1 : 1;
}

NDateTime NDateTime::GetDate() const
{
	int64_t DateTicks = (Ticks / TicksPerDay) * TicksPerDay;
	return NDateTime(DateTicks, Kind);
}

NTimespan NDateTime::GetTimeOfDay() const
{
	return NTimespan(Ticks % TicksPerDay);
}

// 转换
int64_t NDateTime::ToUnixTimestamp() const
{
	return (Ticks - UnixEpochTicks) / TicksPerSecond;
}

int64_t NDateTime::ToFileTime() const
{
	constexpr int64_t FileTimeEpochTicks = 504911232000000000;
	return Ticks - FileTimeEpochTicks;
}

NDateTime NDateTime::ToLocalTime() const
{
	if (Kind == EDateTimeKind::Local)
	{
		return *this;
	}

	// 简化实现，实际需要时区转换
	return NDateTime(Ticks, EDateTimeKind::Local);
}

NDateTime NDateTime::ToUniversalTime() const
{
	if (Kind == EDateTimeKind::Utc)
	{
		return *this;
	}

	// 简化实现，实际需要时区转换
	return NDateTime(Ticks, EDateTimeKind::Utc);
}

// 操作符
NDateTime NDateTime::operator+(const NTimespan& Timespan) const
{
	return NDateTime(Ticks + Timespan.GetTicks(), Kind);
}

NDateTime NDateTime::operator-(const NTimespan& Timespan) const
{
	return NDateTime(Ticks - Timespan.GetTicks(), Kind);
}

NTimespan NDateTime::operator-(const NDateTime& Other) const
{
	return NTimespan(Ticks - Other.Ticks);
}

NDateTime& NDateTime::operator+=(const NTimespan& Timespan)
{
	Ticks += Timespan.GetTicks();
	ValidateRange();
	return *this;
}

NDateTime& NDateTime::operator-=(const NTimespan& Timespan)
{
	Ticks -= Timespan.GetTicks();
	ValidateRange();
	return *this;
}

bool NDateTime::operator==(const NDateTime& Other) const
{
	return Ticks == Other.Ticks;
}

bool NDateTime::operator!=(const NDateTime& Other) const
{
	return Ticks != Other.Ticks;
}

bool NDateTime::operator<(const NDateTime& Other) const
{
	return Ticks < Other.Ticks;
}

bool NDateTime::operator<=(const NDateTime& Other) const
{
	return Ticks <= Other.Ticks;
}

bool NDateTime::operator>(const NDateTime& Other) const
{
	return Ticks > Other.Ticks;
}

bool NDateTime::operator>=(const NDateTime& Other) const
{
	return Ticks >= Other.Ticks;
}

// 工具方法
NDateTime NDateTime::AddDays(double Days) const
{
	return *this + NTimespan::FromDays(Days);
}

NDateTime NDateTime::AddHours(double Hours) const
{
	return *this + NTimespan::FromHours(Hours);
}

NDateTime NDateTime::AddMinutes(double Minutes) const
{
	return *this + NTimespan::FromMinutes(Minutes);
}

NDateTime NDateTime::AddSeconds(double Seconds) const
{
	return *this + NTimespan::FromSeconds(Seconds);
}

NDateTime NDateTime::AddMilliseconds(double Milliseconds) const
{
	return *this + NTimespan::FromMilliseconds(Milliseconds);
}

NDateTime NDateTime::AddMonths(int32_t Months) const
{
	// 简化实现，实际需要更复杂的月份计算
	int32_t Year = GetYear();
	int32_t Month = GetMonth() + Months;

	while (Month > 12)
	{
		Month -= 12;
		Year++;
	}
	while (Month < 1)
	{
		Month += 12;
		Year--;
	}

	int32_t Day = GetDay();
	int32_t MaxDays = DaysInMonth(Year, Month);
	if (Day > MaxDays)
	{
		Day = MaxDays;
	}

	return NDateTime(Year, Month, Day, GetHour(), GetMinute(), GetSecond(), GetMillisecond());
}

NDateTime NDateTime::AddYears(int32_t Years) const
{
	return AddMonths(Years * 12);
}

bool NDateTime::IsLeapYear() const
{
	return IsLeapYear(GetYear());
}

bool NDateTime::IsLeapYear(int32_t Year)
{
	return (Year % 4 == 0 && Year % 100 != 0) || (Year % 400 == 0);
}

int32_t NDateTime::DaysInMonth(int32_t Year, int32_t Month)
{
	static const int32_t DaysPerMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (Month < 1 || Month > 12)
	{
		return 0;
	}

	if (Month == 2 && IsLeapYear(Year))
	{
		return 29;
	}

	return DaysPerMonth[Month];
}

CString NDateTime::ToString() const
{
	std::ostringstream OSS;
	OSS << std::setfill('0') << std::setw(4) << GetYear() << "-" << std::setfill('0') << std::setw(2) << GetMonth()
	    << "-" << std::setfill('0') << std::setw(2) << GetDay() << " " << std::setfill('0') << std::setw(2) << GetHour()
	    << ":" << std::setfill('0') << std::setw(2) << GetMinute() << ":" << std::setfill('0') << std::setw(2)
	    << GetSecond();

	return CString(OSS.str().c_str());
}

CString NDateTime::ToString(const CString& Format) const
{
	// 简化格式支持
	if (Format == CString("yyyy-MM-dd"))
	{
		std::ostringstream OSS;
		OSS << std::setfill('0') << std::setw(4) << GetYear() << "-" << std::setfill('0') << std::setw(2) << GetMonth()
		    << "-" << std::setfill('0') << std::setw(2) << GetDay();
		return CString(OSS.str().c_str());
	}
	else if (Format == CString("HH:mm:ss"))
	{
		std::ostringstream OSS;
		OSS << std::setfill('0') << std::setw(2) << GetHour() << ":" << std::setfill('0') << std::setw(2) << GetMinute()
		    << ":" << std::setfill('0') << std::setw(2) << GetSecond();
		return CString(OSS.str().c_str());
	}

	return ToString();
}

NDateTime NDateTime::Parse(const CString& DateTimeString)
{
	NDateTime Result;
	if (!TryParse(DateTimeString, Result))
	{
		CLogger::Error("NDateTime::Parse: Invalid datetime string: {}", DateTimeString.GetCStr());
		return MinValue;
	}
	return Result;
}

bool NDateTime::TryParse(const CString& DateTimeString, NDateTime& OutDateTime)
{
	// 简化解析实现
	// 实际需要支持多种格式
	const char* Str = DateTimeString.GetCStr();
	int32_t Year, Month, Day, Hour = 0, Minute = 0, Second = 0;

	// 尝试解析 "YYYY-MM-DD HH:MM:SS" 格式
	int32_t Parsed = sscanf(Str, "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);

	if (Parsed >= 3)
	{
		try
		{
			OutDateTime = NDateTime(Year, Month, Day, Hour, Minute, Second);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	return false;
}

// 私有方法
void NDateTime::ValidateRange() const
{
	if (Ticks < MinTicks || Ticks > MaxTicks)
	{
		CLogger::Error("NDateTime: Ticks value {} is out of valid range", Ticks);
	}
}

int64_t NDateTime::DateToTicks(int32_t Year, int32_t Month, int32_t Day)
{
	// 简化实现，实际需要更精确的日期计算
	// 这里使用近似计算，实际应该使用标准日期算法

	if (Year < 1 || Year > 9999 || Month < 1 || Month > 12 || Day < 1 || Day > 31)
	{
		CLogger::Error("NDateTime::DateToTicks: Invalid date parameters");
		return 0;
	}

	// 从公元1年开始计算的简化版本
	int64_t Days = 0;

	// 年份贡献（简化）
	Days += (Year - 1) * 365;
	Days += (Year - 1) / 4;   // 闰年
	Days -= (Year - 1) / 100; // 世纪年不是闰年
	Days += (Year - 1) / 400; // 400的倍数是闰年

	// 月份贡献
	static const int32_t DaysToMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	Days += DaysToMonth[Month - 1];

	// 如果是闰年且月份大于2月，增加一天
	if (Month > 2 && IsLeapYear(Year))
	{
		Days++;
	}

	// 日期贡献
	Days += Day - 1;

	return Days * TicksPerDay;
}

int64_t NDateTime::TimeToTicks(int32_t Hour, int32_t Minute, int32_t Second, int32_t Millisecond)
{
	if (Hour < 0 || Hour >= 24 || Minute < 0 || Minute >= 60 || Second < 0 || Second >= 60 || Millisecond < 0 ||
	    Millisecond >= 1000)
	{
		CLogger::Error("NDateTime::TimeToTicks: Invalid time parameters");
		return 0;
	}

	return Hour * TicksPerHour + Minute * TicksPerMinute + Second * TicksPerSecond + Millisecond * TicksPerMillisecond;
}

// === NStopwatch 实现 ===

NStopwatch::NStopwatch()
    : bIsRunning(false)
{}

void NStopwatch::Start()
{
	if (!bIsRunning)
	{
		StartTime = std::chrono::high_resolution_clock::now();
		bIsRunning = true;
	}
}

void NStopwatch::Stop()
{
	if (bIsRunning)
	{
		StopTime = std::chrono::high_resolution_clock::now();
		bIsRunning = false;
	}
}

void NStopwatch::Reset()
{
	StartTime = std::chrono::high_resolution_clock::time_point();
	StopTime = std::chrono::high_resolution_clock::time_point();
	bIsRunning = false;
}

void NStopwatch::Restart()
{
	Reset();
	Start();
}

NTimespan NStopwatch::GetElapsed() const
{
	auto EndTime = bIsRunning ? std::chrono::high_resolution_clock::now() : StopTime;
	auto Duration = EndTime - StartTime;
	auto Ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration).count() / 100; // 转换为100纳秒单位

	return NTimespan::FromTicks(Ticks);
}

int64_t NStopwatch::GetElapsedTicks() const
{
	return GetElapsed().GetTicks();
}

int64_t NStopwatch::GetElapsedMilliseconds() const
{
	return static_cast<int64_t>(GetElapsed().GetTotalMilliseconds());
}

int64_t NStopwatch::GetElapsedMicroseconds() const
{
	return GetElapsedTicks() / 10; // 100纳秒 -> 微秒
}

NStopwatch NStopwatch::StartNew()
{
	NStopwatch Stopwatch;
	Stopwatch.Start();
	return Stopwatch;
}

int64_t NStopwatch::GetFrequency()
{
	return 10000000; // 100纳秒单位，所以频率是10MHz
}

bool NStopwatch::IsHighResolution()
{
	return std::chrono::high_resolution_clock::is_steady;
}

CString NStopwatch::ToString() const
{
	return GetElapsed().ToString();
}

int64_t NStopwatch::GetCurrentTicks() const
{
	auto Now = std::chrono::high_resolution_clock::now();
	auto Duration = Now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(Duration).count() / 100;
}

} // namespace NLib