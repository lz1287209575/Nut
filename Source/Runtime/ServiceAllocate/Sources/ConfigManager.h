#pragma once

#include <map>
#include <memory>
#include <string>

class FConfigManager
{
public:
	FConfigManager();
	~FConfigManager();

	bool LoadConfig(const std::string& ConfigPath);
	std::string GetString(const std::string& Key, const std::string& DefaultValue = "");
	int GetInt(const std::string& Key, int DefaultValue = 0);
	bool GetBool(const std::string& Key, bool DefaultValue = false);
	double GetDouble(const std::string& Key, double DefaultValue = 0.0);

	// 服务配置相关
	std::string GetServiceName() const;
	int GetListenPort() const;
	std::string GetLogLevel() const;

private:
	std::map<std::string, std::string> Config;
	std::string ServiceName;
	int ListenPort;
	std::string LogLevel;
};