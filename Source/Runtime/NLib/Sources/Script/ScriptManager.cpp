#include "ScriptManager.h"

#include "IO/FileSystem.h"
#include "Logging/LogCategory.h"

namespace NLib
{
CScriptManager::CScriptManager()
{
	// 设置默认全局配置
	GlobalConfig.Flags = EScriptContextFlags::EnableSandbox | EScriptContextFlags::EnableTimeout;
	GlobalConfig.TimeoutMs = 30000;   // 30秒
	GlobalConfig.MemoryLimitMB = 512; // 512MB
	GlobalConfig.MaxStackDepth = 2000;
}

CScriptManager::~CScriptManager()
{
	if (bInitialized)
	{
		Shutdown();
	}
}

bool CScriptManager::Initialize()
{
	CThreadSafeLock Lock(Mutex);

	if (bInitialized)
	{
		return true;
	}

	NLOG_SCRIPT(Info, "Initializing Script Manager...");

	// 注册内置引擎
	RegisterBuiltinEngines();

	// 添加默认模块搜索路径
	ModulePaths.Add(TEXT("./Scripts"));
	ModulePaths.Add(TEXT("./Modules"));
	ModulePaths.Add(TEXT("Scripts"));
	ModulePaths.Add(TEXT("Modules"));

	// 重置统计信息
	Statistics.Reset();

	bInitialized = true;
	NLOG_SCRIPT(Info, "Script Manager initialized successfully");
	return true;
}

void CScriptManager::Shutdown()
{
	CThreadSafeLock Lock(Mutex);

	if (!bInitialized)
	{
		return;
	}

	NLOG_SCRIPT(Info, "Shutting down Script Manager...");

	// 销毁所有活跃上下文
	DestroyAllContexts();

	// 关闭所有引擎
	for (auto& LanguagePair : EngineRegistry)
	{
		for (auto& Registry : LanguagePair.Value)
		{
			if (Registry.Engine && Registry.Engine->IsInitialized())
			{
				Registry.Engine->Shutdown();
			}
		}
	}

	// 清理注册表
	EngineRegistry.Clear();
	DefaultEngines.Clear();

	// 清理全局绑定
	GlobalFunctions.Clear();
	GlobalObjects.Clear();
	GlobalConstants.Clear();

	// 清理路径
	ModulePaths.Clear();

	bInitialized = false;
	NLOG_SCRIPT(Info, "Script Manager shut down successfully");
}

// === 引擎管理 ===

bool CScriptManager::RegisterEngine(EScriptLanguage Language,
                                    const CString& Name,
                                    TSharedPtr<CScriptEngine> Engine,
                                    bool bSetAsDefault)
{
	if (!Engine)
	{
		NLOG_SCRIPT(Error, "Cannot register null script engine");
		return false;
	}

	CThreadSafeLock Lock(Mutex);

	// 检查是否已经注册
	if (EngineRegistry.Contains(Language))
	{
		auto& Engines = EngineRegistry[Language];
		for (const auto& Registry : Engines)
		{
			if (Registry.Name == Name)
			{
				NLOG_SCRIPT(Warning,
				            "Script engine '{}' for language {} already registered",
				            Name.GetData(),
				            static_cast<int>(Language));
				return false;
			}
		}
	}

	// 初始化引擎
	if (!Engine->IsInitialized())
	{
		if (!Engine->Initialize())
		{
			NLOG_SCRIPT(Error, "Failed to initialize script engine '{}'", Name.GetData());
			return false;
		}
	}

	// 创建注册信息
	SScriptEngineRegistry Registry(Language, Name, Engine);
	Registry.Version = Engine->GetVersion();
	Registry.bIsDefault = bSetAsDefault;

	// 添加到注册表
	if (!EngineRegistry.Contains(Language))
	{
		EngineRegistry.Add(Language, TArray<SScriptEngineRegistry, CMemoryManager>());
	}

	EngineRegistry[Language].Add(Registry);

	// 设置为默认引擎
	if (bSetAsDefault || !DefaultEngines.Contains(Language))
	{
		DefaultEngines.Add(Language, Name);
	}

	NLOG_SCRIPT(Info,
	            "Registered script engine '{}' for language {} (version: {})",
	            Name.GetData(),
	            static_cast<int>(Language),
	            Registry.Version.GetData());

	OnEngineRegistered.Broadcast(Language);
	return true;
}

void CScriptManager::UnregisterEngine(EScriptLanguage Language, const CString& Name)
{
	CThreadSafeLock Lock(Mutex);

	if (!EngineRegistry.Contains(Language))
	{
		return;
	}

	auto& Engines = EngineRegistry[Language];

	if (Name.IsEmpty())
	{
		// 注销该语言的所有引擎
		for (auto& Registry : Engines)
		{
			if (Registry.Engine && Registry.Engine->IsInitialized())
			{
				Registry.Engine->Shutdown();
			}
		}
		Engines.Clear();
		DefaultEngines.Remove(Language);
	}
	else
	{
		// 注销指定名称的引擎
		for (int32_t i = 0; i < Engines.Size(); ++i)
		{
			if (Engines[i].Name == Name)
			{
				if (Engines[i].Engine && Engines[i].Engine->IsInitialized())
				{
					Engines[i].Engine->Shutdown();
				}

				Engines.RemoveAt(i);

				// 如果是默认引擎，重新设置默认引擎
				if (DefaultEngines.Contains(Language) && DefaultEngines[Language] == Name)
				{
					if (!Engines.IsEmpty())
					{
						DefaultEngines[Language] = Engines[0].Name;
					}
					else
					{
						DefaultEngines.Remove(Language);
					}
				}
				break;
			}
		}
	}

	NLOG_SCRIPT(Info, "Unregistered script engine '{}' for language {}", Name.GetData(), static_cast<int>(Language));
	OnEngineUnregistered.Broadcast(Language);
}

TSharedPtr<CScriptEngine> CScriptManager::GetEngine(EScriptLanguage Language, const CString& Name) const
{
	CThreadSafeLock Lock(Mutex);

	if (!EngineRegistry.Contains(Language))
	{
		return nullptr;
	}

	const auto& Engines = EngineRegistry.Find(Language)->Value;

	if (Name.IsEmpty())
	{
		// 返回默认引擎
		return GetDefaultEngine(Language);
	}

	// 查找指定名称的引擎
	for (const auto& Registry : Engines)
	{
		if (Registry.Name == Name)
		{
			return Registry.Engine;
		}
	}

	return nullptr;
}

TSharedPtr<CScriptEngine> CScriptManager::GetDefaultEngine(EScriptLanguage Language) const
{
	CThreadSafeLock Lock(Mutex);

	if (!DefaultEngines.Contains(Language))
	{
		return nullptr;
	}

	const CString& DefaultName = DefaultEngines.Find(Language)->Value;
	return GetEngine(Language, DefaultName);
}

bool CScriptManager::SetDefaultEngine(EScriptLanguage Language, const CString& Name)
{
	CThreadSafeLock Lock(Mutex);

	// 检查引擎是否存在
	auto Engine = GetEngine(Language, Name);
	if (!Engine)
	{
		NLOG_SCRIPT(Error,
		            "Cannot set default engine: engine '{}' not found for language {}",
		            Name.GetData(),
		            static_cast<int>(Language));
		return false;
	}

	DefaultEngines.Add(Language, Name);
	NLOG_SCRIPT(Info, "Set default script engine for language {} to '{}'", static_cast<int>(Language), Name.GetData());
	return true;
}

TArray<SScriptEngineRegistry, CMemoryManager> CScriptManager::GetRegisteredEngines() const
{
	CThreadSafeLock Lock(Mutex);

	TArray<SScriptEngineRegistry, CMemoryManager> Result;

	for (const auto& LanguagePair : EngineRegistry)
	{
		for (const auto& Registry : LanguagePair.Value)
		{
			Result.Add(Registry);
		}
	}

	return Result;
}

bool CScriptManager::IsLanguageSupported(EScriptLanguage Language) const
{
	CThreadSafeLock Lock(Mutex);
	return EngineRegistry.Contains(Language) && !EngineRegistry.Find(Language)->Value.IsEmpty();
}

TArray<EScriptLanguage, CMemoryManager> CScriptManager::GetSupportedLanguages() const
{
	CThreadSafeLock Lock(Mutex);

	TArray<EScriptLanguage, CMemoryManager> Result;
	for (const auto& LanguagePair : EngineRegistry)
	{
		if (!LanguagePair.Value.IsEmpty())
		{
			Result.Add(LanguagePair.Key);
		}
	}

	return Result;
}

// === 上下文管理 ===

TSharedPtr<CScriptContext> CScriptManager::CreateContext(const SScriptConfig& Config)
{
	auto Engine = GetDefaultEngine(Config.Language);
	if (!Engine)
	{
		NLOG_SCRIPT(Error, "No engine available for script language {}", static_cast<int>(Config.Language));
		return nullptr;
	}

	// 合并配置
	auto MergedConfig = MergeConfig(Config);

	auto Context = Engine->CreateContext(MergedConfig);
	if (!Context)
	{
		NLOG_SCRIPT(Error, "Failed to create script context for language {}", static_cast<int>(Config.Language));
		return nullptr;
	}

	// 应用全局绑定
	ApplyGlobalBindings(Context);

	// 注册上下文
	CThreadSafeLock Lock(Mutex);
	CString ContextId = GenerateContextId();
	ActiveContexts.Add(ContextId, Context);
	Statistics.ActiveContexts++;

	NLOG_SCRIPT(
	    Debug, "Created script context {} for language {}", ContextId.GetData(), static_cast<int>(Config.Language));

	OnContextCreated.Broadcast(Context);
	return Context;
}

TSharedPtr<CScriptContext> CScriptManager::CreateContext(EScriptLanguage Language, EScriptContextFlags Flags)
{
	SScriptConfig Config(Language);
	Config.Flags = Flags;
	return CreateContext(Config);
}

void CScriptManager::DestroyContext(TSharedPtr<CScriptContext> Context)
{
	if (!Context)
	{
		return;
	}

	CThreadSafeLock Lock(Mutex);

	// 从活跃列表中移除
	for (auto It = ActiveContexts.CreateIterator(); It; ++It)
	{
		if (It->Value == Context)
		{
			ActiveContexts.Remove(It->Key);
			Statistics.ActiveContexts--;
			break;
		}
	}

	// 关闭上下文
	if (Context->IsInitialized())
	{
		Context->Shutdown();
	}

	OnContextDestroyed.Broadcast(Context);
}

TArray<TSharedPtr<CScriptContext>, CMemoryManager> CScriptManager::GetActiveContexts() const
{
	CThreadSafeLock Lock(Mutex);

	TArray<TSharedPtr<CScriptContext>, CMemoryManager> Result;
	for (const auto& ContextPair : ActiveContexts)
	{
		Result.Add(ContextPair.Value);
	}

	return Result;
}

void CScriptManager::DestroyAllContexts()
{
	CThreadSafeLock Lock(Mutex);

	for (auto& ContextPair : ActiveContexts)
	{
		if (ContextPair.Value && ContextPair.Value->IsInitialized())
		{
			ContextPair.Value->Shutdown();
		}
	}

	ActiveContexts.Clear();
	Statistics.ActiveContexts = 0;
}

// === 便利执行函数 ===

SScriptExecutionResult CScriptManager::ExecuteString(EScriptLanguage Language,
                                                     const CString& Code,
                                                     const SScriptConfig& Config)
{
	auto Context = CreateContext(Config.Language != EScriptLanguage::None ? Config : SScriptConfig(Language));
	if (!Context)
	{
		return SScriptExecutionResult(EScriptResult::EngineNotFound, TEXT("No engine available"));
	}

	auto Result = Context->ExecuteString(Code);

	// 更新统计
	CThreadSafeLock Lock(Mutex);
	Statistics.ExecutionCount++;
	Statistics.TotalExecutionTime += Result.ExecutionTimeMs;
	if (!Result.IsSuccess())
	{
		Statistics.ErrorCount++;
		if (Result.Result == EScriptResult::TimeoutError)
		{
			Statistics.TimeoutCount++;
		}
	}

	DestroyContext(Context);
	return Result;
}

SScriptExecutionResult CScriptManager::ExecuteFile(EScriptLanguage Language,
                                                   const CString& FilePath,
                                                   const SScriptConfig& Config)
{
	// 检查文件是否存在
	if (!NFileSystem::FileExists(FilePath))
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument,
		                              CString(TEXT("Script file not found: ")) + FilePath);
	}

	auto Context = CreateContext(Config.Language != EScriptLanguage::None ? Config : SScriptConfig(Language));
	if (!Context)
	{
		return SScriptExecutionResult(EScriptResult::EngineNotFound, TEXT("No engine available"));
	}

	auto Result = Context->ExecuteFile(FilePath);

	// 更新统计
	CThreadSafeLock Lock(Mutex);
	Statistics.ExecutionCount++;
	Statistics.TotalExecutionTime += Result.ExecutionTimeMs;
	if (!Result.IsSuccess())
	{
		Statistics.ErrorCount++;
		if (Result.Result == EScriptResult::TimeoutError)
		{
			Statistics.TimeoutCount++;
		}
	}

	DestroyContext(Context);
	return Result;
}

SScriptExecutionResult CScriptManager::CheckSyntax(EScriptLanguage Language, const CString& Code)
{
	auto Engine = GetDefaultEngine(Language);
	if (!Engine)
	{
		return SScriptExecutionResult(EScriptResult::EngineNotFound, TEXT("No engine available"));
	}

	return Engine->CheckSyntax(Code);
}

SScriptExecutionResult CScriptManager::CompileFile(EScriptLanguage Language,
                                                   const CString& FilePath,
                                                   const CString& OutputPath)
{
	auto Engine = GetDefaultEngine(Language);
	if (!Engine)
	{
		return SScriptExecutionResult(EScriptResult::EngineNotFound, TEXT("No engine available"));
	}

	return Engine->CompileFile(FilePath, OutputPath);
}

// === 内部实现 ===

void CScriptManager::RegisterBuiltinEngines()
{
	// 这里会在各个具体引擎实现后注册
	// 例如：RegisterEngine(EScriptLanguage::Lua, TEXT("LuaEngine"), MakeShared<CLuaEngine>(), true);
	NLOG_SCRIPT(Debug, "Built-in script engines registration completed");
}

void CScriptManager::ApplyGlobalBindings(TSharedPtr<CScriptContext> Context)
{
	if (!Context)
	{
		return;
	}

	// 应用全局函数
	for (const auto& FunctionPair : GlobalFunctions)
	{
		Context->RegisterGlobalFunction(FunctionPair.Key, FunctionPair.Value);
	}

	// 应用全局对象
	for (const auto& ObjectPair : GlobalObjects)
	{
		Context->RegisterGlobalObject(ObjectPair.Key, ObjectPair.Value);
	}

	// 应用全局常量
	for (const auto& ConstantPair : GlobalConstants)
	{
		Context->RegisterGlobalConstant(ConstantPair.Key, ConstantPair.Value);
	}
}

CString CScriptManager::GenerateContextId() const
{
	return CString(TEXT("ScriptContext_")) + CString::FromInt(NextContextId++);
}

SScriptConfig CScriptManager::MergeConfig(const SScriptConfig& UserConfig) const
{
	SScriptConfig MergedConfig = GlobalConfig;

	// 用户配置覆盖全局配置
	if (UserConfig.Language != EScriptLanguage::None)
	{
		MergedConfig.Language = UserConfig.Language;
	}

	if (UserConfig.Flags != EScriptContextFlags::None)
	{
		MergedConfig.Flags = UserConfig.Flags;
	}

	if (UserConfig.TimeoutMs > 0)
	{
		MergedConfig.TimeoutMs = UserConfig.TimeoutMs;
	}

	if (UserConfig.MemoryLimitMB > 0)
	{
		MergedConfig.MemoryLimitMB = UserConfig.MemoryLimitMB;
	}

	if (UserConfig.MaxStackDepth > 0)
	{
		MergedConfig.MaxStackDepth = UserConfig.MaxStackDepth;
	}

	if (!UserConfig.WorkingDirectory.IsEmpty())
	{
		MergedConfig.WorkingDirectory = UserConfig.WorkingDirectory;
	}

	// 合并模块路径
	MergedConfig.ModulePaths.Append(ModulePaths);
	MergedConfig.ModulePaths.Append(UserConfig.ModulePaths);

	// 合并环境变量
	for (const auto& EnvPair : UserConfig.EnvironmentVariables)
	{
		MergedConfig.EnvironmentVariables.Add(EnvPair.Key, EnvPair.Value);
	}

	return MergedConfig;
}

// === 其他功能实现（简化版本，实际实现会更复杂） ===

void CScriptManager::RegisterGlobalFunction(const CString& Name, TSharedPtr<CScriptFunction> Function)
{
	CThreadSafeLock Lock(Mutex);
	GlobalFunctions.Add(Name, Function);
}

void CScriptManager::RegisterGlobalObject(const CString& Name, const CScriptValue& Object)
{
	CThreadSafeLock Lock(Mutex);
	GlobalObjects.Add(Name, Object);
}

void CScriptManager::RegisterGlobalConstant(const CString& Name, const CScriptValue& Value)
{
	CThreadSafeLock Lock(Mutex);
	GlobalConstants.Add(Name, Value);
}

SScriptStatistics CScriptManager::GetStatistics() const
{
	CThreadSafeLock Lock(Mutex);
	return Statistics;
}

void CScriptManager::CollectGarbage()
{
	auto Contexts = GetActiveContexts();
	for (auto& Context : Contexts)
	{
		Context->CollectGarbage();
	}
}

uint64_t CScriptManager::GetTotalMemoryUsage() const
{
	uint64_t TotalMemory = 0;
	auto Contexts = GetActiveContexts();
	for (auto& Context : Contexts)
	{
		TotalMemory += Context->GetMemoryUsage();
	}
	return TotalMemory;
}

} // namespace NLib