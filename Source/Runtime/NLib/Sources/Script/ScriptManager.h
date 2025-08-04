#pragma once

#include "Containers/THashMap.h"
#include "Core/Singleton.h"
#include "ScriptEngine.h"
#include "Threading/ThreadSafe.h"

namespace NLib
{
/**
 * @brief 脚本引擎注册信息
 */
struct SScriptEngineRegistry
{
	EScriptLanguage Language;
	TString Name;
	TString Version;
	TString Description;
	TSharedPtr<CScriptEngine> Engine;
	bool bIsDefault = false;

	SScriptEngineRegistry() = default;
	SScriptEngineRegistry(EScriptLanguage InLanguage, const TString& InName, TSharedPtr<CScriptEngine> InEngine)
	    : Language(InLanguage),
	      Name(InName),
	      Engine(InEngine)
	{}
};

/**
 * @brief 脚本统计信息
 */
struct SScriptStatistics
{
	uint32_t ActiveContexts = 0;
	uint32_t ActiveModules = 0;
	uint64_t TotalMemoryUsed = 0;
	uint64_t TotalExecutionTime = 0;
	uint32_t ExecutionCount = 0;
	uint32_t ErrorCount = 0;
	uint32_t TimeoutCount = 0;

	void Reset()
	{
		ActiveContexts = 0;
		ActiveModules = 0;
		TotalMemoryUsed = 0;
		TotalExecutionTime = 0;
		ExecutionCount = 0;
		ErrorCount = 0;
		TimeoutCount = 0;
	}
};

/**
 * @brief 脚本管理器
 *
 * 管理所有脚本引擎的注册、创建和销毁
 * 提供统一的脚本执行接口
 */
class CScriptManager : public CSingleton<CScriptManager>
{
	GENERATED_BODY()

public:
	CScriptManager();
	~CScriptManager() override;

	// === 引擎管理 ===

	/**
	 * @brief 注册脚本引擎
	 */
	bool RegisterEngine(EScriptLanguage Language,
	                    const TString& Name,
	                    TSharedPtr<CScriptEngine> Engine,
	                    bool bSetAsDefault = false);

	/**
	 * @brief 注销脚本引擎
	 */
	void UnregisterEngine(EScriptLanguage Language, const TString& Name = TString());

	/**
	 * @brief 获取脚本引擎
	 */
	TSharedPtr<CScriptEngine> GetEngine(EScriptLanguage Language, const TString& Name = TString()) const;

	/**
	 * @brief 获取默认脚本引擎
	 */
	TSharedPtr<CScriptEngine> GetDefaultEngine(EScriptLanguage Language) const;

	/**
	 * @brief 设置默认脚本引擎
	 */
	bool SetDefaultEngine(EScriptLanguage Language, const TString& Name);

	/**
	 * @brief 获取所有已注册的引擎
	 */
	TArray<SScriptEngineRegistry, CMemoryManager> GetRegisteredEngines() const;

	/**
	 * @brief 检查语言是否支持
	 */
	bool IsLanguageSupported(EScriptLanguage Language) const;

	/**
	 * @brief 获取支持的语言列表
	 */
	TArray<EScriptLanguage, CMemoryManager> GetSupportedLanguages() const;

	// === 上下文管理 ===

	/**
	 * @brief 创建脚本上下文
	 */
	TSharedPtr<CScriptContext> CreateContext(const SScriptConfig& Config);

	/**
	 * @brief 创建脚本上下文（使用默认引擎）
	 */
	TSharedPtr<CScriptContext> CreateContext(EScriptLanguage Language,
	                                         EScriptContextFlags Flags = EScriptContextFlags::None);

	/**
	 * @brief 销毁脚本上下文
	 */
	void DestroyContext(TSharedPtr<CScriptContext> Context);

	/**
	 * @brief 获取活跃的上下文列表
	 */
	TArray<TSharedPtr<CScriptContext>, CMemoryManager> GetActiveContexts() const;

	/**
	 * @brief 销毁所有上下文
	 */
	void DestroyAllContexts();

	// === 便利执行函数 ===

	/**
	 * @brief 执行脚本字符串
	 */
	SScriptExecutionResult ExecuteString(EScriptLanguage Language,
	                                     const TString& Code,
	                                     const SScriptConfig& Config = SScriptConfig());

	/**
	 * @brief 执行脚本文件
	 */
	SScriptExecutionResult ExecuteFile(EScriptLanguage Language,
	                                   const TString& FilePath,
	                                   const SScriptConfig& Config = SScriptConfig());

	/**
	 * @brief 检查脚本语法
	 */
	SScriptExecutionResult CheckSyntax(EScriptLanguage Language, const TString& Code);

	/**
	 * @brief 编译脚本文件
	 */
	SScriptExecutionResult CompileFile(EScriptLanguage Language,
	                                   const TString& FilePath,
	                                   const TString& OutputPath = TString());

	// === 全局绑定管理 ===

	/**
	 * @brief 注册全局函数
	 */
	void RegisterGlobalFunction(const TString& Name, TSharedPtr<CScriptFunction> Function);

	/**
	 * @brief 注册全局对象
	 */
	void RegisterGlobalObject(const TString& Name, const CScriptValue& Object);

	/**
	 * @brief 注册全局常量
	 */
	void RegisterGlobalConstant(const TString& Name, const CScriptValue& Value);

	/**
	 * @brief 移除全局绑定
	 */
	void UnregisterGlobal(const TString& Name);

	/**
	 * @brief 获取全局绑定列表
	 */
	TArray<TString, CMemoryManager> GetGlobalBindings() const;

	// === 模块管理 ===

	/**
	 * @brief 添加模块搜索路径
	 */
	void AddModulePath(const TString& Path);

	/**
	 * @brief 移除模块搜索路径
	 */
	void RemoveModulePath(const TString& Path);

	/**
	 * @brief 获取模块搜索路径
	 */
	TArray<TString, CMemoryManager> GetModulePaths() const;

	/**
	 * @brief 清空模块搜索路径
	 */
	void ClearModulePaths();

	/**
	 * @brief 预加载模块
	 */
	SScriptExecutionResult PreloadModule(EScriptLanguage Language, const TString& ModulePath);

	// === 统计和监控 ===

	/**
	 * @brief 获取统计信息
	 */
	SScriptStatistics GetStatistics() const;

	/**
	 * @brief 重置统计信息
	 */
	void ResetStatistics();

	/**
	 * @brief 强制垃圾回收所有上下文
	 */
	void CollectGarbage();

	/**
	 * @brief 获取内存使用情况
	 */
	uint64_t GetTotalMemoryUsage() const;

	// === 配置管理 ===

	/**
	 * @brief 设置全局配置
	 */
	void SetGlobalConfig(const SScriptConfig& Config);

	/**
	 * @brief 获取全局配置
	 */
	SScriptConfig GetGlobalConfig() const;

	/**
	 * @brief 合并配置（全局配置 + 用户配置）
	 */
	SScriptConfig MergeConfig(const SScriptConfig& UserConfig) const;

	// === 初始化和清理 ===

	/**
	 * @brief 初始化脚本管理器
	 */
	bool Initialize();

	/**
	 * @brief 关闭脚本管理器
	 */
	void Shutdown();

	/**
	 * @brief 检查是否已初始化
	 */
	bool IsInitialized() const
	{
		return bInitialized;
	}

	// === 事件 ===

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEngineRegistered, EScriptLanguage /*Language*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEngineUnregistered, EScriptLanguage /*Language*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnContextCreated, TSharedPtr<CScriptContext> /*Context*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnContextDestroyed, TSharedPtr<CScriptContext> /*Context*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnScriptError, EScriptLanguage /*Language*/, const TString& /*ErrorMessage*/);

	FOnEngineRegistered OnEngineRegistered;
	FOnEngineUnregistered OnEngineUnregistered;
	FOnContextCreated OnContextCreated;
	FOnContextDestroyed OnContextDestroyed;
	FOnScriptError OnScriptError;

private:
	// === 内部实现 ===

	void RegisterBuiltinEngines();
	void ApplyGlobalBindings(TSharedPtr<CScriptContext> Context);
	TString GenerateContextId() const;

private:
	mutable CThreadSafeMutex Mutex;

	// 引擎注册表
	THashMap<EScriptLanguage, TArray<SScriptEngineRegistry, CMemoryManager>, CMemoryManager> EngineRegistry;
	THashMap<EScriptLanguage, TString, CMemoryManager> DefaultEngines;

	// 活跃上下文
	THashMap<TString, TSharedPtr<CScriptContext>, CMemoryManager> ActiveContexts;

	// 全局绑定
	THashMap<TString, TSharedPtr<CScriptFunction>, CMemoryManager> GlobalFunctions;
	THashMap<TString, CScriptValue, CMemoryManager> GlobalObjects;
	THashMap<TString, CScriptValue, CMemoryManager> GlobalConstants;

	// 配置和路径
	SScriptConfig GlobalConfig;
	TArray<TString, CMemoryManager> ModulePaths;

	// 统计信息
	mutable SScriptStatistics Statistics;

	// 状态
	bool bInitialized = false;
	uint32_t NextContextId = 1;
};

// === 便利函数 ===

/**
 * @brief 获取脚本管理器实例
 */
inline CScriptManager& GetScriptManager()
{
	return CScriptManager::GetInstance();
}

/**
 * @brief 执行脚本文件的便利函数
 */
inline SScriptExecutionResult ExecuteScriptFile(EScriptLanguage Language, const TString& FilePath)
{
	return GetScriptManager().ExecuteFile(Language, FilePath);
}

/**
 * @brief 执行脚本字符串的便利函数
 */
inline SScriptExecutionResult ExecuteScriptString(EScriptLanguage Language, const TString& Code)
{
	return GetScriptManager().ExecuteString(Language, Code);
}

// === 类型别名 ===
using FScriptManager = CScriptManager;

} // namespace NLib