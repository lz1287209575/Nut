#include "ConfigManager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

FConfigManager::FConfigManager() : ListenPort(50052)
{
}

FConfigManager::~FConfigManager()
{
}

bool FConfigManager::LoadConfig(const std::string &ConfigPath)
{
    std::ifstream File(ConfigPath);
    if (!File.is_open())
    {
        std::cerr << "无法打开配置文件: " << ConfigPath << std::endl;
        return false;
    }
    std::string Line;
    while (std::getline(File, Line))
    {
        size_t Pos = Line.find(':');
        if (Pos != std::string::npos)
        {
            std::string Key = Line.substr(0, Pos);
            std::string Value = Line.substr(Pos + 1);
            Key.erase(0, Key.find_first_not_of(" \t\""));
            Key.erase(Key.find_last_not_of(" \t\",") + 1);
            Value.erase(0, Value.find_first_not_of(" \t\""));
            Value.erase(Value.find_last_not_of(" \t\",") + 1);
            Config[Key] = Value;
        }
    }
    ServiceName = GetString("service_name", "ServiceAllocate");
    ListenPort = GetInt("listen_port", 50052);
    LogLevel = GetString("log_level", "info");
    return true;
}

std::string FConfigManager::GetString(const std::string &Key, const std::string &DefaultValue)
{
    auto It = Config.find(Key);
    return (It != Config.end()) ? It->second : DefaultValue;
}

int FConfigManager::GetInt(const std::string &Key, int DefaultValue)
{
    auto It = Config.find(Key);
    if (It != Config.end())
    {
        try
        {
            return std::stoi(It->second);
        }
        catch (...)
        {
            return DefaultValue;
        }
    }
    return DefaultValue;
}

bool FConfigManager::GetBool(const std::string &Key, bool DefaultValue)
{
    auto It = Config.find(Key);
    if (It != Config.end())
    {
        std::string Value = It->second;
        std::transform(Value.begin(), Value.end(), Value.begin(), ::tolower);
        return (Value == "true" || Value == "1" || Value == "yes");
    }
    return DefaultValue;
}

double FConfigManager::GetDouble(const std::string &Key, double DefaultValue)
{
    auto It = Config.find(Key);
    if (It != Config.end())
    {
        try
        {
            return std::stod(It->second);
        }
        catch (...)
        {
            return DefaultValue;
        }
    }
    return DefaultValue;
}

std::string FConfigManager::GetServiceName() const
{
    return ServiceName;
}

int FConfigManager::GetListenPort() const
{
    return ListenPort;
}

std::string FConfigManager::GetLogLevel() const
{
    return LogLevel;
}