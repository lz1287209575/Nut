#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

ConfigManager::ConfigManager() : m_listenPort(50052) {
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::LoadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << configPath << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 简单的键值对解析，实际项目中可以使用JSON库
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除空格和引号
            key.erase(0, key.find_first_not_of(" \t\""));
            key.erase(key.find_last_not_of(" \t\",") + 1);
            value.erase(0, value.find_first_not_of(" \t\""));
            value.erase(value.find_last_not_of(" \t\",") + 1);
            
            m_config[key] = value;
        }
    }
    
    // 解析服务配置
    m_serviceName = GetString("service_name", "ServiceAllocate");
    m_listenPort = GetInt("listen_port", 50052);
    m_logLevel = GetString("log_level", "info");
    
    return true;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) {
    auto it = m_config.find(key);
    return (it != m_config.end()) ? it->second : defaultValue;
}

int ConfigManager::GetInt(const std::string& key, int defaultValue) {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue) {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }
    return defaultValue;
}

double ConfigManager::GetDouble(const std::string& key, double defaultValue) {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

std::string ConfigManager::GetServiceName() const {
    return m_serviceName;
}

int ConfigManager::GetListenPort() const {
    return m_listenPort;
}

std::string ConfigManager::GetLogLevel() const {
    return m_logLevel;
} 