#include "CLogger.h"

namespace NLib
{

std::shared_ptr<spdlog::logger> CLogger::LoggerInstance = nullptr;
bool CLogger::bInitialized = false;

void CLogger::Init()
{
	if (!bInitialized)
	{
		LoggerInstance = spdlog::stdout_color_mt("CLogger");
		LoggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
		bInitialized = true;
	}
}

std::shared_ptr<spdlog::logger>& CLogger::Get()
{
	if (!bInitialized)
	{
		Init();
	}
	return LoggerInstance;
}

void CLogger::SetLevel(CLogger::LogLevel level)
{
	Get()->set_level(static_cast<spdlog::level::level_enum>(level));
}

void CLogger::Info(const std::string& msg)
{
	Get()->info(msg);
}

void CLogger::Debug(const std::string& msg)
{
	Get()->debug(msg);
}

void CLogger::Warn(const std::string& msg)
{
	Get()->warn(msg);
}

void CLogger::Error(const std::string& msg)
{
	Get()->error(msg);
}

} // namespace NLib
