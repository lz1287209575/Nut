#pragma once

#include "ConfigManager.generate.h"
#include "ConfigValue.h"
#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Events/Delegate.h"
#include "JsonParser.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace NLib
{
/**
 * @brief 配置源类型枚举
 */
enum class EConfigSourceType : uint8_t
{
	File,        // 文件配置
	CommandLine, // 命令行参数
	Environment, // 环境变量
	Memory,      // 内存配置
	Remote       // 远程配置
};

/**
 * @brief 配置优先级
 */
enum class EConfigPriority : uint8_t
{
	Lowest = 0,
	Low = 1,
	Normal = 2,
	High = 3,
	Highest = 4,
	Override = 5 // 强制覆盖
};

/**
 * @brief 配置源信息
 */
struct SConfigSource
{
	TString Name;             // 配置源名称
	EConfigSourceType Type;   // 配置源类型
	EConfigPriority Priority; // 优先级
	TString Location;         // 位置（文件路径等）
	CConfigValue Data;        // 配置数据
	CDateTime LastModified;   // 最后修改时间
	bool bAutoReload = true;  // 是否自动重载
	bool bIsLoaded = false;   // 是否已加载

	SConfigSource() = default;

	SConfigSource(const TString& InName,
	              EConfigSourceType InType,
	              const TString& InLocation,
	              EConfigPriority InPriority = EConfigPriority::Normal)
	    : Name(InName),
	      Type(InType),
	      Priority(InPriority),
	      Location(InLocation),
	      LastModified(CDateTime::Now())
	{}
};

/**
 * @brief 配置变更事件参数
 */
struct SConfigChangeEvent
{
	TString Key;           // 配置键
	CConfigValue OldValue; // 旧值
	CConfigValue NewValue; // 新值
	TString SourceName;    // 配置源名称
	CDateTime ChangeTime;  // 变更时间

	SConfigChangeEvent() = default;

	SConfigChangeEvent(const TString& InKey,
	                   const CConfigValue& InOldValue,
	                   const CConfigValue& InNewValue,
	                   const TString& InSourceName)
	    : Key(InKey),
	      OldValue(InOldValue),
	      NewValue(InNewValue),
	      SourceName(InSourceName),
	      ChangeTime(CDateTime::Now())
	{}
};

/**
 * @brief 配置验证器接口
 */
class IConfigValidator
{
public:
	virtual ~IConfigValidator() = default;

	/**
	 * @brief 验证配置值
	 */
	virtual bool Validate(const TString& Key, const CConfigValue& Value, TString& OutErrorMessage) const = 0;

	/**
	 * @brief 获取验证器描述
	 */
	virtual TString GetDescription() const = 0;
};

/**
 * @brief 类型验证器
 */
class CTypeValidator : public IConfigValidator
{
public:
	explicit CTypeValidator(EConfigValueType InExpectedType)
	    : ExpectedType(InExpectedType)
	{}

	bool Validate(const TString& Key, const CConfigValue& Value, TString& OutErrorMessage) const override
	{
		if (Value.GetType() != ExpectedType)
		{
			OutErrorMessage = TString("Expected type ") + GetTypeName(ExpectedType) + TString(", got ") +
			                  Value.GetTypeName();
			return false;
		}
		return true;
	}

	TString GetDescription() const override
	{
		return TString("Type: ") + GetTypeName(ExpectedType);
	}

private:
	static TString GetTypeName(EConfigValueType Type)
	{
		switch (Type)
		{
		case EConfigValueType::Bool:
			return TString("bool");
		case EConfigValueType::Int32:
			return TString("int32");
		case EConfigValueType::Int64:
			return TString("int64");
		case EConfigValueType::Float:
			return TString("float");
		case EConfigValueType::Double:
			return TString("double");
		case EConfigValueType::String:
			return TString("string");
		case EConfigValueType::Array:
			return TString("array");
		case EConfigValueType::Object:
			return TString("object");
		default:
			return TString("unknown");
		}
	}

private:
	EConfigValueType ExpectedType;
};

/**
 * @brief 范围验证器
 */
template <typename T>
class TRangeValidator : public IConfigValidator
{
public:
	TRangeValidator(T InMinValue, T InMaxValue)
	    : MinValue(InMinValue),
	      MaxValue(InMaxValue)
	{}

	bool Validate(const TString& Key, const CConfigValue& Value, TString& OutErrorMessage) const override
	{
		T NumValue;

		if constexpr (std::is_integral_v<T>)
		{
			if (!Value.IsNumber())
			{
				OutErrorMessage = "Value must be a number";
				return false;
			}
			NumValue = static_cast<T>(Value.AsInt64());
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if (!Value.IsNumber())
			{
				OutErrorMessage = "Value must be a number";
				return false;
			}
			NumValue = static_cast<T>(Value.AsDouble());
		}
		else
		{
			OutErrorMessage = "Range validator only supports numeric types";
			return false;
		}

		if (NumValue < MinValue || NumValue > MaxValue)
		{
			OutErrorMessage = TString("Value must be between ") + TString::FromDouble(static_cast<double>(MinValue)) +
			                  TString(" and ") + TString::FromDouble(static_cast<double>(MaxValue));
			return false;
		}

		return true;
	}

	TString GetDescription() const override
	{
		return TString("Range: [") + TString::FromDouble(static_cast<double>(MinValue)) + TString(", ") +
		       TString::FromDouble(static_cast<double>(MaxValue)) + TString("]");
	}

private:
	T MinValue;
	T MaxValue;
};

/**
 * @brief 配置管理器
 *
 * 提供完整的配置管理功能：
 * - 多源配置合并
 * - 配置热重载
 * - 配置验证
 * - 配置监控
 * - 环境变量支持
 */
NCLASS()
class NConfigManager : public NObject
{
	GENERATED_BODY()

public:
	// === 委托定义 ===
	DECLARE_MULTICAST_DELEGATE(FOnConfigChanged, const SConfigChangeEvent&);
	DECLARE_MULTICAST_DELEGATE(FOnConfigSourceReloaded, const TString&);
	DECLARE_MULTICAST_DELEGATE(FOnConfigValidationFailed, const TString&, const TString&);

public:
	// === 单例模式 ===

	static NConfigManager& GetInstance()
	{
		static NConfigManager Instance;
		return Instance;
	}

private:
	// === 构造函数（私有） ===

	NConfigManager()
	    : bIsInitialized(false),
	      bAutoReloadEnabled(true),
	      bWatcherThreadRunning(false),
	      FileWatchInterval(CTimespan::FromSeconds(1.0))
	{}

public:
	// 禁用拷贝
	NConfigManager(const NConfigManager&) = delete;
	NConfigManager& operator=(const NConfigManager&) = delete;

	~NConfigManager()
	{
		Shutdown();
	}

public:
	// === 初始化和关闭 ===

	/**
	 * @brief 初始化配置管理器
	 */
	bool Initialize();

	/**
	 * @brief 关闭配置管理器
	 */
	void Shutdown();

	/**
	 * @brief 检查是否已初始化
	 */
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

public:
	// === 配置源管理 ===

	/**
	 * @brief 添加JSON文件配置源
	 */
	bool AddJsonFile(const TString& Name,
	                 const TString& FilePath,
	                 EConfigPriority Priority = EConfigPriority::Normal,
	                 bool bOptional = false);

	/**
	 * @brief 添加环境变量配置源
	 */
	bool AddEnvironmentVariables(const TString& Prefix = TString(""), EConfigPriority Priority = EConfigPriority::High);

	/**
	 * @brief 添加命令行参数配置源
	 */
	bool AddCommandLineArgs(int argc, char* argv[], EConfigPriority Priority = EConfigPriority::Highest);

	/**
	 * @brief 添加内存配置源
	 */
	bool AddMemoryConfig(const TString& Name,
	                     const CConfigValue& Config,
	                     EConfigPriority Priority = EConfigPriority::Normal);

	/**
	 * @brief 移除配置源
	 */
	bool RemoveConfigSource(const TString& Name);

	/**
	 * @brief 重载配置源
	 */
	bool ReloadConfigSource(const TString& Name);

	/**
	 * @brief 重载所有配置源
	 */
	void ReloadAllSources();

public:
	// === 配置访问 ===

	/**
	 * @brief 获取配置值
	 */
	CConfigValue GetConfig(const TString& Key, const CConfigValue& DefaultValue = CConfigValue()) const;

	/**
	 * @brief 设置配置值
	 */
	void SetConfig(const TString& Key, const CConfigValue& Value, const TString& SourceName = TString("Memory"));

	/**
	 * @brief 检查配置是否存在
	 */
	bool HasConfig(const TString& Key) const;

	/**
	 * @brief 获取所有配置键
	 */
	TArray<TString, CMemoryManager> GetAllKeys() const;

	/**
	 * @brief 获取指定前缀的所有配置
	 */
	CConfigObject GetConfigsWithPrefix(const TString& Prefix) const;

public:
	// === 类型安全的配置访问 ===

	/**
	 * @brief 获取布尔配置
	 */
	bool GetBool(const TString& Key, bool DefaultValue = false) const
	{
		return GetConfig(Key).AsBool(DefaultValue);
	}

	/**
	 * @brief 获取整数配置
	 */
	int32_t GetInt32(const TString& Key, int32_t DefaultValue = 0) const
	{
		return GetConfig(Key).AsInt32(DefaultValue);
	}

	int64_t GetInt64(const TString& Key, int64_t DefaultValue = 0) const
	{
		return GetConfig(Key).AsInt64(DefaultValue);
	}

	/**
	 * @brief 获取浮点数配置
	 */
	float GetFloat(const TString& Key, float DefaultValue = 0.0f) const
	{
		return GetConfig(Key).AsFloat(DefaultValue);
	}

	double GetDouble(const TString& Key, double DefaultValue = 0.0) const
	{
		return GetConfig(Key).AsDouble(DefaultValue);
	}

	/**
	 * @brief 获取字符串配置
	 */
	TString GetString(const TString& Key, const TString& DefaultValue = TString()) const
	{
		return GetConfig(Key).AsString(DefaultValue);
	}

	/**
	 * @brief 获取数组配置
	 */
	const CConfigArray& GetArray(const TString& Key) const
	{
		return GetConfig(Key).AsArray();
	}

	/**
	 * @brief 获取对象配置
	 */
	const CConfigObject& GetObject(const TString& Key) const
	{
		return GetConfig(Key).AsObject();
	}

public:
	// === 配置验证 ===

	/**
	 * @brief 添加配置验证器
	 */
	void AddValidator(const TString& Key, TSharedPtr<IConfigValidator> Validator);

	/**
	 * @brief 移除配置验证器
	 */
	void RemoveValidator(const TString& Key);

	/**
	 * @brief 验证所有配置
	 */
	bool ValidateAllConfigs(TArray<TString, CMemoryManager>& OutErrors) const;

	/**
	 * @brief 验证指定配置
	 */
	bool ValidateConfig(const TString& Key, TString& OutError) const;

public:
	// === 配置监控 ===

	/**
	 * @brief 启用/禁用自动重载
	 */
	void SetAutoReloadEnabled(bool bEnabled);

	/**
	 * @brief 设置文件监控间隔
	 */
	void SetFileWatchInterval(const CTimespan& Interval);

	/**
	 * @brief 获取配置源信息
	 */
	TArray<SConfigSource, CMemoryManager> GetConfigSources() const;

	/**
	 * @brief 获取合并后的配置
	 */
	CConfigValue GetMergedConfig() const;

public:
	// === 委托事件 ===

	FOnConfigChanged OnConfigChanged;
	FOnConfigSourceReloaded OnConfigSourceReloaded;
	FOnConfigValidationFailed OnConfigValidationFailed;

public:
	// === 调试和诊断 ===

	/**
	 * @brief 生成配置报告
	 */
	TString GenerateConfigReport() const;

	/**
	 * @brief 导出配置到文件
	 */
	bool ExportConfig(const TString& FilePath, bool bPrettyPrint = true) const;

	/**
	 * @brief 获取配置统计信息
	 */
	struct SConfigStats
	{
		int32_t TotalSources = 0;
		int32_t LoadedSources = 0;
		int32_t TotalConfigs = 0;
		int32_t ValidatedConfigs = 0;
		int32_t FailedValidations = 0;
		CTimespan LastReloadTime;
	};

	SConfigStats GetConfigStats() const;

private:
	// === 内部实现 ===

	/**
	 * @brief 合并所有配置源
	 */
	void MergeAllSources();

	/**
	 * @brief 加载配置源
	 */
	bool LoadConfigSource(SConfigSource& Source);

	/**
	 * @brief 解析命令行参数
	 */
	CConfigValue ParseCommandLineArgs(int argc, char* argv[]);

	/**
	 * @brief 解析环境变量
	 */
	CConfigValue ParseEnvironmentVariables(const TString& Prefix);

	/**
	 * @brief 文件监控线程
	 */
	void FileWatcherThread();

	/**
	 * @brief 检查文件是否已修改
	 */
	bool IsFileModified(const SConfigSource& Source) const;

	/**
	 * @brief 通知配置变更
	 */
	void NotifyConfigChanged(const TString& Key,
	                         const CConfigValue& OldValue,
	                         const CConfigValue& NewValue,
	                         const TString& SourceName);

	/**
	 * @brief 应用配置值
	 */
	void ApplyConfigValue(const TString& Key, const CConfigValue& Value, const TString& SourceName);

private:
	// === 成员变量 ===

	std::atomic<bool> bIsInitialized;        // 是否已初始化
	std::atomic<bool> bAutoReloadEnabled;    // 是否启用自动重载
	std::atomic<bool> bWatcherThreadRunning; // 文件监控线程是否运行

	CTimespan FileWatchInterval; // 文件监控间隔

	// 配置数据
	TArray<SConfigSource, CMemoryManager> ConfigSources;         // 配置源列表
	CConfigValue MergedConfig;                                   // 合并后的配置
	THashMap<TString, CConfigValue, CMemoryManager> ConfigCache; // 配置缓存

	// 验证器
	THashMap<TString, TSharedPtr<IConfigValidator>, CMemoryManager> Validators; // 配置验证器

	// 线程安全
	mutable std::mutex ConfigMutex;  // 配置互斥锁
	mutable std::mutex SourcesMutex; // 配置源互斥锁

	// 文件监控
	std::thread FileWatcherThread_; // 文件监控线程
};

// === 便捷验证器创建函数 ===

/**
 * @brief 创建类型验证器
 */
inline TSharedPtr<IConfigValidator> CreateTypeValidator(EConfigValueType Type)
{
	return MakeShared<CTypeValidator>(Type);
}

/**
 * @brief 创建整数范围验证器
 */
inline TSharedPtr<IConfigValidator> CreateIntRangeValidator(int64_t MinValue, int64_t MaxValue)
{
	return MakeShared<TRangeValidator<int64_t>>(MinValue, MaxValue);
}

/**
 * @brief 创建浮点数范围验证器
 */
inline TSharedPtr<IConfigValidator> CreateFloatRangeValidator(double MinValue, double MaxValue)
{
	return MakeShared<TRangeValidator<double>>(MinValue, MaxValue);
}

// === 全局访问函数 ===

/**
 * @brief 获取全局配置管理器
 */
inline NConfigManager& GetConfigManager()
{
	return NConfigManager::GetInstance();
}

/**
 * @brief 便捷配置访问宏
 */
#define CONFIG_GET(Key, DefaultValue) NLib::GetConfigManager().GetConfig(Key, DefaultValue)
#define CONFIG_GET_BOOL(Key, DefaultValue) NLib::GetConfigManager().GetBool(Key, DefaultValue)
#define CONFIG_GET_INT(Key, DefaultValue) NLib::GetConfigManager().GetInt32(Key, DefaultValue)
#define CONFIG_GET_FLOAT(Key, DefaultValue) NLib::GetConfigManager().GetFloat(Key, DefaultValue)
#define CONFIG_GET_STRING(Key, DefaultValue) NLib::GetConfigManager().GetString(Key, DefaultValue)

#define CONFIG_SET(Key, Value) NLib::GetConfigManager().SetConfig(Key, Value)
#define CONFIG_HAS(Key) NLib::GetConfigManager().HasConfig(Key)

// === 类型别名 ===
using FConfigManager = NConfigManager;
using FConfigSource = SConfigSource;
using FConfigChangeEvent = SConfigChangeEvent;

} // namespace NLib