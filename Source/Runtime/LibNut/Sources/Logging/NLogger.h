#pragma once
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <string>

namespace Nut
{

class NLogger
{
  public:
    enum class LogLevel
    {
        Info = spdlog::level::level_enum::info,
        Debug = spdlog::level::level_enum::debug,
        Warn = spdlog::level::level_enum::warn,
        Error = spdlog::level::level_enum::err,
    };

    static std::shared_ptr<spdlog::logger> &Get();
    static void SetLevel(LogLevel level);
    static void Info(const std::string &msg);
    static void Debug(const std::string &msg);
    static void Warn(const std::string &msg);
    static void Error(const std::string &msg);
    
    // 格式化版本 - 使用 fmt::runtime 包装非常量字符串
    template<typename... Args>
    static void Info(const std::string &format, Args... args) {
        Get()->info(fmt::runtime(format), args...);
    }
    
    template<typename... Args>
    static void Debug(const std::string &format, Args... args) {
        Get()->debug(fmt::runtime(format), args...);
    }
    
    template<typename... Args>
    static void Warn(const std::string &format, Args... args) {
        Get()->warn(fmt::runtime(format), args...);
    }
    
    template<typename... Args>
    static void Error(const std::string &format, Args... args) {
        Get()->error(fmt::runtime(format), args...);
    }

  private:
    static void Init();
    static std::shared_ptr<spdlog::logger> LoggerInstance;
    static bool bInitialized;
};

} // namespace Nut
