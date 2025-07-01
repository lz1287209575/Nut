#pragma once

#include <string>
#include <map>
#include <memory>

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    bool LoadConfig(const std::string& configPath);
    std::string GetString(const std::string& key, const std::string& defaultValue = "");
    int GetInt(const std::string& key, int defaultValue = 0);
    bool GetBool(const std::string& key, bool defaultValue = false);
    double GetDouble(const std::string& key, double defaultValue = 0.0);
    
    // 服务配置相关
    std::string GetServiceName() const;
    int GetListenPort() const;
    std::string GetLogLevel() const;
    
private:
    std::map<std::string, std::string> m_config;
    std::string m_serviceName;
    int m_listenPort;
    std::string m_logLevel;
}; 