#include "NutLogger.h"

std::shared_ptr<spdlog::logger> NutLogger::LoggerInstance = nullptr;
bool NutLogger::bInitialized = false;

void NutLogger::Init() {
    if (!bInitialized) {
        LoggerInstance = spdlog::stdout_color_mt("NutLogger");
        LoggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        bInitialized = true;
    }
}

std::shared_ptr<spdlog::logger>& NutLogger::Get() {
    if (!bInitialized) {
        Init();
    }
    return LoggerInstance;
}

void NutLogger::SetLevel(spdlog::level::level_enum level) {
    Get()->set_level(level);
}

void NutLogger::Info(const std::string& msg) {
    Get()->info(msg);
}

void NutLogger::Debug(const std::string& msg) {
    Get()->debug(msg);
}

void NutLogger::Warn(const std::string& msg) {
    Get()->warn(msg);
}

void NutLogger::Error(const std::string& msg) {
    Get()->error(msg);
} 