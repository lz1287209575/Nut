#pragma once

#include <string>
#include <memory>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    Logger();
    ~Logger();
    
    void SetLogLevel(LogLevel level);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);
    
private:
    void Log(LogLevel level, const std::string& message);
    std::string GetCurrentTime();
    std::string LevelToString(LogLevel level);
    
    LogLevel m_logLevel;
}; 