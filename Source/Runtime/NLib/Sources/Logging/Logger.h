#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

// spdlog headers
#include "spdlog/fmt/fmt.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace NLib
{
/**
 * @brief 日志级别枚举
 */
enum class ELogLevel : uint8_t
{
	Trace = 0,    // 追踪级别 - 最详细的日志
	Debug = 1,    // 调试级别 - 调试信息
	Info = 2,     // 信息级别 - 一般信息
	Warning = 3,  // 警告级别 - 警告信息
	Error = 4,    // 错误级别 - 错误信息
	Critical = 5, // 致命级别 - 致命错误
	Off = 6       // 关闭日志
};

/**
 * @brief 日志输出目标类型
 */
enum class ELogSinkType : uint8_t
{
	Console,      // 控制台输出
	File,         // 文件输出
	RotatingFile, // 循环文件输出
	DailyFile     // 按日期分割的文件输出
};

/**
 * @brief 日志配置结构
 */
struct SLoggerConfig
{
	ELogLevel Level = ELogLevel::Info;                           // 日志级别
	std::string Pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v"; // 日志格式
	std::string LoggerName = "NLib";                             // 日志器名称
	bool bFlushOnLog = false;                                    // 是否每次都刷新
	bool bAsyncLogging = false;                                  // 是否使用异步日志

	// 文件日志配置
	std::string FilePath = "logs/nlib.log"; // 日志文件路径
	size_t MaxFileSize = 1024 * 1024 * 10;  // 最大文件大小 (10MB)
	size_t MaxFiles = 5;                    // 最大文件数量
};

/**
 * @brief NLib统一日志系统
 *
 * 基于spdlog实现的高性能日志系统，提供：
 * - 多种输出目标（控制台、文件、循环文件等）
 * - 多种日志级别
 * - 异步日志支持
 * - 线程安全
 * - 格式化输出
 */
class NLogger
{
public:
	/**
	 * @brief 获取全局日志器实例
	 * @return 日志器实例引用
	 */
	static NLogger& GetInstance();

	// 禁用拷贝和赋值
	NLogger(const NLogger&) = delete;
	NLogger& operator=(const NLogger&) = delete;

public:
	// === 日志器配置 ===

	/**
	 * @brief 初始化日志系统
	 * @param Config 日志配置
	 */
	void Initialize(const SLoggerConfig& Config = SLoggerConfig{});

	/**
	 * @brief 关闭日志系统
	 */
	void Shutdown();

	/**
	 * @brief 检查日志系统是否已初始化
	 * @return true if initialized, false otherwise
	 */
	bool IsInitialized() const;

public:
	// === 日志输出目标管理 ===

	/**
	 * @brief 添加控制台输出
	 * @param bColorized 是否使用颜色
	 */
	void AddConsoleSink(bool bColorized = true);

	/**
	 * @brief 添加文件输出
	 * @param FilePath 文件路径
	 * @param bTruncate 是否截断现有文件
	 */
	void AddFileSink(const std::string& FilePath, bool bTruncate = false);

	/**
	 * @brief 添加循环文件输出
	 * @param FilePath 文件路径
	 * @param MaxFileSize 最大文件大小
	 * @param MaxFiles 最大文件数量
	 */
	void AddRotatingFileSink(const std::string& FilePath, size_t MaxFileSize, size_t MaxFiles);

	/**
	 * @brief 添加按日期分割的文件输出
	 * @param FilePath 文件路径
	 * @param Hour 分割时间（小时）
	 * @param Minute 分割时间（分钟）
	 */
	void AddDailySink(const std::string& FilePath, int Hour = 0, int Minute = 0);

public:
	// === 日志级别控制 ===

	/**
	 * @brief 设置日志级别
	 * @param Level 日志级别
	 */
	void SetLevel(ELogLevel Level);

	/**
	 * @brief 获取当前日志级别
	 * @return 当前日志级别
	 */
	ELogLevel GetLevel() const;

	/**
	 * @brief 检查指定级别是否应该记录
	 * @param Level 要检查的级别
	 * @return true if should log, false otherwise
	 */
	bool ShouldLog(ELogLevel Level) const;

public:
	// === 日志输出接口 ===

	/**
	 * @brief 记录追踪级别日志
	 * @param Message 日志消息
	 */
	void Trace(const std::string& Message);

	/**
	 * @brief 记录调试级别日志
	 * @param Message 日志消息
	 */
	void Debug(const std::string& Message);

	/**
	 * @brief 记录信息级别日志
	 * @param Message 日志消息
	 */
	void Info(const std::string& Message);

	/**
	 * @brief 记录警告级别日志
	 * @param Message 日志消息
	 */
	void Warning(const std::string& Message);

	/**
	 * @brief 记录错误级别日志
	 * @param Message 日志消息
	 */
	void Error(const std::string& Message);

	/**
	 * @brief 记录致命级别日志
	 * @param Message 日志消息
	 */
	void Critical(const std::string& Message);

public:
	// === 格式化日志接口 ===

	/**
	 * @brief 格式化追踪日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void TraceF(const std::string& Format, TArgs&&... Args);

	/**
	 * @brief 格式化调试日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void DebugF(const std::string& Format, TArgs&&... Args);

	/**
	 * @brief 格式化信息日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void InfoF(const std::string& Format, TArgs&&... Args);

	/**
	 * @brief 格式化警告日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void WarningF(const std::string& Format, TArgs&&... Args);

	/**
	 * @brief 格式化错误日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void ErrorF(const std::string& Format, TArgs&&... Args);

	/**
	 * @brief 格式化致命日志
	 * @tparam TArgs 参数类型
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void CriticalF(const std::string& Format, TArgs&&... Args);

public:
	// === 通用日志接口 ===

	/**
	 * @brief 记录指定级别的日志
	 * @param Level 日志级别
	 * @param Message 日志消息
	 */
	void Log(ELogLevel Level, const std::string& Message);

	/**
	 * @brief 格式化记录指定级别的日志
	 * @tparam TArgs 参数类型
	 * @param Level 日志级别
	 * @param Format 格式字符串
	 * @param Args 参数
	 */
	template <typename... TArgs>
	void LogF(ELogLevel Level, const std::string& Format, TArgs&&... Args);

public:
	// === 高级功能 ===

	/**
	 * @brief 立即刷新所有日志
	 */
	void Flush();

	/**
	 * @brief 设置日志格式
	 * @param Pattern 格式字符串
	 */
	void SetPattern(const std::string& Pattern);

	/**
	 * @brief 启用/禁用自动刷新
	 * @param bEnable 是否启用
	 */
	void SetAutoFlush(bool bEnable);

	/**
	 * @brief 获取底层spdlog日志器
	 * @return spdlog日志器指针
	 */
	std::shared_ptr<spdlog::logger> GetSpdlogger() const;

private:
	// === 构造函数 ===
	NLogger();
	~NLogger();

	// === 内部方法 ===
	spdlog::level::level_enum ConvertLogLevel(ELogLevel Level) const;
	ELogLevel ConvertSpdlogLevel(spdlog::level::level_enum Level) const;
	void CreateLogger();
	void EnsureDirectoryExists(const std::string& FilePath);

private:
	// === 成员变量 ===
	std::shared_ptr<spdlog::logger> Logger; // spdlog日志器
	std::vector<spdlog::sink_ptr> Sinks;    // 输出目标列表
	SLoggerConfig Config;                   // 日志配置
	std::atomic<bool> bInitialized;         // 是否已初始化
	mutable std::mutex LoggerMutex;         // 线程安全锁
};

// === 模板函数实现 ===

template <typename... TArgs>
void NLogger::TraceF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Trace))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->trace(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::DebugF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Debug))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->debug(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::InfoF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Info))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->info(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::WarningF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Warning))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->warn(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::ErrorF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Error))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->error(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::CriticalF(const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(ELogLevel::Critical))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->critical(fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

template <typename... TArgs>
void NLogger::LogF(ELogLevel Level, const std::string& Format, TArgs&&... Args)
{
	if (ShouldLog(Level))
	{
		std::lock_guard<std::mutex> Lock(LoggerMutex);
		if (Logger)
		{
			Logger->log(ConvertLogLevel(Level), fmt::runtime(Format), std::forward<TArgs>(Args)...);
		}
	}
}

} // namespace NLib