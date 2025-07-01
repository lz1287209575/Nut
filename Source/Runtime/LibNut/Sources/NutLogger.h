#pragma once
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class NutLogger {
public:
    static std::shared_ptr<spdlog::logger>& Get();
    static void SetLevel(spdlog::level::level_enum level);
    static void Info(const std::string& msg);
    static void Debug(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);
private:
    static void Init();
    static std::shared_ptr<spdlog::logger> LoggerInstance;
    static bool bInitialized;
}; 