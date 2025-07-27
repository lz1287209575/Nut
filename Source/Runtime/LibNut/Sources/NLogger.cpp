#include "NLogger.h"

std::shared_ptr<spdlog::logger> NLogger::LoggerInstance = nullptr;
bool NLogger::bInitialized = false;

void NLogger::Init() {
    if (!bInitialized) {
        LoggerInstance = spdlog::stdout_color_mt("NLogger");
        LoggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        bInitialized = true;
    }
}

std::shared_ptr<spdlog::logger>& NLogger::Get() {
    if (!bInitialized) {
        Init();
    }
    return LoggerInstance;
}

void NLogger::SetLevel(spdlog::level::level_enum level) {
    Get()->set_level(level);
}

void NLogger::Info(const std::string& msg) {
    Get()->info(msg);
}

void NLogger::Debug(const std::string& msg) {
    Get()->debug(msg);
}

void NLogger::Warn(const std::string& msg) {
    Get()->warn(msg);
}

void NLogger::Error(const std::string& msg) {
    Get()->error(msg);
} 