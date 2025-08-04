#pragma once

#include "Containers/THashMap.h"
#include "Core/TString.h"
#include "Memory/Memory.h"
#include "ScriptEngine.h"

// Lua 5.4前向声明
extern "C"
{
	struct lua_State;
}

namespace NLib
{
/**
 * @brief Lua脚本值包装器
 */
class CLuaScriptValue : public CScriptValue
{
	GENERATED_BODY()

public:
	CLuaScriptValue();
	CLuaScriptValue(lua_State* L, int Index);
	CLuaScriptValue(const CLuaScriptValue& Other);
	CLuaScriptValue& operator=(const CLuaScriptValue& Other);
	~CLuaScriptValue() override;

	// === CScriptValue接口实现 ===

	EScriptValueType GetType() const override;
	bool IsNull() const override;
	bool IsBoolean() const override;
	bool IsNumber() const override;
	bool IsString() const override;
	bool IsArray() const override;
	bool IsObject() const override;
	bool IsFunction() const override;
	bool IsUserData() const override;

	bool ToBool() const override;
	int32_t ToInt32() const override;
	int64_t ToInt64() const override;
	float ToFloat() const override;
	double ToDouble() const override;
	TString ToString() const override;

	int32_t GetArrayLength() const override;
	CScriptValue GetArrayElement(int32_t Index) const override;
	void SetArrayElement(int32_t Index, const CScriptValue& Value) override;

	TArray<TString, CMemoryManager> GetObjectKeys() const override;
	CScriptValue GetObjectProperty(const TString& Key) const override;
	void SetObjectProperty(const TString& Key, const CScriptValue& Value) override;
	bool HasObjectProperty(const TString& Key) const override;

	SScriptExecutionResult CallFunction(const TArray<CScriptValue, CMemoryManager>& Args) override;

	CConfigValue ToConfigValue() const override;
	void FromConfigValue(const CConfigValue& Config) override;

	// === Lua特有方法 ===

	/**
	 * @brief 将值推入Lua栈
	 */
	void PushToLuaStack(lua_State* L) const;

	/**
	 * @brief 从Lua栈获取值
	 */
	void PopFromLuaStack(lua_State* L, int Index);

	/**
	 * @brief 获取Lua状态机
	 */
	lua_State* GetLuaState() const
	{
		return LuaState;
	}

	/**
	 * @brief 获取在Lua注册表中的引用
	 */
	int GetLuaRef() const
	{
		return LuaRef;
	}

	/**
	 * @brief 检查值是否有效
	 */
	bool IsValid() const
	{
		return LuaState != nullptr && LuaRef != -1;
	}

private:
	void CreateReference(lua_State* L, int Index);
	void ReleaseReference();
	void CopyFrom(const CLuaScriptValue& Other);

private:
	lua_State* LuaState = nullptr;
	int LuaRef = -1; // Lua注册表中的引用ID
	EScriptValueType CachedType = EScriptValueType::Null;
};

/**
 * @brief Lua脚本模块
 */
class CLuaScriptModule : public CScriptModule
{
	GENERATED_BODY()

public:
	explicit CLuaScriptModule(lua_State* InLuaState, const TString& InName);
	~CLuaScriptModule() override;

	// === CScriptModule接口实现 ===

	TString GetName() const override
	{
		return ModuleName;
	}
	TString GetVersion() const override
	{
		return TEXT("1.0");
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Lua;
	}

	SScriptExecutionResult Load(const TString& ModulePath) override;
	SScriptExecutionResult Unload() override;
	bool IsLoaded() const override
	{
		return bLoaded;
	}

	CScriptValue GetGlobal(const TString& Name) const override;
	void SetGlobal(const TString& Name, const CScriptValue& Value) override;

	SScriptExecutionResult ExecuteString(const TString& Code) override;
	SScriptExecutionResult ExecuteFile(const TString& FilePath) override;

	void RegisterFunction(const TString& Name, TSharedPtr<CScriptFunction> Function) override;
	void RegisterObject(const TString& Name, const CScriptValue& Object) override;

	// === Lua特有方法 ===

	/**
	 * @brief 获取Lua状态机
	 */
	lua_State* GetLuaState() const
	{
		return LuaState;
	}

	/**
	 * @brief 创建Lua表作为模块环境
	 */
	void CreateModuleEnvironment();

private:
	SScriptExecutionResult HandleLuaError(int LuaResult, const TString& Operation);
	void SetupModuleEnvironment();

private:
	lua_State* LuaState = nullptr;
	TString ModuleName;
	bool bLoaded = false;
	int ModuleEnvRef = -1; // 模块环境表的引用
};

/**
 * @brief Lua脚本上下文
 */
class CLuaScriptContext : public CScriptContext
{
	GENERATED_BODY()

public:
	CLuaScriptContext();
	~CLuaScriptContext() override;

	// === CScriptContext接口实现 ===

	bool Initialize(const SScriptConfig& Config) override;
	void Shutdown() override;
	bool IsInitialized() const override
	{
		return LuaState != nullptr;
	}

	SScriptConfig GetConfig() const override
	{
		return Config;
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Lua;
	}

	TSharedPtr<CScriptModule> CreateModule(const TString& Name) override;
	TSharedPtr<CScriptModule> GetModule(const TString& Name) const override;
	void DestroyModule(const TString& Name) override;

	SScriptExecutionResult ExecuteString(const TString& Code, const TString& ModuleName = TEXT("__main__")) override;
	SScriptExecutionResult ExecuteFile(const TString& FilePath, const TString& ModuleName = TEXT("")) override;

	void CollectGarbage() override;
	uint64_t GetMemoryUsage() const override;
	void ResetTimeout() override;

	void RegisterGlobalFunction(const TString& Name, TSharedPtr<CScriptFunction> Function) override;
	void RegisterGlobalObject(const TString& Name, const CScriptValue& Object) override;
	void RegisterGlobalConstant(const TString& Name, const CScriptValue& Value) override;

	// === Lua特有方法 ===

	/**
	 * @brief 获取Lua状态机
	 */
	lua_State* GetLuaState() const
	{
		return LuaState;
	}

	/**
	 * @brief 设置Lua hook用于超时检查
	 */
	void SetupTimeoutHook();

	/**
	 * @brief 检查内存限制
	 */
	bool CheckMemoryLimit();

	/**
	 * @brief 创建安全的Lua环境
	 */
	void SetupSandbox();

private:
	bool InitializeLua();
	void ShutdownLua();
	SScriptExecutionResult HandleLuaError(int LuaResult, const TString& Operation);
	void RegisterNLibAPI();

	// Lua回调函数
	static void* LuaAllocator(void* UserData, void* Ptr, size_t OldSize, size_t NewSize);
	static void LuaHook(lua_State* L, lua_Debug* ar);
	static int LuaPanic(lua_State* L);

private:
	lua_State* LuaState = nullptr;
	SScriptConfig Config;
	THashMap<TString, TSharedPtr<CLuaScriptModule>, CMemoryManager> Modules;

	// 状态跟踪
	uint64_t AllocatedMemory = 0;
	uint64_t StartTime = 0;
	bool bTimeoutEnabled = false;
	bool bMemoryLimitEnabled = false;
};

/**
 * @brief Lua脚本引擎
 */
class CLuaScriptEngine : public CScriptEngine
{
	GENERATED_BODY()

public:
	CLuaScriptEngine();
	~CLuaScriptEngine() override;

	// === CScriptEngine接口实现 ===

	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Lua;
	}
	TString GetVersion() const override;
	bool IsSupported() const override;

	TSharedPtr<CScriptContext> CreateContext(const SScriptConfig& Config) override;
	void DestroyContext(TSharedPtr<CScriptContext> Context) override;

	bool Initialize() override;
	void Shutdown() override;
	bool IsInitialized() const override
	{
		return bInitialized;
	}

	CScriptValue CreateValue() override;
	CScriptValue CreateNull() override;
	CScriptValue CreateBool(bool Value) override;
	CScriptValue CreateInt(int32_t Value) override;
	CScriptValue CreateFloat(float Value) override;
	CScriptValue CreateString(const TString& Value) override;
	CScriptValue CreateArray() override;
	CScriptValue CreateObject() override;

	SScriptExecutionResult CheckSyntax(const TString& Code) override;
	SScriptExecutionResult CompileFile(const TString& FilePath, const TString& OutputPath = TString()) override;

	// === Lua特有方法 ===

	/**
	 * @brief 获取Lua版本信息
	 */
	static TString GetLuaVersionString();

	/**
	 * @brief 检查Lua是否可用
	 */
	static bool IsLuaAvailable();

private:
	void RegisterStandardLibraries();

private:
	bool bInitialized = false;
	TArray<TSharedPtr<CLuaScriptContext>, CMemoryManager> ActiveContexts;
};

/**
 * @brief Lua类型转换器
 */
class CLuaTypeConverter
{
public:
	/**
	 * @brief 将C++值转换为Lua值并推入栈
	 */
	template <typename T>
	static void PushValue(lua_State* L, const T& Value);

	/**
	 * @brief 从Lua栈获取C++值
	 */
	template <typename T>
	static T GetValue(lua_State* L, int Index);

	/**
	 * @brief 检查栈上的值是否为指定类型
	 */
	template <typename T>
	static bool IsType(lua_State* L, int Index);

	/**
	 * @brief 转换CScriptValue为CLuaScriptValue
	 */
	static CLuaScriptValue ToLuaValue(const CScriptValue& ScriptValue, lua_State* L);

	/**
	 * @brief 转换CLuaScriptValue为CScriptValue
	 */
	static CScriptValue FromLuaValue(const CLuaScriptValue& LuaValue);
};

// === 类型别名 ===
using FLuaScriptEngine = CLuaScriptEngine;
using FLuaScriptContext = CLuaScriptContext;
using FLuaScriptModule = CLuaScriptModule;
using FLuaScriptValue = CLuaScriptValue;

} // namespace NLib

// === Lua绑定辅助宏 ===

/**
 * @brief 注册C++函数到Lua
 */
#define LUA_BIND_FUNCTION(L, Name, Function)                                                                           \
	lua_pushcfunction(L, Function);                                                                                    \
	lua_setglobal(L, Name)

/**
 * @brief 注册C++对象到Lua
 */
#define LUA_BIND_OBJECT(L, Name, Object) /* 需要根据对象类型实现具体绑定 */

/**
 * @brief 检查Lua函数参数数量
 */
#define LUA_CHECK_ARGS(L, Expected)                                                                                    \
	do                                                                                                                 \
	{                                                                                                                  \
		int argc = lua_gettop(L);                                                                                      \
		if (argc != Expected)                                                                                          \
		{                                                                                                              \
			return luaL_error(L, "Expected %d arguments, got %d", Expected, argc);                                     \
		}                                                                                                              \
	} while (0)