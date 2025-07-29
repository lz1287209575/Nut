#pragma once

#include <memory>
#include <string>

enum class ELogLevel
{
    Debug,
    Info,
    Warn,
    Error
};

class FLogger
{
  public:
    FLogger();
    ~FLogger();

    void SetLogLevel(ELogLevel InLevel);
    void Debug(const std::string &Message);
    void Info(const std::string &Message);
    void Warn(const std::string &Message);
    void Error(const std::string &Message);

  private:
    void Log(ELogLevel Level, const std::string &Message);
    std::string GetCurrentTime();
    std::string LevelToString(ELogLevel Level);

    ELogLevel LogLevel;
};
