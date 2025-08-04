#include "Logger.h"

#include "LogCategory.h"

#include <filesystem>
#include <iostream>

namespace NLib
{
// === NLogger 实现 ===

NLogger& NLogger::GetInstance()
{
	static NLogger Instance;
	return Instance;
}

NLogger::NLogger()
    : bInitialized(false)
{
	// 默认初始化
	Initialize();
}

NLogger::~NLogger()
{
	Shutdown();
}

// === 日志器配置 ===

void NLogger::Initialize(const SLoggerConfig& InConfig)
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	if (bInitialized.load())
	{
		Shutdown();
	}

	Config = InConfig;

	// 清空现有的sinks
	Sinks.clear();

	// 添加默认的控制台输出
	AddConsoleSink(true);

	// 如果配置了文件路径，添加文件输出
	if (!Config.FilePath.empty())
	{
		AddRotatingFileSink(Config.FilePath, Config.MaxFileSize, Config.MaxFiles);
	}

	// 创建日志器
	CreateLogger();

	bInitialized = true;

	// 初始化默认日志分类
	InitializeDefaultLogCategories();

	Info("NLogger initialized successfully");
}

void NLogger::Shutdown()
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	if (bInitialized.load())
	{
		Info("NLogger shutting down");

		if (Logger)
		{
			Logger->flush();
			Logger.reset();
		}

		Sinks.clear();
		bInitialized = false;
	}
}

bool NLogger::IsInitialized() const
{
	return bInitialized.load();
}

// === 日志输出目标管理 ===

void NLogger::AddConsoleSink(bool bColorized)
{
	try
	{
		spdlog::sink_ptr ConsoleSink;

		if (bColorized)
		{
			ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		}
		else
		{
			ConsoleSink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
		}

		ConsoleSink->set_level(ConvertLogLevel(Config.Level));
		ConsoleSink->set_pattern(Config.Pattern);

		Sinks.push_back(ConsoleSink);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to add console sink: " << e.what() << std::endl;
	}
}

void NLogger::AddFileSink(const std::string& FilePath, bool bTruncate)
{
	try
	{
		EnsureDirectoryExists(FilePath);

		auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(FilePath, bTruncate);
		FileSink->set_level(ConvertLogLevel(Config.Level));
		FileSink->set_pattern(Config.Pattern);

		Sinks.push_back(FileSink);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to add file sink: " << e.what() << std::endl;
	}
}

void NLogger::AddRotatingFileSink(const std::string& FilePath, size_t MaxFileSize, size_t MaxFiles)
{
	try
	{
		EnsureDirectoryExists(FilePath);

		auto RotatingSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(FilePath, MaxFileSize, MaxFiles);
		RotatingSink->set_level(ConvertLogLevel(Config.Level));
		RotatingSink->set_pattern(Config.Pattern);

		Sinks.push_back(RotatingSink);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to add rotating file sink: " << e.what() << std::endl;
	}
}

void NLogger::AddDailySink(const std::string& FilePath, int Hour, int Minute)
{
	try
	{
		EnsureDirectoryExists(FilePath);

		auto DailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(FilePath, Hour, Minute);
		DailySink->set_level(ConvertLogLevel(Config.Level));
		DailySink->set_pattern(Config.Pattern);

		Sinks.push_back(DailySink);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to add daily file sink: " << e.what() << std::endl;
	}
}

// === 日志级别控制 ===

void NLogger::SetLevel(ELogLevel Level)
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	Config.Level = Level;

	if (Logger)
	{
		Logger->set_level(ConvertLogLevel(Level));
	}

	// 更新所有sinks的级别
	for (auto& Sink : Sinks)
	{
		Sink->set_level(ConvertLogLevel(Level));
	}
}

ELogLevel NLogger::GetLevel() const
{
	return Config.Level;
}

bool NLogger::ShouldLog(ELogLevel Level) const
{
	return Level >= Config.Level && Level != ELogLevel::Off;
}

// === 日志输出接口 ===

void NLogger::Trace(const std::string& Message)
{
	Log(ELogLevel::Trace, Message);
}

void NLogger::Debug(const std::string& Message)
{
	Log(ELogLevel::Debug, Message);
}

void NLogger::Info(const std::string& Message)
{
	Log(ELogLevel::Info, Message);
}

void NLogger::Warning(const std::string& Message)
{
	Log(ELogLevel::Warning, Message);
}

void NLogger::Error(const std::string& Message)
{
	Log(ELogLevel::Error, Message);
}

void NLogger::Critical(const std::string& Message)
{
	Log(ELogLevel::Critical, Message);
}

// === 通用日志接口 ===

void NLogger::Log(ELogLevel Level, const std::string& Message)
{
	if (!ShouldLog(Level))
	{
		return;
	}

	std::lock_guard<std::mutex> Lock(LoggerMutex);

	if (Logger)
	{
		Logger->log(ConvertLogLevel(Level), Message);

		if (Config.bFlushOnLog)
		{
			Logger->flush();
		}
	}
}

// === 高级功能 ===

void NLogger::Flush()
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	if (Logger)
	{
		Logger->flush();
	}
}

void NLogger::SetPattern(const std::string& Pattern)
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	Config.Pattern = Pattern;

	if (Logger)
	{
		Logger->set_pattern(Pattern);
	}

	// 更新所有sinks的格式
	for (auto& Sink : Sinks)
	{
		Sink->set_pattern(Pattern);
	}
}

void NLogger::SetAutoFlush(bool bEnable)
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);

	Config.bFlushOnLog = bEnable;

	if (Logger)
	{
		if (bEnable)
		{
			Logger->flush_on(spdlog::level::trace); // 所有级别都刷新
		}
		else
		{
			Logger->flush_on(spdlog::level::off); // 不自动刷新
		}
	}
}

std::shared_ptr<spdlog::logger> NLogger::GetSpdlogger() const
{
	std::lock_guard<std::mutex> Lock(LoggerMutex);
	return Logger;
}

// === 内部方法 ===

spdlog::level::level_enum NLogger::ConvertLogLevel(ELogLevel Level) const
{
	switch (Level)
	{
	case ELogLevel::Trace:
		return spdlog::level::trace;
	case ELogLevel::Debug:
		return spdlog::level::debug;
	case ELogLevel::Info:
		return spdlog::level::info;
	case ELogLevel::Warning:
		return spdlog::level::warn;
	case ELogLevel::Error:
		return spdlog::level::err;
	case ELogLevel::Critical:
		return spdlog::level::critical;
	case ELogLevel::Off:
		return spdlog::level::off;
	default:
		return spdlog::level::info;
	}
}

ELogLevel NLogger::ConvertSpdlogLevel(spdlog::level::level_enum Level) const
{
	switch (Level)
	{
	case spdlog::level::trace:
		return ELogLevel::Trace;
	case spdlog::level::debug:
		return ELogLevel::Debug;
	case spdlog::level::info:
		return ELogLevel::Info;
	case spdlog::level::warn:
		return ELogLevel::Warning;
	case spdlog::level::err:
		return ELogLevel::Error;
	case spdlog::level::critical:
		return ELogLevel::Critical;
	case spdlog::level::off:
		return ELogLevel::Off;
	default:
		return ELogLevel::Info;
	}
}

void NLogger::CreateLogger()
{
	try
	{
		if (Sinks.empty())
		{
			std::cerr << "[NLogger] Warning: No sinks configured, adding default console sink" << std::endl;
			AddConsoleSink(true);
		}

		Logger = std::make_shared<spdlog::logger>(Config.LoggerName, Sinks.begin(), Sinks.end());
		Logger->set_level(ConvertLogLevel(Config.Level));
		Logger->set_pattern(Config.Pattern);

		// 设置刷新策略
		if (Config.bFlushOnLog)
		{
			Logger->flush_on(spdlog::level::trace);
		}

		// 注册到spdlog的全局注册表
		spdlog::register_logger(Logger);
		spdlog::set_default_logger(Logger);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to create logger: " << e.what() << std::endl;
	}
}

void NLogger::EnsureDirectoryExists(const std::string& FilePath)
{
	try
	{
		std::filesystem::path Path(FilePath);
		std::filesystem::path Directory = Path.parent_path();

		if (!Directory.empty() && !std::filesystem::exists(Directory))
		{
			std::filesystem::create_directories(Directory);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[NLogger] Failed to create directory for log file: " << e.what() << std::endl;
	}
}

} // namespace NLib