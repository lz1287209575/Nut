#pragma once

/**
 * @file Config.h
 * @brief NLib配置管理模块主头文件
 *
 * 提供完整的配置管理功能：
 * - 多源配置支持（JSON文件、环境变量、命令行参数、内存配置）
 * - 类型安全的配置访问
 * - 配置热重载和文件监控
 * - 配置验证系统
 * - 路径式配置访问（支持嵌套对象和数组）
 * - JSON解析和生成
 * - 事件驱动的配置变更通知
 */

// === 核心配置类 ===
#include "ConfigManager.h" // 配置管理器
#include "ConfigValue.h"   // 配置值类型系统
#include "JsonParser.h"    // JSON解析和生成

namespace NLib
{
/**
 * @brief 配置模块初始化
 *
 * 初始化配置管理器和相关系统
 *
 * @return 是否初始化成功
 */
inline bool InitializeConfigModule()
{
	return NConfigManager::GetInstance().Initialize();
}

/**
 * @brief 配置模块关闭
 *
 * 关闭配置管理器并清理资源
 */
inline void ShutdownConfigModule()
{
	NConfigManager::GetInstance().Shutdown();
}

/**
 * @brief 检查配置模块是否已初始化
 *
 * @return 是否已初始化
 */
inline bool IsConfigModuleInitialized()
{
	return NConfigManager::GetInstance().IsInitialized();
}

// === 便捷配置访问函数 ===

/**
 * @brief 快速加载JSON配置文件
 *
 * @param FilePath 文件路径
 * @param Priority 配置优先级
 * @param bOptional 是否为可选文件
 * @return 是否加载成功
 */
inline bool LoadConfigFile(const CString& FilePath,
                           EConfigPriority Priority = EConfigPriority::Normal,
                           bool bOptional = false)
{
	CString FileName = FilePath;
	int32_t LastSlash = FileName.LastIndexOf('/');
	if (LastSlash >= 0)
	{
		FileName = FileName.Substring(LastSlash + 1);
	}

	return NConfigManager::GetInstance().AddJsonFile(FileName, FilePath, Priority, bOptional);
}

/**
 * @brief 快速加载环境变量配置
 *
 * @param Prefix 环境变量前缀
 * @param Priority 配置优先级
 * @return 是否加载成功
 */
inline bool LoadEnvironmentConfig(const CString& Prefix = CString(), EConfigPriority Priority = EConfigPriority::High)
{
	return NConfigManager::GetInstance().AddEnvironmentVariables(Prefix, Priority);
}

/**
 * @brief 快速加载命令行参数配置
 *
 * @param argc 参数个数
 * @param argv 参数数组
 * @param Priority 配置优先级
 * @return 是否加载成功
 */
inline bool LoadCommandLineConfig(int argc, char* argv[], EConfigPriority Priority = EConfigPriority::Highest)
{
	return NConfigManager::GetInstance().AddCommandLineArgs(argc, argv, Priority);
}

/**
 * @brief 标准配置初始化流程
 *
 * 按照标准顺序加载配置：
 * 1. 默认配置文件（最低优先级）
 * 2. 用户配置文件（普通优先级）
 * 3. 环境变量（高优先级）
 * 4. 命令行参数（最高优先级）
 *
 * @param DefaultConfigPath 默认配置文件路径
 * @param UserConfigPath 用户配置文件路径
 * @param EnvPrefix 环境变量前缀
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 是否成功
 */
inline bool InitializeStandardConfig(const CString& DefaultConfigPath,
                                     const CString& UserConfigPath = CString(),
                                     const CString& EnvPrefix = CString(),
                                     int argc = 0,
                                     char* argv[] = nullptr)
{
	auto& ConfigMgr = NConfigManager::GetInstance();

	if (!ConfigMgr.Initialize())
	{
		return false;
	}

	// 1. 加载默认配置文件
	if (!DefaultConfigPath.IsEmpty())
	{
		ConfigMgr.AddJsonFile("Default", DefaultConfigPath, EConfigPriority::Lowest, false);
	}

	// 2. 加载用户配置文件（可选）
	if (!UserConfigPath.IsEmpty())
	{
		ConfigMgr.AddJsonFile("User", UserConfigPath, EConfigPriority::Normal, true);
	}

	// 3. 加载环境变量
	if (!EnvPrefix.IsEmpty())
	{
		ConfigMgr.AddEnvironmentVariables(EnvPrefix, EConfigPriority::High);
	}

	// 4. 加载命令行参数
	if (argc > 0 && argv != nullptr)
	{
		ConfigMgr.AddCommandLineArgs(argc, argv, EConfigPriority::Highest);
	}

	return true;
}

// === 配置验证便捷函数 ===

/**
 * @brief 添加类型验证器
 *
 * @param Key 配置键
 * @param Type 期望的类型
 */
inline void ValidateConfigType(const CString& Key, EConfigValueType Type)
{
	NConfigManager::GetInstance().AddValidator(Key, CreateTypeValidator(Type));
}

/**
 * @brief 添加整数范围验证器
 *
 * @param Key 配置键
 * @param MinValue 最小值
 * @param MaxValue 最大值
 */
inline void ValidateConfigIntRange(const CString& Key, int64_t MinValue, int64_t MaxValue)
{
	NConfigManager::GetInstance().AddValidator(Key, CreateIntRangeValidator(MinValue, MaxValue));
}

/**
 * @brief 添加浮点数范围验证器
 *
 * @param Key 配置键
 * @param MinValue 最小值
 * @param MaxValue 最大值
 */
inline void ValidateConfigFloatRange(const CString& Key, double MinValue, double MaxValue)
{
	NConfigManager::GetInstance().AddValidator(Key, CreateFloatRangeValidator(MinValue, MaxValue));
}

/**
 * @brief 验证所有配置并输出错误
 *
 * @return 是否所有配置都有效
 */
inline bool ValidateAllConfigurations()
{
	TArray<CString, CMemoryManager> Errors;
	bool bValid = NConfigManager::GetInstance().ValidateAllConfigs(Errors);

	if (!bValid)
	{
		NLOG_CONFIG(Error, "Configuration validation failed:");
		for (const auto& Error : Errors)
		{
			NLOG_CONFIG(Error, "  {}", Error.GetData());
		}
	}

	return bValid;
}

// === 配置热重载便捷函数 ===

/**
 * @brief 启用配置自动重载
 *
 * @param bEnabled 是否启用
 * @param WatchInterval 监控间隔（秒）
 */
inline void EnableConfigAutoReload(bool bEnabled = true, double WatchInterval = 1.0)
{
	auto& ConfigMgr = NConfigManager::GetInstance();
	ConfigMgr.SetAutoReloadEnabled(bEnabled);
	ConfigMgr.SetFileWatchInterval(CTimespan::FromSeconds(WatchInterval));
}

/**
 * @brief 手动重载所有配置
 */
inline void ReloadAllConfigurations()
{
	NConfigManager::GetInstance().ReloadAllSources();
}

/**
 * @brief 重载指定配置源
 *
 * @param SourceName 配置源名称
 * @return 是否重载成功
 */
inline bool ReloadConfiguration(const CString& SourceName)
{
	return NConfigManager::GetInstance().ReloadConfigSource(SourceName);
}

// === 配置导出和诊断函数 ===

/**
 * @brief 导出当前配置到文件
 *
 * @param FilePath 导出文件路径
 * @param bPrettyPrint 是否格式化输出
 * @return 是否导出成功
 */
inline bool ExportCurrentConfig(const CString& FilePath, bool bPrettyPrint = true)
{
	return NConfigManager::GetInstance().ExportConfig(FilePath, bPrettyPrint);
}

/**
 * @brief 生成配置诊断报告
 *
 * @return 诊断报告字符串
 */
inline CString GenerateConfigDiagnostics()
{
	return NConfigManager::GetInstance().GenerateConfigReport();
}

/**
 * @brief 打印配置统计信息
 */
inline void PrintConfigStatistics()
{
	auto Stats = NConfigManager::GetInstance().GetConfigStats();

	NLOG_CONFIG(Info, "=== Configuration Statistics ===");
	NLOG_CONFIG(Info, "Total Sources: {}", Stats.TotalSources);
	NLOG_CONFIG(Info, "Loaded Sources: {}", Stats.LoadedSources);
	NLOG_CONFIG(Info, "Total Configs: {}", Stats.TotalConfigs);
	NLOG_CONFIG(Info, "Validated Configs: {}", Stats.ValidatedConfigs);
	NLOG_CONFIG(Info, "Failed Validations: {}", Stats.FailedValidations);
}

// === 模块导出 ===

// 主要类的类型别名
using FConfig = NConfigManager;
using FConfigValue = CConfigValue;
using FConfigArray = CConfigArray;
using FConfigObject = CConfigObject;
using FJsonParser = CJsonParser;
using FJsonGenerator = CJsonGenerator;

// 枚举类型别名
using EConfigType = EConfigValueType;
using EConfigSourceType = EConfigSourceType;
using EConfigPriority = EConfigPriority;

// 事件类型别名
using FConfigChangeEvent = SConfigChangeEvent;
using FConfigSource = SConfigSource;
using FConfigStats = NConfigManager::SConfigStats;

// 验证器类型别名
using IConfigValidator = IConfigValidator;
using FTypeValidator = CTypeValidator;

template <typename T>
using TRangeValidator = TRangeValidator<T>;

} // namespace NLib

/**
 * @brief 配置模块使用示例
 *
 * @code
 * // 1. 初始化配置模块
 * NLib::InitializeStandardConfig("config/default.json", "config/user.json", "MYAPP", argc, argv);
 *
 * // 2. 添加配置验证
 * NLib::ValidateConfigType("server.port", NLib::EConfigValueType::Int32);
 * NLib::ValidateConfigIntRange("server.port", 1024, 65535);
 * NLib::ValidateConfigType("server.host", NLib::EConfigValueType::String);
 *
 * // 3. 启用自动重载
 * NLib::EnableConfigAutoReload(true, 2.0); // 每2秒检查一次
 *
 * // 4. 使用配置
 * CString Host = CONFIG_GET_STRING("server.host", "localhost");
 * int32_t Port = CONFIG_GET_INT("server.port", 8080);
 * bool Debug = CONFIG_GET_BOOL("debug.enabled", false);
 *
 * // 5. 监听配置变更
 * NLib::GetConfigManager().OnConfigChanged.BindLambda([](const NLib::SConfigChangeEvent& Event) {
 *     NLOG_CONFIG(Info, "Config changed: {} -> {}", Event.Key.GetData(), Event.NewValue.ToString().GetData());
 * });
 *
 * // 6. 运行时修改配置
 * CONFIG_SET("runtime.mode", "production");
 *
 * // 7. 导出当前配置
 * NLib::ExportCurrentConfig("config/current.json");
 *
 * // 8. 关闭配置模块
 * NLib::ShutdownConfigModule();
 * @endcode
 */
