#include "ConfigManager.h"

#include "IO/FileSystem.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace NLib
{
using SizeType = std::size_t;

// === NConfigManager 初始化和关闭 ===

bool NConfigManager::Initialize()
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

void NConfigManager::Shutdown()
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

bool NConfigManager::AddJsonFile(const CString& Name, const CString& FilePath, EConfigPriority Priority, bool bOptional)
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

	ConfigSources.PushBack(NewSource);

	// 重新合并配置
	MergeAllSources();

	// 启动文件监控
	if (bAutoReloadEnabled.load() && !bWatcherThreadRunning.load())
	{
		bWatcherThreadRunning.store(true);
		FileWatcherThread_ = std::thread(&NConfigManager::FileWatcherThread, this);
	}

	NLOG_CONFIG(Info, "Successfully added JSON config source: {}", Name.GetData());
	return true;
}

bool NConfigManager::AddEnvironmentVariables(const CString& Prefix, EConfigPriority Priority)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Adding environment variables config source with prefix: {}", Prefix.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	CString SourceName = CString("Environment_") + (Prefix.IsEmpty() ? CString("All") : Prefix);

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

	ConfigSources.PushBack(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added environment variables config source");
	return true;
}

bool NConfigManager::AddCommandLineArgs(int argc, char* argv[], EConfigPriority Priority)
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
	SConfigSource NewSource(CString("CommandLine"), EConfigSourceType::CommandLine, CString(), Priority);
	NewSource.bAutoReload = false; // 命令行参数不支持自动重载
	NewSource.Data = ParseCommandLineArgs(argc, argv);
	NewSource.bIsLoaded = true;

	ConfigSources.PushBack(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added command line arguments config source");
	return true;
}

bool NConfigManager::AddMemoryConfig(const CString& Name, const CConfigValue& Config, EConfigPriority Priority)
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
	SConfigSource NewSource(Name, EConfigSourceType::Memory, CString(), Priority);
	NewSource.bAutoReload = false; // 内存配置不支持自动重载
	NewSource.Data = Config;
	NewSource.bIsLoaded = true;

	ConfigSources.PushBack(NewSource);

	// 重新合并配置
	MergeAllSources();

	NLOG_CONFIG(Info, "Successfully added memory config source: {}", Name.GetData());
	return true;
}

bool NConfigManager::RemoveConfigSource(const CString& Name)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Removing config source: {}", Name.GetData());

	std::lock_guard<std::mutex> Lock(SourcesMutex);

	for (SizeType i = 0; i < ConfigSources.Size(); ++i)
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

bool NConfigManager::ReloadConfigSource(const CString& Name)
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

void NConfigManager::ReloadAllSources()
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

CConfigValue NConfigManager::GetConfig(const CString& Key, const CConfigValue& DefaultValue) const
{
	if (!bIsInitialized.load())
	{
		return DefaultValue;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	// 首先检查缓存
	if (ConfigCache.Contains(Key))
	{
		return *ConfigCache.Find(Key);
	}

	// 从合并配置中获取
	const CConfigValue& Value = MergedConfig.GetByPath(Key);
	if (!Value.IsNull())
	{
		// 缓存结果
		const_cast<NConfigManager*>(this)->ConfigCache.Insert(Key, Value);
		return Value;
	}

	return DefaultValue;
}

void NConfigManager::SetConfig(const CString& Key, const CConfigValue& Value, const CString& SourceName)
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
		SConfigSource NewSource(SourceName, EConfigSourceType::Memory, CString(), EConfigPriority::High);
		NewSource.bAutoReload = false;
		NewSource.Data = CConfigObject();
		NewSource.bIsLoaded = true;
		ConfigSources.PushBack(NewSource);
		TargetSource = &ConfigSources[ConfigSources.Size() - 1];
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

bool NConfigManager::HasConfig(const CString& Key) const
{
	if (!bIsInitialized.load())
	{
		return false;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	return MergedConfig.HasPath(Key);
}

TArray<CString, CMemoryManager> NConfigManager::GetAllKeys() const
{
	TArray<CString, CMemoryManager> Keys;

	if (!bIsInitialized.load())
	{
		return Keys;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	// 从所有配置源收集键
	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);
	for (const auto& Source : ConfigSources)
	{
		CollectKeysFromValue(Source.Data, CString(), Keys);
	}

	// 去重
	for (SizeType i = 0; i < Keys.Size() - 1; ++i)
	{
		for (SizeType j = i + 1; j < Keys.Size(); ++j)
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

CConfigObject NConfigManager::GetConfigsWithPrefix(const CString& Prefix) const
{
	CConfigObject Result;

	if (!bIsInitialized.load())
	{
		return Result;
	}

	auto AllKeys = GetAllKeys();
	for (const auto& Key : AllKeys)
	{
		if (Key.Find(Prefix.GetData()) == 0)  // StartsWith equivalent
		{
			CString RelativeKey = Key.SubString(Prefix.Size());
			if (RelativeKey.Find(".") == 0)  // StartsWith(".") equivalent
			{
				RelativeKey = RelativeKey.SubString(1);
			}

			if (!RelativeKey.IsEmpty())
			{
				Result.Insert(RelativeKey, GetConfig(Key));
			}
		}
	}

	return Result;
}

// === 配置验证 ===

void NConfigManager::AddValidator(const CString& Key, TSharedPtr<IConfigValidator> Validator)
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return;
	}

	NLOG_CONFIG(Info, "Adding validator for key: {}", Key.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	Validators.Insert(Key, Validator);
}

void NConfigManager::RemoveValidator(const CString& Key)
{
	if (!bIsInitialized.load())
	{
		return;
	}

	NLOG_CONFIG(Info, "Removing validator for key: {}", Key.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	Validators.Remove(Key);
}

bool NConfigManager::ValidateAllConfigs(TArray<CString, CMemoryManager>& OutErrors) const
{
	OutErrors.Clear();

	if (!bIsInitialized.load())
	{
		OutErrors.PushBack("ConfigManager not initialized");
		return false;
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	bool bAllValid = true;

	for (const auto& ValidatorPair : Validators)
	{
		const CString& Key = ValidatorPair.first;
		const auto& Validator = ValidatorPair.second;

		CConfigValue Value = GetConfig(Key);
		CString ErrorMessage;

		if (!Validator->Validate(Key, Value, ErrorMessage))
		{
			OutErrors.PushBack(CString("Validation failed for '") + Key + CString("': ") + ErrorMessage);
			bAllValid = false;

			// 触发验证失败事件
			const_cast<NConfigManager*>(this)->OnConfigValidationFailed.Broadcast(Key, ErrorMessage);
		}
	}

	return bAllValid;
}

bool NConfigManager::ValidateConfig(const CString& Key, CString& OutError) const
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

	const auto& Validator = *Validators.Find(Key);
	CConfigValue Value = GetConfig(Key);

	if (!Validator->Validate(Key, Value, OutError))
	{
		// 触发验证失败事件
		const_cast<NConfigManager*>(this)->OnConfigValidationFailed.Broadcast(Key, OutError);
		return false;
	}

	return true;
}

// === 配置监控 ===

void NConfigManager::SetAutoReloadEnabled(bool bEnabled)
{
	bAutoReloadEnabled.store(bEnabled);

	if (bEnabled && !bWatcherThreadRunning.load() && bIsInitialized.load())
	{
		bWatcherThreadRunning.store(true);
		FileWatcherThread_ = std::thread(&NConfigManager::FileWatcherThread, this);
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

void NConfigManager::SetFileWatchInterval(const CTimespan& Interval)
{
	FileWatchInterval = Interval;
	NLOG_CONFIG(Info, "File watch interval set to {} seconds", Interval.GetTotalSeconds());
}

TArray<SConfigSource, CMemoryManager> NConfigManager::GetConfigSources() const
{
	if (!bIsInitialized.load())
	{
		return TArray<SConfigSource, CMemoryManager>();
	}

	std::lock_guard<std::mutex> Lock(SourcesMutex);
	// 创建新的数组并拷贝内容
	TArray<SConfigSource, CMemoryManager> Result;
	Result.Reserve(ConfigSources.Size());
	for (const auto& Source : ConfigSources)
	{
		Result.PushBack(Source);
	}
	return Result;
}

CConfigValue NConfigManager::GetMergedConfig() const
{
	if (!bIsInitialized.load())
	{
		return CConfigValue();
	}

	std::lock_guard<std::mutex> Lock(ConfigMutex);
	return MergedConfig;
}

// === 调试和诊断 ===

CString NConfigManager::GenerateConfigReport() const
{
	if (!bIsInitialized.load())
	{
		return CString("ConfigManager not initialized");
	}

	CString Report = CString("=== Configuration Report ===\n\n");

	std::lock_guard<std::mutex> SourcesLock(SourcesMutex);

	Report += CString("Config Sources (") + CString::FromInt(ConfigSources.Size()) + CString("):\n");
	for (const auto& Source : ConfigSources)
	{
		Report += CString("  - ") + Source.Name;
		Report += CString(" (") + GetSourceTypeName(Source.Type) + CString(")");
		Report += CString(" Priority: ") + CString::FromInt(static_cast<int>(Source.Priority));
		Report += CString(" Loaded: ") + (Source.bIsLoaded ? CString("Yes") : CString("No"));
		if (!Source.Location.IsEmpty())
		{
			Report += CString(" Location: ") + Source.Location;
		}
		Report += CString("\n");
	}

	Report += CString("\nValidators (") + CString::FromInt(Validators.Size()) + CString("):\n");
	std::lock_guard<std::mutex> ConfigLock(ConfigMutex);
	for (const auto& ValidatorPair : Validators)
	{
		Report += CString("  - ") + ValidatorPair.first;
		Report += CString(": ") + ValidatorPair.second->GetDescription();
		Report += CString("\n");
	}

	Report += CString("\nCache Entries: ") + CString::FromInt(ConfigCache.Size()) + CString("\n");
	Report += CString("Auto Reload: ") + (bAutoReloadEnabled.load() ? CString("Enabled") : CString("Disabled")) +
	          CString("\n");
	Report += CString("File Watch Interval: ") + CString::FromDouble(FileWatchInterval.GetTotalSeconds()) +
	          CString(" seconds\n");

	return Report;
}

bool NConfigManager::ExportConfig(const CString& FilePath, bool bPrettyPrint) const
{
	if (!bIsInitialized.load())
	{
		NLOG_CONFIG(Error, "ConfigManager not initialized");
		return false;
	}

	NLOG_CONFIG(Info, "Exporting config to file: {}", FilePath.GetData());

	std::lock_guard<std::mutex> Lock(ConfigMutex);

	CString JsonString = MergedConfig.ToJsonString(bPrettyPrint);

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

NConfigManager::SConfigStats NConfigManager::GetConfigStats() const
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
	TArray<CString, CMemoryManager> Errors;
	ValidateAllConfigs(Errors);
	Stats.FailedValidations = Errors.Size();

	return Stats;
}

// === 内部实现 ===

void NConfigManager::MergeAllSources()
{
	// 按优先级排序配置源
	TArray<SConfigSource*, CMemoryManager> SortedSources;
	for (auto& Source : ConfigSources)
	{
		if (Source.bIsLoaded)
		{
			SortedSources.PushBack(&Source);
		}
	}

	// 简单排序（优先级低的在前）
	for (SizeType i = 0; i < SortedSources.Size() - 1; ++i)
	{
		for (SizeType j = i + 1; j < SortedSources.Size(); ++j)
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

bool NConfigManager::LoadConfigSource(SConfigSource& Source)
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

CConfigValue NConfigManager::ParseCommandLineArgs(int argc, char* argv[])
{
	CConfigObject Result;

	for (int i = 1; i < argc; ++i)
	{
		CString Arg(argv[i]);

		// 处理 --key=value 格式
		if (Arg.Find("--") == 0)  // StartsWith("--") equivalent
		{
			CString KeyValue = Arg.SubString(2);
			auto EqualPos = KeyValue.Find('=');

			if (EqualPos != CString::NoPosition)
			{
				CString Key = KeyValue.SubString(0, EqualPos);
				CString Value = KeyValue.SubString(EqualPos + 1);

				// 尝试解析值类型
				CConfigValue ConfigVal = ParseStringValue(Value);
				Result.Insert(Key, ConfigVal);
			}
			else
			{
				// 没有值的开关，设为true
				Result.Insert(KeyValue, CConfigValue(true));
			}
		}
		// 处理 -key value 格式
		else if (Arg.StartsWith("-") && i + 1 < argc)
		{
			CString Key = Arg.SubString(1);
			CString Value(argv[i + 1]);

			CConfigValue ConfigVal = ParseStringValue(Value);
			Result.Insert(Key, ConfigVal);
			i++; // 跳过下一个参数
		}
	}

	NLOG_CONFIG(Debug, "Parsed {} command line arguments", Result.Size());
	return CConfigValue(Result);
}

CConfigValue NConfigManager::ParseEnvironmentVariables(const CString& Prefix)
{
	CConfigObject Result;

	// 获取所有环境变量
	extern char** environ;
	for (char** env = environ; *env != nullptr; ++env)
	{
		CString EnvVar(*env);
		int32_t EqualPos = EnvVar.IndexOf('=');

		if (EqualPos >= 0)
		{
			CString Key = EnvVar.SubString(0, EqualPos);
			CString Value = EnvVar.SubString(EqualPos + 1);

			// 检查前缀匹配
			if (Prefix.IsEmpty() || Key.StartsWith(Prefix))
			{
				// 移除前缀
				if (!Prefix.IsEmpty())
				{
					Key = Key.SubString(Prefix.Length());
					if (Key.StartsWith("_"))
					{
						Key = Key.SubString(1);
					}
				}

				if (!Key.IsEmpty())
				{
					// 将下划线转换为点号以支持嵌套路径
					Key = Key.Replace("_", ".");

					CConfigValue ConfigVal = ParseStringValue(Value);
					Result.Insert(Key, ConfigVal);
				}
			}
		}
	}

	NLOG_CONFIG(Debug, "Parsed {} environment variables with prefix '{}'", Result.Size(), Prefix.GetData());
	return CConfigValue(Result);
}

void NConfigManager::FileWatcherThread()
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

bool NConfigManager::IsFileModified(const SConfigSource& Source) const
{
	if (Source.Type != EConfigSourceType::File)
	{
		return false;
	}

	CDateTime CurrentModTime = GetFileModificationTime(Source.Location);
	return CurrentModTime > Source.LastModified;
}

void NConfigManager::NotifyConfigChanged(const CString& Key,
                                         const CConfigValue& OldValue,
                                         const CConfigValue& NewValue,
                                         const CString& SourceName)
{
	SConfigChangeEvent Event(Key, OldValue, NewValue, SourceName);
	OnConfigChanged.Broadcast(Event);

	NLOG_CONFIG(Debug, "Config changed: {} in source: {}", Key.GetData(), SourceName.GetData());
}

void NConfigManager::ApplyConfigValue(const CString& Key, const CConfigValue& Value, const CString& SourceName)
{
	// 这个方法可以用于应用配置值的副作用，比如更新系统设置等
	// 目前只是记录日志
	NLOG_CONFIG(Debug,
	            "Applied config value: {} = {} from {}",
	            Key.GetData(),
	            Value.AsString().GetData(),
	            SourceName.GetData());
}

// === 辅助函数 ===

CDateTime NConfigManager::GetFileModificationTime(const CString& FilePath) const
{
	try
	{
		// 简化实现，直接返回当前时间
		// TODO: 实现正确的文件时间获取逻辑
		return CDateTime::Now();
	}
	catch (...)
	{
		return CDateTime::Now();
	}
}

CString NConfigManager::GetSourceTypeName(EConfigSourceType Type) const
{
	switch (Type)
	{
	case EConfigSourceType::File:
		return CString("File");
	case EConfigSourceType::CommandLine:
		return CString("CommandLine");
	case EConfigSourceType::Environment:
		return CString("Environment");
	case EConfigSourceType::Memory:
		return CString("Memory");
	case EConfigSourceType::Remote:
		return CString("Remote");
	default:
		return CString("Unknown");
	}
}

CConfigValue NConfigManager::ParseStringValue(const CString& Value) const
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

void NConfigManager::MergeConfigObjects(CConfigObject& Target, const CConfigObject& Source) const
{
	for (const auto& Pair : Source)
	{
		const CString& Key = Pair.first;
		const CConfigValue& Value = Pair.second;

		if (Target.Contains(Key) && Target[Key].IsObject() && Value.IsObject())
		{
			// 递归合并对象
			CConfigObject MergedObj = std::move(Target[Key].AsObject());
			MergeConfigObjects(MergedObj, Value.AsObject());
			Target.Insert(Key, CConfigValue(std::move(MergedObj)));
		}
		else
		{
			// 直接覆盖
			Target.Insert(Key, Value);
		}
	}
}

void NConfigManager::CollectKeysFromValue(const CConfigValue& Value,
                                          const CString& Prefix,
                                          TArray<CString>& OutKeys) const
{
	if (Value.IsObject())
	{
		const auto& Object = Value.AsObject();
		for (const auto& Pair : Object)
		{
			CString FullKey = Prefix.IsEmpty() ? Pair.first : (Prefix + CString(".") + Pair.first);
			OutKeys.PushBack(FullKey);
			CollectKeysFromValue(Pair.second, FullKey, OutKeys);
		}
	}
	else if (Value.IsArray())
	{
		const auto& Array = Value.AsArray();
		for (SizeType i = 0; i < Array.Size(); ++i)
		{
			CString FullKey = Prefix + CString("[") + CString::FromInt(static_cast<int32_t>(i)) + CString("]");
			OutKeys.PushBack(FullKey);
			CollectKeysFromValue(Array[i], FullKey, OutKeys);
		}
	}
}

int32_t NConfigManager::CountConfigValues(const CConfigValue& Value) const
{
	int32_t Count = 1;

	if (Value.IsObject())
	{
		const auto& Object = Value.AsObject();
		for (const auto& Pair : Object)
		{
			Count += CountConfigValues(Pair.second);
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