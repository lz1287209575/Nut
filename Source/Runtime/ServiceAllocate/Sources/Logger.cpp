#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

FLogger::FLogger() : LogLevel(ELogLevel::Info) {
}

FLogger::~FLogger() {
}

void FLogger::SetLogLevel(ELogLevel InLevel) {
    LogLevel = InLevel;
}

void FLogger::Debug(const std::string& Message) {
    Log(ELogLevel::Debug, Message);
}

void FLogger::Info(const std::string& Message) {
    Log(ELogLevel::Info, Message);
}

void FLogger::Warn(const std::string& Message) {
    Log(ELogLevel::Warn, Message);
}

void FLogger::Error(const std::string& Message) {
    Log(ELogLevel::Error, Message);
}

void FLogger::Log(ELogLevel Level, const std::string& Message) {
    if (Level < LogLevel) {
        return;
    }
    std::string Timestamp = GetCurrentTime();
    std::string LevelStr = LevelToString(Level);
    std::cout << "[" << Timestamp << "] [" << LevelStr << "] " << Message << std::endl;
}

std::string FLogger::GetCurrentTime() {
    auto Now = std::chrono::system_clock::now();
    auto TimeT = std::chrono::system_clock::to_time_t(Now);
    auto Ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        Now.time_since_epoch()) % 1000;
    std::stringstream Ss;
    Ss << std::put_time(std::localtime(&TimeT), "%Y-%m-%d %H:%M:%S");
    Ss << '.' << std::setfill('0') << std::setw(3) << Ms.count();
    return Ss.str();
}

std::string FLogger::LevelToString(ELogLevel Level) {
    switch (Level) {
        case ELogLevel::Debug: return "DEBUG";
        case ELogLevel::Info:  return "INFO ";
        case ELogLevel::Warn:  return "WARN ";
        case ELogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
} 