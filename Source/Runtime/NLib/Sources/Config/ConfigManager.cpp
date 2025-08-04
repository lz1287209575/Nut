#include "ConfigManager.h"

#include "IO/FileSystem.h"
#include "Logging/LogCategory.h"
#include "Time/TimeManager.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace NLib
{
// === CConfigManager 初始化和关闭 ===

bool CConfigManager::Initialize()
{
	std::lock_guard<std::mutex> Lock(ConfigMutex);

	if (bIsInitialized.load())
	{
		NLOG_CONFIG(Warning, "ConfigManager already initialized");
		return true;
	}

	NLOG_CONFIG(Info, "Initializing ConfigManager");

	// 初始化内部状态
	ConfigSources.Clear();
	MergedConfig = CConfigValue();
	ConfigCache.Clear();
	Validators.Clear();

	// 设置默认文件监控间隔
	FileWatchInterval = CTimespan::FromSeconds(1.0);
	bAutoReloadEnabled.store(true);

	bIsInitialized.store(true);

	NLOG_CONFIG(Info, "ConfigManager initialized successfully");
	return true;
}

void CConfigManager::Shutdown()
{
	if (!bIsInitialized.load())
	{
		return;
	}

	NLOG_CONFIG(Info, "Shutting down ConfigManager");

	// 停止文件监控线程
	if (bWatcherThreadRunning.load())
	{
		bWatcherThreadRunning.store(false);
		if (FileWatcherThread_.joinable())
		{
			FileWatcherThread_.join();
		}
	}

	std::lock_guard<std::mutex> ConfigLock(ConfigMutex);
	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);

	// 清理资源
	ConfigSources.Clear();
	MergedConfig = CConfigValue();
	ConfigCache.Clear();
	Validators.Clear();

	bIsInitialized.store(false);

	NLOG_CONFIG(Info, "ConfigManager shutdown complete");
}

// === 配置源管理 ===

bool CConfigManager::AddJsonFile(const TString& Name, const TString& FilePath, EConfigPriority Priority, bool bOptional)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Adding JSON file config source: {} -> {}", Name.GetData(), FilePath.GetData());

	// 检查文件是否存在
	if (!bOptional && !std::filesystem::exists(FilePath.GetData()))
	{
		NLOG_CONFIG(Error, "Required config file does not exist: {}", FilePath.GetData());
		return false;
	}

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	// 检查是否已存在同名配置源
	for (const auto& Source : ConfigSources)
	{
		if (Source.Name == Name)
		{
			NLOG_CONFIG(Warning, "Config source with name '{}' already exists", Name.GetData());
			return false;
		}
	}

	// 创建配置源
	SConfigSource NewSource(Name, EConfigSourceType::File, FilePath, Priority);
	NewSource.bAutoReload = true;

	// 尝试加载配置
	if (!LoadConfigSource(NewSource))
	{
		if (!bOptional)
		{
			NLOG_CONFIG(Error, "Failed to load required config file: {}", FilePath.GetData());
			return false;
		}
		else
		{
			NLOG_CONFIG(Warning, "Failed to load optional config file: {}", FilePath.GetData());
		}
	}

	ConfigSources.Add(NewSource);

	// 重新合并配置
	MergeAllSources();

	// 启动文件监控
	if (bAutoReloadEnabled.load() && !bWatcherThreadRunning.load())
	{
		bWatcherThreadRunning.store(true);
		FileWatcherThread_ = std::thread(&CConfigManager::FileWatcherThread, this);
	}

	NLOG_CONFIG(Info, "Successfully added JSON config source: {}", Name.GetData());
	return true;
}

bool CConfigManager::AddEnvironmentVariables(const TString& Prefix, EConfigPriority Priority)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Adding environment variables config source with prefix: {}", Prefix.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	TString SourceName = TString("Environment_") + (Prefix.IsEmpty() ? TString("All") : Prefix);

	// 检查是否已存在
	for (const auto& Source : ConfigSources)
	{
		if (Source.Name == SourceName)
		{
			NLOG_CONFIG(Warning, "Environment config source already exists: {}", SourceName.GetData());
			return false;
		}
	}

	// 创建配置源
	SConfigSource NewSource(SourceName, EConfigSourceType::Environment, Prefix, Priority);
	NewSource.bAutoReload = false; // 环境变量不支持自动重载
	NewSource.Data = ParseEnvironmentVariables(Prefix);
	NewSource.bIsLoaded = true;

	ConfigSources.Add(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added environment variables config source");
	return true;
}

bool CConfigManager::AddCommandLineArgs(int argc, char* argv[], EConfigPriority Priority)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Adding command line arguments config source");

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	// 检查是否已存在
	for (const auto& Source : ConfigSources)
	{
		if (Source.Type == EConfigSourceType::CommandLine)
		{
			NLOG_CONFIG(Warning, "Command line config source already exists");
			return false;
		}
	}

	// 创建配置源
	SConfigSource NewSource(TString("CommandLine"), EConfigSourceType::CommandLine, TString(), Priority);
	NewSource.bAutoReload = false; // 命令行参数不支持自动重载
	NewSource.Data = ParseCommandLineArgs(argc, argv);
	NewSource.bIsLoaded = true;

	ConfigSources.Add(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added command line arguments config source");
	return true;
}

bool CConfigManager::AddMemoryConfig(const TString& Name, const CConfigValue& Config, EConfigPriority Priority)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Adding memory config source: {}", Name.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	// 检查是否已存在同名配置源
	for (const auto& Source : ConfigSources)
	{
		if (Source.Name == Name)
		{
			NLOG_CONFIG(Warning, "Config source with name '{}' already exists", Name.GetData());
			return false;
		}
	}

	// 创建配置源
	SConfigSource NewSource(Name, EConfigSourceType::Memory, TString(), Priority);
	NewSource.bAutoReload = false; // 内存配置不支持自动重载
	NewSource.Data = Config;
	NewSource.bIsLoaded = true;

	ConfigSources.Add(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added memory config source: {}", Name.GetData());
	return true;
}

bool CConfigManager::RemoveConfigSource(const TString& Name)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Removing config source: {}", Name.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	for (int32_t i = 0; i < ConfigSources.Size(); ++i)
	{
		if (ConfigSources[i].Name == Name)
		{
			ConfigSources.RemoveAt(i);

			// 重新合并配置
			MergeAllSources();

			NLOG_CONFIG(Info, "Successfully removed config source: {}", Name.GetData());
			return true;
		}
	}

	NLOG_CONFIG(Warning, "Config source not found: {}", Name.GetData());
	return false;
}

bool CConfigManager::ReloadConfigSource(const TString& Name)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Reloading config source: {}", Name.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	for (auto& Source : ConfigSources)
	{
		if (Source.Name == Name)
		{
			if (LoadConfigSource(Source))
			{
				MergeAllSources();
				OnConfigSourceReloaded.Broadcast(Name);
				NLOG_CONFIG(Info, "Successfully reloaded config source: {}", Name.GetData());
				return true;
			}
			else
			{
				NLOG_CONFIG(Error, "Failed to reload config source: {}", Name.GetData());
				return false;
			}
		}
	}

	NLOG_CONFIG(Warning, "Config source not found: {}", Name.GetData());
	return false;
}

void CConfigManager::ReloadAllSources()
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return;
	}

	NLOG_CONFIG(Info, "Reloading all config sources");

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	bool bAnyReloaded = false;

	for (auto& Source : ConfigSources)
	{
		if (Source.Type == EConfigSourceType::File && LoadConfigSource(Source))
		{
			OnConfigSourceReloaded.Broadcast(Source.Name);
			bAnyReloaded = true;
		}
	}

	if (bAnyReloaded)
	{
		MergeAllSources();
	}

	NLOG_CONFIG(Info, "Completed reloading all config sources");
}

// === 配置访问 ===

CConfigValue CConfigManager::GetConfig(const TString& Key, const CConfigValue& DefaultValue) const
{
	if (!bIsInitialized.load())
	{
		return DefaultValue;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	// 首先检查缓存
	if (ConfigCache.Contains(Key))
	{
		return ConfigCache[Key];
	}

	// 从合并配置中获取
	const CConfigValue& Value = MergedConfig.GetByPath(Key);
	if (!Value.IsNull())
	{
		// 缓存结果
		const_cast<CConfigManager*>(this)->ConfigCache.Add(Key, Value);
		return Value;
	}

	return DefaultValue;
}

void CConfigManager::SetConfig(const TString& Key, const CConfigValue& Value, const TString& SourceName)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return;
	}

	NLOG_CONFIG(Debug, "Setting config: {} in source: {}", Key.GetData(), SourceName.GetData());

	std::lock_guard<std::mutex> ConfigLock(ConfigMutex);
	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);

	// 查找或创建内存配置源
	SConfigSource* TargetSource = nullptr;
	for (auto& Source : ConfigSources)
	{
		if (Source.Name == SourceName && Source.Type == EConfigSourceType::Memory)
		{
			TargetSource = &Source;
			break;
		}
	}

	if (!TargetSource)
	{
		// 创建新的内存配置源
		SConfigSource NewSource(SourceName, EConfigSourceType::Memory, TString(), EConfigPriority::High);
		NewSource.bAutoReload = false;
		NewSource.Data = CConfigObject();
		NewSource.bIsLoaded = true;
		ConfigSources.Add(NewSource);
		TargetSource = &ConfigSources.Last();
	}

	// 获取旧值用于事件通知
	CConfigValue OldValue = GetConfig(Key);

	// 设置新值
	TargetSource->Data.SetByPath(Key, Value);
	TargetSource->LastModified = CDateTime::Now();

	// 重新合并配置
	MergeAllSources();

	// 清除缓存
	ConfigCache.Remove(Key);

	// 触发变更事件
	NotifyConfigChanged(Key, OldValue, Value, SourceName);
}

bool CConfigManager::HasConfig(const TString& Key) const
{
	if (!bIsInitialized.load())
	{
		return false;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	return MergedConfig.HasPath(Key);
}

TArray<TString, CMemoryManager> CConfigManager::GetAllKeys() const
{
	TArray<TString, CMemoryManager> Keys;

	if (!bIsInitialized.load())
	{
		return Keys;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	// 从所有配置源收集键
	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);
	for (const auto& Source : ConfigSources)
	{
		CollectKeysFromValue(Source.Data, TString(), Keys);
	}

	// 去重
	for (int32_t i = 0; i < Keys.Size() - 1; ++i)
	{
		for (int32_t j = i + 1; j < Keys.Size(); ++j)
		{
			if (Keys[i] == Keys[j])
			{
				Keys.RemoveAt(j);
				--j;
			}
		}
	}

	return Keys;
}

CConfigObject CConfigManager::GetConfigsWithPrefix(const TString& Prefix) const
{
	CConfigObject Result;

	if (!bIsInitialized.load())
	{
		return Result;
	}

	auto AllKeys = GetAllKeys();
	for (const auto& Key : AllKeys)
	{
		if (Key.StartsWith(Prefix))
		{
			TString RelativeKey = Key.Substring(Prefix.Length());
			if (RelativeKey.StartsWith("."))
			{
				RelativeKey = RelativeKey.Substring(1);
			}

			if (!RelativeKey.IsEmpty())
			{
				Result.Add(RelativeKey, GetConfig(Key));
			}
		}
	}

	return Result;
}

// === 配置验证 ===

void CConfigManager::AddValidator(const TString& Key, TSharedPtr<IConfigValidator> Validator)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return;
	}

	NLOG_CONFIG(Info, "Adding validator for key: {}", Key.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	Validators.Add(Key, Validator);
}

void CConfigManager::RemoveValidator(const TString& Key)
{
	if (!bIsInitialized.load())
	{
		return;
	}

	NLOG_CONFIG(Info, "Removing validator for key: {}", Key.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	Validators.Remove(Key);
}

bool CConfigManager::ValidateAllConfigs(TArray<TString, CMemoryManager>& OutErrors) const
{
	OutErrors.Clear();

	if (!bIsInitialized.load())
	{
		OutErrors.Add("ConfigManager not initialized");
		return false;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	bool bAllValid = true;

	for (const auto& ValidatorPair : Validators)
	{
		const TString& Key = ValidatorPair.Key;
		const auto& Validator = ValidatorPair.Value;

		CConfigValue Value = GetConfig(Key);
		TString ErrorMessage;

		if (!Validator->Validate(Key, Value, ErrorMessage))
		{
			OutErrors.Add(TString("Validation failed for '") + Key + TString("': ") + ErrorMessage);
			bAllValid = false;

			// 触发验证失败事件
			const_cast<CConfigManager*>(this)->OnConfigValidationFailed.Broadcast(Key, ErrorMessage);
		}
	}

	return bAllValid;
}

bool CConfigManager::ValidateConfig(const TString& Key, TString& OutError) const
{
	if (!bIsInitialized.load())
	{
		OutError = "ConfigManager not initialized";
		return false;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	if (!Validators.Contains(Key))
	{
		return true; // 没有验证器，认为有效
	}

	const auto& Validator = Validators[Key];
	CConfigValue Value = GetConfig(Key);

	if (!Validator->Validate(Key, Value, OutError))
	{
		// 触发验证失败事件
		const_cast<CConfigManager*>(this)->OnConfigValidationFailed.Broadcast(Key, OutError);
		return false;
	}

	return true;
}

// === 配置监控 ===

void CConfigManager::SetAutoReloadEnabled(bool bEnabled)
{
	bAutoReloadEnabled.store(bEnabled);

	if (bEnabled && !bWatcherThreadRunning.load() && bIsInitialized.load())
	{
		bWatcherThreadRunning.store(true);
		FileWatcherThread_ = std::thread(&CConfigManager::FileWatcherThread, this);
	}
	else if (!bEnabled && bWatcherThreadRunning.load())
	{
		bWatcherThreadRunning.store(false);
		if (FileWatcherThread_.joinable())
		{
			FileWatcherThread_.join();
		}
	}

	NLOG_CONFIG(Info, "Auto reload {}", bEnabled ? "enabled" : "disabled");
}

void CConfigManager::SetFileWatchInterval(const CTimespan& Interval)
{
	FileWatchInterval = Interval;
	NLOG_CONFIG(Info, "File watch interval set to {} seconds", Interval.GetTotalSeconds());
}

TArray<SConfigSource, CMemoryManager> CConfigManager::GetConfigSources() const
{
	if (!bIsInitialized.load())
	{
		return TArray<SConfigSource, CMemoryManager>();
	}

	std::lock_guard<std::mutex> Lock(SourcesMutex);
	return ConfigSources;
}

CConfigValue CConfigManager::GetMergedConfig() const
{
	if (!bIsInitialized.load())
	{
		return CConfigValue();
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	return MergedConfig;
}

// === 调试和诊断 ===

TString CConfigManager::GenerateConfigReport() const
{
	if (!bIsInitialized.load())
	{
		return TString("ConfigManager not initialized");
	}

	TString Report = TString("=== Configuration Report ===\n\n");

	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);

	Report += TString("Config Sources (") + TString::FromInt(ConfigSources.Size()) + TString("):\n");
	for (const auto& Source : ConfigSources)
	{
		Report += TString("  - ") + Source.Name;
		Report += TString(" (") + GetSourceTypeName(Source.Type) + TString(")");
		Report += TString(" Priority: ") + TString::FromInt(static_cast<int>(Source.Priority));
		Report += TString(" Loaded: ") + (Source.bIsLoaded ? TString("Yes") : TString("No"));
		if (!Source.Location.IsEmpty())
		{
			Report += TString(" Location: ") + Source.Location;
		}
		Report += TString("\n");
	}

	Report += TString("\nValidators (") + TString::FromInt(Validators.Size()) + TString("):\n");
	std::lock_guard<std::mutex> ConfigLock(ConfigMutex);
	for (const auto& ValidatorPair : Validators)
	{
		Report += TString("  - ") + ValidatorPair.Key;
		Report += TString(": ") + ValidatorPair.Value->GetDescription();
		Report += TString("\n");
	}

	Report += TString("\nCache Entries: ") + TString::FromInt(ConfigCache.Size()) + TString("\n");
	Report += TString("Auto Reload: ") + (bAutoReloadEnabled.load() ? TString("Enabled") : TString("Disabled")) +
	          TString("\n");
	Report += TString("File Watch Interval: ") + TString::FromDouble(FileWatchInterval.GetTotalSeconds()) +
	          TString(" seconds\n");

	return Report;
}

bool CConfigManager::ExportConfig(const TString& FilePath, bool bPrettyPrint) const
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Exporting config to file: {}", FilePath.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	TString JsonString = MergedConfig.ToJsonString(bPrettyPrint);

	std::ofstream File(FilePath.GetData());
	if (!File.is_open())
	{
		NLOG_CONFIG(Error, "Failed to open file for writing: {}", FilePath.GetData());
		return false;
	}

	File << JsonString.GetData();
	File.close();

	if (File.fail())
	{
		NLOG_CONFIG(Error, "Failed to write config to file: {}", FilePath.GetData());
		return false;
	}

	NLOG_CONFIG(Info, "Successfully exported config to file: {}", FilePath.GetData());
	return true;
}

CConfigManager::SConfigStats CConfigManager::GetConfigStats() const
{
	SConfigStats Stats;

	if (!bIsInitialized.load())
	{
		return Stats;
	}

	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);
	std::lock_guard<std::mutex> ConfigLock(ConfigMutex);

	Stats.TotalSources = ConfigSources.Size();
	Stats.LoadedSources = 0;
	for (const auto& Source : ConfigSources)
	{
		if (Source.bIsLoaded)
		{
			Stats.LoadedSources++;
		}
	}

	Stats.TotalConfigs = CountConfigValues(MergedConfig);
	Stats.ValidatedConfigs = Validators.Size();

	// 验证失败计数需要运行验证
	TArray<TString, CMemoryManager> Errors;
	ValidateAllConfigs(Errors);
	Stats.FailedValidations = Errors.Size();

	return Stats;
}

// === 内部实现 ===

void CConfigManager::MergeAllSources()
{
	// 按优先级排序配置源
	TArray<SConfigSource*, CMemoryManager> SortedSources;
	for (auto& Source : ConfigSources)
	{
		if (Source.bIsLoaded)
		{
			SortedSources.Add(&Source);
		}
	}

	// 简单排序（优先级低的在前）
	for (int32_t i = 0; i < SortedSources.Size() - 1; ++i)
	{
		for (int32_t j = i + 1; j < SortedSources.Size(); ++j)
		{
			if (SortedSources[i]->Priority > SortedSources[j]->Priority)
			{
				SConfigSource* Temp = SortedSources[i];
				SortedSources[i] = SortedSources[j];
				SortedSources[j] = Temp;
			}
		}
	}

	// 合并配置
	CConfigObject MergedObject;
	for (const auto* Source : SortedSources)
	{
		if (Source->Data.IsObject())
		{
			MergeConfigObjects(MergedObject, Source->Data.AsObject());
		}
	}

	MergedConfig = CConfigValue(MergedObject);

	// 清除缓存
	ConfigCache.Clear();

	NLOG_CONFIG(Debug, "Merged {} config sources", SortedSources.Size());
}

bool CConfigManager::LoadConfigSource(SConfigSource& Source)
{
	switch (Source.Type)
	{
	case EConfigSourceType::File: {
		if (!std::filesystem::exists(Source.Location.GetData()))
		{
			NLOG_CONFIG(Warning, "Config file does not exist: {}", Source.Location.GetData());
			return false;
		}

		auto ParseResult = ParseJsonFile(Source.Location);
		if (ParseResult.bSuccess)
		{
			Source.Data = ParseResult.Value;
			Source.bIsLoaded = true;
			Source.LastModified = GetFileModificationTime(Source.Location);
			return true;
		}
		else
		{
			NLOG_CONFIG(Error,
			            "Failed to parse JSON file '{}': {}",
			            Source.Location.GetData(),
			            ParseResult.Error.ToString().GetData());
			return false;
		}
	}

	case EConfigSourceType::Environment: {
		Source.Data = ParseEnvironmentVariables(Source.Location);
		Source.bIsLoaded = true;
		return true;
	}

	default:
		return false;
	}
}

CConfigValue CConfigManager::ParseCommandLineArgs(int argc, char* argv[])
{
	CConfigObject Result;

	for (int i = 1; i < argc; ++i)
	{
		TString Arg(argv[i]);

		// 处理 --key=value 格式
		if (Arg.StartsWith("--"))
		{
			TString KeyValue = Arg.Substring(2);
			int32_t EqualPos = KeyValue.IndexOf('=');

			if (EqualPos >= 0)
			{
				TString Key = KeyValue.Substring(0, EqualPos);
				TString Value = KeyValue.Substring(EqualPos + 1);

				// 尝试解析值类型
				CConfigValue ConfigVal = ParseStringValue(Value);
				Result.Add(Key, ConfigVal);
			}
			else
			{
				// 没有值的开关，设为true
				Result.Add(KeyValue, CConfigValue(true));
			}
		}
		// 处理 -key value 格式
		else if (Arg.StartsWith("-") && i + 1 < argc)
		{
			TString Key = Arg.Substring(1);
			TString Value(argv[i + 1]);

			CConfigValue ConfigVal = ParseStringValue(Value);
			Result.Add(Key, ConfigVal);
			i++; // 跳过下一个参数
		}
	}

	NLOG_CONFIG(Debug, "Parsed {} command line arguments", Result.Size());
	return CConfigValue(Result);
}

CConfigValue CConfigManager::ParseEnvironmentVariables(const TString& Prefix)
{
	CConfigObject Result;

	// 获取所有环境变量
	extern char** environ;
	for (char** env = environ; *env != nullptr; ++env)
	{
		TString EnvVar(*env);
		int32_t EqualPos = EnvVar.IndexOf('=');

		if (EqualPos >= 0)
		{
			TString Key = EnvVar.Substring(0, EqualPos);
			TString Value = EnvVar.Substring(EqualPos + 1);

			// 检查前缀匹配
			if (Prefix.IsEmpty() || Key.StartsWith(Prefix))
			{
				// 移除前缀
				if (!Prefix.IsEmpty())
				{
					Key = Key.Substring(Prefix.Length());
					if (Key.StartsWith("_"))
					{
						Key = Key.Substring(1);
					}
				}

				if (!Key.IsEmpty())
				{
					// 将下划线转换为点号以支持嵌套路径
					Key = Key.Replace("_", ".");

					CConfigValue ConfigVal = ParseStringValue(Value);
					Result.Add(Key, ConfigVal);
				}
			}
		}
	}

	NLOG_CONFIG(Debug, "Parsed {} environment variables with prefix '{}'", Result.Size(), Prefix.GetData());
	return CConfigValue(Result);
}

void CConfigManager::FileWatcherThread()
{
	NLOG_CONFIG(Info, "File watcher thread started");

	while (bWatcherThreadRunning.load())
	{
		try
		{
			std::this_thread::sleep_for(
			    std::chrono::milliseconds(static_cast<int64_t>(FileWatchInterval.GetTotalMilliseconds())));

			if (!bWatcherThreadRunning.load())
			{
				break;
			}

			std::lock_guard<std::mutex> Lock(SourcesMutex);

			for (auto& Source : ConfigSources)
			{
				if (Source.Type == EConfigSourceType::File && Source.bAutoReload && IsFileModified(Source))
				{
					NLOG_CONFIG(Info, "Detected file change, reloading: {}", Source.Name.GetData());

					if (LoadConfigSource(Source))
					{
						MergeAllSources();
						OnConfigSourceReloaded.Broadcast(Source.Name);
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			NLOG_CONFIG(Error, "File watcher thread error: {}", e.what());
		}
	}

	NLOG_CONFIG(Info, "File watcher thread stopped");
}

bool CConfigManager::IsFileModified(const SConfigSource& Source) const
{
	if (Source.Type != EConfigSourceType::File)
	{
		return false;
	}

	CDateTime CurrentModTime = GetFileModificationTime(Source.Location);
	return CurrentModTime > Source.LastModified;
}

void CConfigManager::NotifyConfigChanged(const TString& Key,
                                         const CConfigValue& OldValue,
                                         const CConfigValue& NewValue,
                                         const TString& SourceName)
{
	SConfigChangeEvent Event(Key, OldValue, NewValue, SourceName);
	OnConfigChanged.Broadcast(Event);

	NLOG_CONFIG(Debug, "Config changed: {} in source: {}", Key.GetData(), SourceName.GetData());
}

void CConfigManager::ApplyConfigValue(const TString& Key, const CConfigValue& Value, const TString& SourceName)
{
	// 这个方法可以用于应用配置值的副作用，比如更新系统设置等
	// 目前只是记录日志
	NLOG_CONFIG(Verbose,
	            "Applied config value: {} = {} from {}",
	            Key.GetData(),
	            Value.ToString().GetData(),
	            SourceName.GetData());
}

// === 辅助函数 ===

CDateTime CConfigManager::GetFileModificationTime(const TString& FilePath) const
{
	try
	{
		auto FileTime = std::filesystem::last_write_time(FilePath.GetData());
		auto SystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		    FileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

		return CDateTime::FromSystemTime(SystemTime);
	}
	catch (...)
	{
		return CDateTime::Now();
	}
}

TString CConfigManager::GetSourceTypeName(EConfigSourceType Type) const
{
	switch (Type)
	{
	case EConfigSourceType::File:
		return TString("File");
	case EConfigSourceType::CommandLine:
		return TString("CommandLine");
	case EConfigSourceType::Environment:
		return TString("Environment");
	case EConfigSourceType::Memory:
		return TString("Memory");
	case EConfigSourceType::Remote:
		return TString("Remote");
	default:
		return TString("Unknown");
	}
}

CConfigValue CConfigManager::ParseStringValue(const TString& Value) const
{
	// 尝试解析为布尔值
	if (Value.ToLower() == "true" || Value.ToLower() == "yes" || Value == "1")
	{
		return CConfigValue(true);
	}
	if (Value.ToLower() == "false" || Value.ToLower() == "no" || Value == "0")
	{
		return CConfigValue(false);
	}

	// 尝试解析为数字
	try
	{
		if (Value.Contains("."))
		{
			double DoubleVal = std::stod(Value.GetData());
			return CConfigValue(DoubleVal);
		}
		else
		{
			long long IntVal = std::stoll(Value.GetData());
			if (IntVal >= INT32_MIN && IntVal <= INT32_MAX)
			{
				return CConfigValue(static_cast<int32_t>(IntVal));
			}
			else
			{
				return CConfigValue(static_cast<int64_t>(IntVal));
			}
		}
	}
	catch (...)
	{
		// 不是数字，作为字符串处理
	}

	return CConfigValue(Value);
}

void CConfigManager::MergeConfigObjects(CConfigObject& Target, const CConfigObject& Source) const
{
	for (const auto& Pair : Source)
	{
		const TString& Key = Pair.Key;
		const CConfigValue& Value = Pair.Value;

		if (Target.Contains(Key) && Target[Key].IsObject() && Value.IsObject())
		{
			// 递归合并对象
			CConfigObject MergedObj = Target[Key].AsObject();
			MergeConfigObjects(MergedObj, Value.AsObject());
			Target.Add(Key, CConfigValue(MergedObj));
		}
		else
		{
			// 直接覆盖
			Target.Add(Key, Value);
		}
	}
}

void CConfigManager::CollectKeysFromValue(const CConfigValue& Value,
                                          const TString& Prefix,
                                          TArray<TString, CMemoryManager>& OutKeys) const
{
	if (Value.IsObject())
	{
		const auto& Object = Value.AsObject();
		for (const auto& Pair : Object)
		{
			TString FullKey = Prefix.IsEmpty() ? Pair.Key : (Prefix + TString(".") + Pair.Key);
			OutKeys.Add(FullKey);
			CollectKeysFromValue(Pair.Value, FullKey, OutKeys);
		}
	}
	else if (Value.IsArray())
	{
		const auto& Array = Value.AsArray();
		for (int32_t i = 0; i < Array.Size(); ++i)
		{
			TString FullKey = Prefix + TString("[") + TString::FromInt(i) + TString("]");
			OutKeys.Add(FullKey);
			CollectKeysFromValue(Array[i], FullKey, OutKeys);
		}
	}
}

int32_t CConfigManager::CountConfigValues(const CConfigValue& Value) const
{
	int32_t Count = 1;

	if (Value.IsObject())
	{
		const auto& Object = Value.AsObject();
		for (const auto& Pair : Object)
		{
			Count += CountConfigValues(Pair.Value);
		}
	}
	else if (Value.IsArray())
	{
		const auto& Array = Value.AsArray();
		for (const auto& Element : Array)
		{
			Count += CountConfigValues(Element);
		}
	}

	return Count;
}

} // namespace NLib