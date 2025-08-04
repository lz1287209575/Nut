#include "LuaEngine.h"

#include "IO/FileSystem.h"
#include "Logging/LogCategory.h"
#include "Time/Time.h"

// Lua 5.4 头文件
extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace NLib
{
// === CLuaScriptValue 实现 ===

CLuaScriptValue::CLuaScriptValue()
    : LuaState(nullptr),
      LuaRef(-1),
      CachedType(EScriptValueType::Null)
{}

CLuaScriptValue::CLuaScriptValue(lua_State* L, int Index)
    : LuaState(L),
      LuaRef(-1)
{
	CreateReference(L, Index);
}

CLuaScriptValue::CLuaScriptValue(const CLuaScriptValue& Other)
    : LuaState(nullptr),
      LuaRef(-1)
{
	CopyFrom(Other);
}

CLuaScriptValue& CLuaScriptValue::operator=(const CLuaScriptValue& Other)
{
	if (this != &Other)
	{
		ReleaseReference();
		CopyFrom(Other);
	}
	return *this;
}

CLuaScriptValue::~CLuaScriptValue()
{
	ReleaseReference();
}

EScriptValueType CLuaScriptValue::GetType() const
{
	if (!IsValid())
		return EScriptValueType::Null;

	if (CachedType != EScriptValueType::Null)
		return CachedType;

	// 将值推入栈以检查类型
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	int Type = lua_type(LuaState, -1);
	lua_pop(LuaState, 1);

	switch (Type)
	{
	case LUA_TNIL:
		return EScriptValueType::Null;
	case LUA_TBOOLEAN:
		return EScriptValueType::Boolean;
	case LUA_TNUMBER:
		return lua_isinteger(LuaState, -1) ? EScriptValueType::Integer : EScriptValueType::Float;
	case LUA_TSTRING:
		return EScriptValueType::String;
	case LUA_TTABLE:
		// 简单区分数组和对象（检查是否有连续的数字索引）
		return IsArray() ? EScriptValueType::Array : EScriptValueType::Object;
	case LUA_TFUNCTION:
		return EScriptValueType::Function;
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
		return EScriptValueType::UserData;
	case LUA_TTHREAD:
		return EScriptValueType::Thread;
	default:
		return EScriptValueType::Null;
	}
}

bool CLuaScriptValue::IsNull() const
{
	return GetType() == EScriptValueType::Null;
}

bool CLuaScriptValue::IsBoolean() const
{
	return GetType() == EScriptValueType::Boolean;
}

bool CLuaScriptValue::IsNumber() const
{
	EScriptValueType Type = GetType();
	return Type == EScriptValueType::Integer || Type == EScriptValueType::Float;
}

bool CLuaScriptValue::IsString() const
{
	return GetType() == EScriptValueType::String;
}

bool CLuaScriptValue::IsArray() const
{
	if (!IsValid())
		return false;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	if (!lua_istable(LuaState, -1))
	{
		lua_pop(LuaState, 1);
		return false;
	}

	// 检查表是否为数组（连续的数字索引从1开始）
	lua_len(LuaState, -1);
	lua_Integer Length = lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	bool IsArrayLike = true;
	for (lua_Integer i = 1; i <= Length; ++i)
	{
		lua_geti(LuaState, -1, i);
		if (lua_isnil(LuaState, -1))
		{
			IsArrayLike = false;
			lua_pop(LuaState, 1);
			break;
		}
		lua_pop(LuaState, 1);
	}

	lua_pop(LuaState, 1);
	return IsArrayLike;
}

bool CLuaScriptValue::IsObject() const
{
	if (!IsValid())
		return false;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	bool IsTable = lua_istable(LuaState, -1);
	lua_pop(LuaState, 1);

	return IsTable && !IsArray();
}

bool CLuaScriptValue::IsFunction() const
{
	return GetType() == EScriptValueType::Function;
}

bool CLuaScriptValue::IsUserData() const
{
	return GetType() == EScriptValueType::UserData;
}

bool CLuaScriptValue::ToBool() const
{
	if (!IsValid())
		return false;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	bool Result = lua_toboolean(LuaState, -1) != 0;
	lua_pop(LuaState, 1);
	return Result;
}

int32_t CLuaScriptValue::ToInt32() const
{
	if (!IsValid())
		return 0;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	int32_t Result = static_cast<int32_t>(lua_tointeger(LuaState, -1));
	lua_pop(LuaState, 1);
	return Result;
}

int64_t CLuaScriptValue::ToInt64() const
{
	if (!IsValid())
		return 0;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	int64_t Result = static_cast<int64_t>(lua_tointeger(LuaState, -1));
	lua_pop(LuaState, 1);
	return Result;
}

float CLuaScriptValue::ToFloat() const
{
	if (!IsValid())
		return 0.0f;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	float Result = static_cast<float>(lua_tonumber(LuaState, -1));
	lua_pop(LuaState, 1);
	return Result;
}

double CLuaScriptValue::ToDouble() const
{
	if (!IsValid())
		return 0.0;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	double Result = lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);
	return Result;
}

TString CLuaScriptValue::ToString() const
{
	if (!IsValid())
		return TString();

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	size_t Length = 0;
	const char* Str = lua_tolstring(LuaState, -1, &Length);
	TString Result;
	if (Str)
	{
		Result = TString(Str, static_cast<int32_t>(Length));
	}

	lua_pop(LuaState, 1);
	return Result;
}

int32_t CLuaScriptValue::GetArrayLength() const
{
	if (!IsArray())
		return 0;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	lua_len(LuaState, -1);
	int32_t Length = static_cast<int32_t>(lua_tointeger(LuaState, -1));
	lua_pop(LuaState, 2);
	return Length;
}

CScriptValue CLuaScriptValue::GetArrayElement(int32_t Index) const
{
	if (!IsArray())
		return CScriptValue();

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	lua_geti(LuaState, -1, Index + 1); // Lua数组从1开始

	CLuaScriptValue Result(LuaState, -1);
	lua_pop(LuaState, 2);

	return Result;
}

void CLuaScriptValue::SetArrayElement(int32_t Index, const CScriptValue& Value)
{
	if (!IsArray())
		return;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	// 将新值推入栈
	const CLuaScriptValue* LuaValue = dynamic_cast<const CLuaScriptValue*>(&Value);
	if (LuaValue && LuaValue->IsValid())
	{
		LuaValue->PushToLuaStack(LuaState);
	}
	else
	{
		lua_pushnil(LuaState);
	}

	lua_seti(LuaState, -2, Index + 1); // Lua数组从1开始
	lua_pop(LuaState, 1);
}

TArray<TString, CMemoryManager> CLuaScriptValue::GetObjectKeys() const
{
	TArray<TString, CMemoryManager> Keys;

	if (!IsObject())
		return Keys;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	lua_pushnil(LuaState);
	while (lua_next(LuaState, -2) != 0)
	{
		// 键在索引-2，值在索引-1
		if (lua_type(LuaState, -2) == LUA_TSTRING)
		{
			size_t Length = 0;
			const char* Key = lua_tolstring(LuaState, -2, &Length);
			if (Key)
			{
				Keys.Add(TString(Key, static_cast<int32_t>(Length)));
			}
		}
		lua_pop(LuaState, 1); // 移除值，保留键用于下次迭代
	}

	lua_pop(LuaState, 1);
	return Keys;
}

CScriptValue CLuaScriptValue::GetObjectProperty(const TString& Key) const
{
	if (!IsObject())
		return CScriptValue();

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	lua_getfield(LuaState, -1, Key.GetData());

	CLuaScriptValue Result(LuaState, -1);
	lua_pop(LuaState, 2);

	return Result;
}

void CLuaScriptValue::SetObjectProperty(const TString& Key, const CScriptValue& Value)
{
	if (!IsObject())
		return;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	// 将新值推入栈
	const CLuaScriptValue* LuaValue = dynamic_cast<const CLuaScriptValue*>(&Value);
	if (LuaValue && LuaValue->IsValid())
	{
		LuaValue->PushToLuaStack(LuaState);
	}
	else
	{
		lua_pushnil(LuaState);
	}

	lua_setfield(LuaState, -2, Key.GetData());
	lua_pop(LuaState, 1);
}

bool CLuaScriptValue::HasObjectProperty(const TString& Key) const
{
	if (!IsObject())
		return false;

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);
	lua_getfield(LuaState, -1, Key.GetData());

	bool HasProperty = !lua_isnil(LuaState, -1);
	lua_pop(LuaState, 2);

	return HasProperty;
}

SScriptExecutionResult CLuaScriptValue::CallFunction(const TArray<CScriptValue, CMemoryManager>& Args)
{
	if (!IsFunction())
	{
		return SScriptExecutionResult(EScriptResult::TypeError, TEXT("Value is not a function"));
	}

	// 推入函数
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	// 推入参数
	for (const auto& Arg : Args)
	{
		const CLuaScriptValue* LuaArg = dynamic_cast<const CLuaScriptValue*>(&Arg);
		if (LuaArg && LuaArg->IsValid())
		{
			LuaArg->PushToLuaStack(LuaState);
		}
		else
		{
			lua_pushnil(LuaState);
		}
	}

	// 调用函数
	int Result = lua_pcall(LuaState, Args.Size(), 1, 0);

	if (Result != LUA_OK)
	{
		TString ErrorMessage;
		if (lua_isstring(LuaState, -1))
		{
			ErrorMessage = TString(lua_tostring(LuaState, -1));
		}
		lua_pop(LuaState, 1);

		EScriptResult ScriptResult = EScriptResult::RuntimeError;
		switch (Result)
		{
		case LUA_ERRRUN:
			ScriptResult = EScriptResult::RuntimeError;
			break;
		case LUA_ERRMEM:
			ScriptResult = EScriptResult::MemoryError;
			break;
		default:
			ScriptResult = EScriptResult::RuntimeError;
			break;
		}

		return SScriptExecutionResult(ScriptResult, ErrorMessage);
	}

	// 获取返回值
	CLuaScriptValue ReturnValue(LuaState, -1);
	lua_pop(LuaState, 1);

	SScriptExecutionResult ExecutionResult(EScriptResult::Success);
	ExecutionResult.ReturnValue = ReturnValue;
	return ExecutionResult;
}

CConfigValue CLuaScriptValue::ToConfigValue() const
{
	if (!IsValid())
		return CConfigValue();

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaRef);

	CConfigValue Result;
	int Type = lua_type(LuaState, -1);

	switch (Type)
	{
	case LUA_TNIL:
		Result = CConfigValue();
		break;
	case LUA_TBOOLEAN:
		Result = CConfigValue(lua_toboolean(LuaState, -1) != 0);
		break;
	case LUA_TNUMBER:
		if (lua_isinteger(LuaState, -1))
		{
			Result = CConfigValue(static_cast<int32_t>(lua_tointeger(LuaState, -1)));
		}
		else
		{
			Result = CConfigValue(static_cast<double>(lua_tonumber(LuaState, -1)));
		}
		break;
	case LUA_TSTRING: {
		size_t Length = 0;
		const char* Str = lua_tolstring(LuaState, -1, &Length);
		if (Str)
		{
			Result = CConfigValue(TString(Str, static_cast<int32_t>(Length)));
		}
	}
	break;
	case LUA_TTABLE:
		// TODO: 转换表为ConfigObject或ConfigArray
		break;
	default:
		// 其他类型转换为字符串
		Result = CConfigValue(ToString());
		break;
	}

	lua_pop(LuaState, 1);
	return Result;
}

void CLuaScriptValue::FromConfigValue(const CConfigValue& Config)
{
	// TODO: 从ConfigValue创建Lua值
	NLOG_SCRIPT(Warning, "CLuaScriptValue::FromConfigValue not implemented yet");
}

void CLuaScriptValue::PushToLuaStack(lua_State* L) const
{
	if (IsValid() && L == LuaState)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaRef);
	}
	else
	{
		lua_pushnil(L);
	}
}

void CLuaScriptValue::PopFromLuaStack(lua_State* L, int Index)
{
	ReleaseReference();
	CreateReference(L, Index);
}

void CLuaScriptValue::CreateReference(lua_State* L, int Index)
{
	if (!L)
		return;

	LuaState = L;

	// 将值复制到栈顶
	lua_pushvalue(L, Index);

	// 创建引用
	LuaRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// 缓存类型信息
	CachedType = EScriptValueType::Null; // 重置缓存
}

void CLuaScriptValue::ReleaseReference()
{
	if (LuaState && LuaRef != -1)
	{
		luaL_unref(LuaState, LUA_REGISTRYINDEX, LuaRef);
	}

	LuaState = nullptr;
	LuaRef = -1;
	CachedType = EScriptValueType::Null;
}

void CLuaScriptValue::CopyFrom(const CLuaScriptValue& Other)
{
	if (!Other.IsValid())
		return;

	LuaState = Other.LuaState;

	// 推入原值并创建新引用
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, Other.LuaRef);
	LuaRef = luaL_ref(LuaState, LUA_REGISTRYINDEX);

	CachedType = Other.CachedType;
}

// === CLuaScriptEngine 实现 ===

CLuaScriptEngine::CLuaScriptEngine()
    : bInitialized(false)
{}

CLuaScriptEngine::~CLuaScriptEngine()
{
	if (bInitialized)
	{
		Shutdown();
	}
}

TString CLuaScriptEngine::GetVersion() const
{
	return GetLuaVersionString();
}

bool CLuaScriptEngine::IsSupported() const
{
	return IsLuaAvailable();
}

TSharedPtr<CScriptContext> CLuaScriptEngine::CreateContext(const SScriptConfig& Config)
{
	if (!bInitialized)
	{
		NLOG_SCRIPT(Error, "Lua engine not initialized");
		return nullptr;
	}

	auto Context = MakeShared<CLuaScriptContext>();
	if (!Context->Initialize(Config))
	{
		NLOG_SCRIPT(Error, "Failed to initialize Lua script context");
		return nullptr;
	}

	ActiveContexts.Add(Context);
	return Context;
}

void CLuaScriptEngine::DestroyContext(TSharedPtr<CScriptContext> Context)
{
	if (!Context)
		return;

	// 从活跃列表中移除
	for (int32_t i = 0; i < ActiveContexts.Size(); ++i)
	{
		if (ActiveContexts[i] == Context)
		{
			ActiveContexts.RemoveAt(i);
			break;
		}
	}

	Context->Shutdown();
}

bool CLuaScriptEngine::Initialize()
{
	if (bInitialized)
		return true;

	NLOG_SCRIPT(Info, "Initializing Lua Script Engine...");

	if (!IsLuaAvailable())
	{
		NLOG_SCRIPT(Error, "Lua is not available");
		return false;
	}

	NLOG_SCRIPT(Info, "Lua Script Engine initialized successfully ({})", GetLuaVersionString().GetData());

	bInitialized = true;
	return true;
}

void CLuaScriptEngine::Shutdown()
{
	if (!bInitialized)
		return;

	NLOG_SCRIPT(Info, "Shutting down Lua Script Engine...");

	// 关闭所有活跃上下文
	for (auto& Context : ActiveContexts)
	{
		if (Context)
		{
			Context->Shutdown();
		}
	}
	ActiveContexts.Clear();

	bInitialized = false;
	NLOG_SCRIPT(Info, "Lua Script Engine shut down");
}

CScriptValue CLuaScriptEngine::CreateValue()
{
	// 返回空的Lua值
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateNull()
{
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateBool(bool Value)
{
	// 需要一个临时Lua状态来创建值
	// 实际实现中可能需要从上下文获取
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateBool needs context to create Lua values");
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateInt(int32_t Value)
{
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateInt needs context to create Lua values");
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateFloat(float Value)
{
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateFloat needs context to create Lua values");
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateString(const TString& Value)
{
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateString needs context to create Lua values");
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateArray()
{
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateArray needs context to create Lua values");
	return CLuaScriptValue();
}

CScriptValue CLuaScriptEngine::CreateObject()
{
	NLOG_SCRIPT(Warning, "CLuaScriptEngine::CreateObject needs context to create Lua values");
	return CLuaScriptValue();
}

SScriptExecutionResult CLuaScriptEngine::CheckSyntax(const TString& Code)
{
	// 创建临时Lua状态进行语法检查
	lua_State* L = luaL_newstate();
	if (!L)
	{
		return SScriptExecutionResult(EScriptResult::MemoryError, TEXT("Failed to create Lua state"));
	}

	int Result = luaL_loadstring(L, Code.GetData());

	SScriptExecutionResult ExecutionResult;
	if (Result != LUA_OK)
	{
		TString ErrorMessage;
		if (lua_isstring(L, -1))
		{
			ErrorMessage = TString(lua_tostring(L, -1));
		}

		ExecutionResult = SScriptExecutionResult(EScriptResult::CompileError, ErrorMessage);
	}
	else
	{
		ExecutionResult = SScriptExecutionResult(EScriptResult::Success);
	}

	lua_close(L);
	return ExecutionResult;
}

SScriptExecutionResult CLuaScriptEngine::CompileFile(const TString& FilePath, const TString& OutputPath)
{
	// Lua是解释型语言，通常不需要预编译
	// 这里可以实现语法检查

	if (!CFileSystem::FileExists(FilePath))
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument,
		                              TString(TEXT("Script file not found: ")) + FilePath);
	}

	// 读取文件内容进行语法检查
	auto FileContent = CFileSystem::ReadFileAsString(FilePath);
	if (!FileContent.IsSuccess())
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument, TEXT("Failed to read script file"));
	}

	return CheckSyntax(FileContent.GetValue());
}

TString CLuaScriptEngine::GetLuaVersionString()
{
	return TString(LUA_VERSION);
}

bool CLuaScriptEngine::IsLuaAvailable()
{
	// 尝试创建Lua状态来检查是否可用
	lua_State* L = luaL_newstate();
	if (L)
	{
		lua_close(L);
		return true;
	}
	return false;
}

// === CLuaScriptModule 实现 ===

CLuaScriptModule::CLuaScriptModule(lua_State* InLuaState, const TString& InName)
    : LuaState(InLuaState),
      ModuleName(InName),
      bLoaded(false),
      ModuleEnvRef(-1)
{
	CreateModuleEnvironment();
}

CLuaScriptModule::~CLuaScriptModule()
{
	if (bLoaded)
	{
		Unload();
	}

	if (LuaState && ModuleEnvRef != -1)
	{
		luaL_unref(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);
	}
}

SScriptExecutionResult CLuaScriptModule::Load(const TString& ModulePath)
{
	if (bLoaded)
	{
		return SScriptExecutionResult(EScriptResult::Success);
	}

	if (!CFileSystem::FileExists(ModulePath))
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument,
		                              TString(TEXT("Module file not found: ")) + ModulePath);
	}

	// 读取模块文件
	auto FileContent = CFileSystem::ReadFileAsString(ModulePath);
	if (!FileContent.IsSuccess())
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument, TEXT("Failed to read module file"));
	}

	// 编译模块代码
	int Result = luaL_loadstring(LuaState, FileContent.GetValue().GetData());
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("compiling module"));
	}

	// 设置模块环境并执行
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);
	lua_setupvalue(LuaState, -2, 1); // 设置_ENV

	Result = lua_pcall(LuaState, 0, 0, 0);
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("executing module"));
	}

	bLoaded = true;
	NLOG_SCRIPT(Info, "Module '{}' loaded successfully", ModuleName.GetData());

	return SScriptExecutionResult(EScriptResult::Success);
}

SScriptExecutionResult CLuaScriptModule::Unload()
{
	if (!bLoaded)
	{
		return SScriptExecutionResult(EScriptResult::Success);
	}

	// 清理模块环境
	if (LuaState && ModuleEnvRef != -1)
	{
		lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);

		// 清除所有全局变量（除了内置的）
		lua_pushnil(LuaState);
		while (lua_next(LuaState, -2) != 0)
		{
			lua_pop(LuaState, 1); // 移除值
			if (lua_type(LuaState, -1) == LUA_TSTRING)
			{
				const char* Key = lua_tostring(LuaState, -1);
				// 保留一些基础函数
				if (strcmp(Key, "_G") != 0 && strcmp(Key, "pairs") != 0 && strcmp(Key, "ipairs") != 0 &&
				    strcmp(Key, "next") != 0)
				{
					lua_pushvalue(LuaState, -1); // 复制键
					lua_pushnil(LuaState);
					lua_settable(LuaState, -4); // 设置为nil
				}
			}
		}

		lua_pop(LuaState, 1);
	}

	bLoaded = false;
	NLOG_SCRIPT(Info, "Module '{}' unloaded", ModuleName.GetData());

	return SScriptExecutionResult(EScriptResult::Success);
}

CScriptValue CLuaScriptModule::GetGlobal(const TString& Name) const
{
	if (!bLoaded || !LuaState || ModuleEnvRef == -1)
	{
		return CLuaScriptValue();
	}

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);
	lua_getfield(LuaState, -1, Name.GetData());

	CLuaScriptValue Result(LuaState, -1);
	lua_pop(LuaState, 2);

	return Result;
}

void CLuaScriptModule::SetGlobal(const TString& Name, const CScriptValue& Value)
{
	if (!bLoaded || !LuaState || ModuleEnvRef == -1)
	{
		return;
	}

	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);

	const CLuaScriptValue* LuaValue = dynamic_cast<const CLuaScriptValue*>(&Value);
	if (LuaValue && LuaValue->IsValid())
	{
		LuaValue->PushToLuaStack(LuaState);
	}
	else
	{
		lua_pushnil(LuaState);
	}

	lua_setfield(LuaState, -2, Name.GetData());
	lua_pop(LuaState, 1);
}

SScriptExecutionResult CLuaScriptModule::ExecuteString(const TString& Code)
{
	if (!bLoaded || !LuaState || ModuleEnvRef == -1)
	{
		return SScriptExecutionResult(EScriptResult::RuntimeError, TEXT("Module not loaded"));
	}

	// 编译代码
	int Result = luaL_loadstring(LuaState, Code.GetData());
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("compiling code"));
	}

	// 设置模块环境并执行
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ModuleEnvRef);
	lua_setupvalue(LuaState, -2, 1); // 设置_ENV

	Result = lua_pcall(LuaState, 0, LUA_MULTRET, 0);
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("executing code"));
	}

	// 获取返回值（如果有）
	CLuaScriptValue ReturnValue;
	int NumResults = lua_gettop(LuaState);
	if (NumResults > 0)
	{
		ReturnValue = CLuaScriptValue(LuaState, -1);
		lua_pop(LuaState, NumResults);
	}

	SScriptExecutionResult ExecutionResult(EScriptResult::Success);
	ExecutionResult.ReturnValue = ReturnValue;
	return ExecutionResult;
}

SScriptExecutionResult CLuaScriptModule::ExecuteFile(const TString& FilePath)
{
	if (!CFileSystem::FileExists(FilePath))
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument,
		                              TString(TEXT("Script file not found: ")) + FilePath);
	}

	auto FileContent = CFileSystem::ReadFileAsString(FilePath);
	if (!FileContent.IsSuccess())
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument, TEXT("Failed to read script file"));
	}

	return ExecuteString(FileContent.GetValue());
}

void CLuaScriptModule::RegisterFunction(const TString& Name, TSharedPtr<CScriptFunction> Function)
{
	if (!bLoaded || !LuaState || ModuleEnvRef == -1 || !Function)
	{
		return;
	}

	// TODO: 实现函数注册到模块环境
	NLOG_SCRIPT(Warning, "CLuaScriptModule::RegisterFunction not fully implemented");
}

void CLuaScriptModule::RegisterObject(const TString& Name, const CScriptValue& Object)
{
	SetGlobal(Name, Object);
}

void CLuaScriptModule::CreateModuleEnvironment()
{
	if (!LuaState)
		return;

	// 创建新的环境表
	lua_newtable(LuaState);

	// 设置环境表的元表，使其能访问全局变量
	lua_newtable(LuaState);
	lua_pushglobaltable(LuaState);
	lua_setfield(LuaState, -2, "__index");
	lua_setmetatable(LuaState, -2);

	// 将环境表保存到注册表
	ModuleEnvRef = luaL_ref(LuaState, LUA_REGISTRYINDEX);
}

SScriptExecutionResult CLuaScriptModule::HandleLuaError(int LuaResult, const TString& Operation)
{
	TString ErrorMessage;
	if (lua_isstring(LuaState, -1))
	{
		ErrorMessage = TString(lua_tostring(LuaState, -1));
	}
	else
	{
		ErrorMessage = TString(TEXT("Unknown error during ")) + Operation;
	}

	lua_pop(LuaState, 1);

	EScriptResult ScriptResult = EScriptResult::RuntimeError;
	switch (LuaResult)
	{
	case LUA_ERRRUN:
		ScriptResult = EScriptResult::RuntimeError;
		break;
	case LUA_ERRMEM:
		ScriptResult = EScriptResult::MemoryError;
		break;
	case LUA_ERRSYNTAX:
		ScriptResult = EScriptResult::CompileError;
		break;
	default:
		ScriptResult = EScriptResult::RuntimeError;
		break;
	}

	NLOG_SCRIPT(Error,
	            "Lua error in module '{}' during {}: {}",
	            ModuleName.GetData(),
	            Operation.GetData(),
	            ErrorMessage.GetData());

	return SScriptExecutionResult(ScriptResult, ErrorMessage);
}

// === CLuaScriptContext 实现 ===

CLuaScriptContext::CLuaScriptContext()
    : LuaState(nullptr),
      AllocatedMemory(0),
      StartTime(0),
      bTimeoutEnabled(false),
      bMemoryLimitEnabled(false)
{}

CLuaScriptContext::~CLuaScriptContext()
{
	if (LuaState)
	{
		Shutdown();
	}
}

bool CLuaScriptContext::Initialize(const SScriptConfig& InConfig)
{
	if (LuaState)
	{
		NLOG_SCRIPT(Warning, "Lua context already initialized");
		return true;
	}

	Config = InConfig;

	if (!InitializeLua())
	{
		NLOG_SCRIPT(Error, "Failed to initialize Lua state");
		return false;
	}

	// 设置安全沙箱
	if (Config.bEnableSandbox)
	{
		SetupSandbox();
	}

	// 设置超时检查
	if (Config.TimeoutMilliseconds > 0)
	{
		bTimeoutEnabled = true;
		SetupTimeoutHook();
	}

	// 设置内存限制
	if (Config.MemoryLimitBytes > 0)
	{
		bMemoryLimitEnabled = true;
	}

	// 注册NLib API
	RegisterNLibAPI();

	NLOG_SCRIPT(Info, "Lua script context initialized successfully");
	return true;
}

void CLuaScriptContext::Shutdown()
{
	if (!LuaState)
		return;

	NLOG_SCRIPT(Info, "Shutting down Lua script context...");

	// 销毁所有模块
	for (auto& ModulePair : Modules)
	{
		if (ModulePair.Value)
		{
			ModulePair.Value->Unload();
		}
	}
	Modules.Clear();

	ShutdownLua();

	NLOG_SCRIPT(Info, "Lua script context shut down");
}

TSharedPtr<CScriptModule> CLuaScriptContext::CreateModule(const TString& Name)
{
	if (!LuaState)
	{
		NLOG_SCRIPT(Error, "Lua context not initialized");
		return nullptr;
	}

	if (Modules.Contains(Name))
	{
		NLOG_SCRIPT(Warning, "Module '{}' already exists", Name.GetData());
		return Modules[Name];
	}

	auto Module = MakeShared<CLuaScriptModule>(LuaState, Name);
	Modules.Add(Name, Module);

	NLOG_SCRIPT(Info, "Created module '{}'", Name.GetData());
	return Module;
}

TSharedPtr<CScriptModule> CLuaScriptContext::GetModule(const TString& Name) const
{
	auto* Module = Modules.Find(Name);
	return Module ? *Module : nullptr;
}

void CLuaScriptContext::DestroyModule(const TString& Name)
{
	auto* Module = Modules.Find(Name);
	if (Module && *Module)
	{
		(*Module)->Unload();
		Modules.Remove(Name);
		NLOG_SCRIPT(Info, "Destroyed module '{}'", Name.GetData());
	}
}

SScriptExecutionResult CLuaScriptContext::ExecuteString(const TString& Code, const TString& ModuleName)
{
	if (!LuaState)
	{
		return SScriptExecutionResult(EScriptResult::RuntimeError, TEXT("Context not initialized"));
	}

	ResetTimeout();

	// 编译代码
	int Result = luaL_loadstring(LuaState, Code.GetData());
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("compiling code"));
	}

	// 执行代码
	Result = lua_pcall(LuaState, 0, LUA_MULTRET, 0);
	if (Result != LUA_OK)
	{
		return HandleLuaError(Result, TEXT("executing code"));
	}

	// 获取返回值
	CLuaScriptValue ReturnValue;
	int NumResults = lua_gettop(LuaState);
	if (NumResults > 0)
	{
		ReturnValue = CLuaScriptValue(LuaState, -1);
		lua_pop(LuaState, NumResults);
	}

	SScriptExecutionResult ExecutionResult(EScriptResult::Success);
	ExecutionResult.ReturnValue = ReturnValue;
	return ExecutionResult;
}

SScriptExecutionResult CLuaScriptContext::ExecuteFile(const TString& FilePath, const TString& ModuleName)
{
	if (!CFileSystem::FileExists(FilePath))
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument,
		                              TString(TEXT("Script file not found: ")) + FilePath);
	}

	auto FileContent = CFileSystem::ReadFileAsString(FilePath);
	if (!FileContent.IsSuccess())
	{
		return SScriptExecutionResult(EScriptResult::InvalidArgument, TEXT("Failed to read script file"));
	}

	return ExecuteString(FileContent.GetValue(), ModuleName);
}

void CLuaScriptContext::CollectGarbage()
{
	if (LuaState)
	{
		lua_gc(LuaState, LUA_GCCOLLECT, 0);
	}
}

uint64_t CLuaScriptContext::GetMemoryUsage() const
{
	if (LuaState)
	{
		return static_cast<uint64_t>(lua_gc(LuaState, LUA_GCCOUNT, 0)) * 1024 +
		       static_cast<uint64_t>(lua_gc(LuaState, LUA_GCCOUNTB, 0));
	}
	return 0;
}

void CLuaScriptContext::ResetTimeout()
{
	if (bTimeoutEnabled)
	{
		StartTime = CTime::GetCurrentTimeMilliseconds();
	}
}

void CLuaScriptContext::RegisterGlobalFunction(const TString& Name, TSharedPtr<CScriptFunction> Function)
{
	if (!LuaState || !Function)
		return;

	// TODO: 实现全局函数注册
	NLOG_SCRIPT(Warning, "CLuaScriptContext::RegisterGlobalFunction not fully implemented");
}

void CLuaScriptContext::RegisterGlobalObject(const TString& Name, const CScriptValue& Object)
{
	if (!LuaState)
		return;

	const CLuaScriptValue* LuaValue = dynamic_cast<const CLuaScriptValue*>(&Object);
	if (LuaValue && LuaValue->IsValid())
	{
		LuaValue->PushToLuaStack(LuaState);
	}
	else
	{
		lua_pushnil(LuaState);
	}

	lua_setglobal(LuaState, Name.GetData());
}

void CLuaScriptContext::RegisterGlobalConstant(const TString& Name, const CScriptValue& Value)
{
	RegisterGlobalObject(Name, Value);
}

void CLuaScriptContext::SetupTimeoutHook()
{
	if (LuaState && bTimeoutEnabled)
	{
		lua_sethook(LuaState, LuaHook, LUA_MASKCOUNT, 1000); // 每1000条指令检查一次
	}
}

bool CLuaScriptContext::CheckMemoryLimit()
{
	if (!bMemoryLimitEnabled)
		return true;

	uint64_t CurrentMemory = GetMemoryUsage();
	return CurrentMemory <= Config.MemoryLimitBytes;
}

void CLuaScriptContext::SetupSandbox()
{
	if (!LuaState)
		return;

	// 移除危险的函数
	const char* DangerousFunctions[] = {"dofile",
	                                    "loadfile",
	                                    "load",
	                                    "loadstring",
	                                    "require",
	                                    "module",
	                                    "getfenv",
	                                    "setfenv",
	                                    "rawget",
	                                    "rawset",
	                                    "rawlen",
	                                    "rawequal",
	                                    "collectgarbage",
	                                    "gcinfo"};

	for (const char* FuncName : DangerousFunctions)
	{
		lua_pushnil(LuaState);
		lua_setglobal(LuaState, FuncName);
	}

	// 限制一些库的访问
	lua_getglobal(LuaState, "os");
	if (lua_istable(LuaState, -1))
	{
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "execute");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "exit");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "remove");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "rename");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "tmpname");
	}
	lua_pop(LuaState, 1);

	lua_getglobal(LuaState, "io");
	if (lua_istable(LuaState, -1))
	{
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "open");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "popen");
		lua_pushnil(LuaState);
		lua_setfield(LuaState, -2, "tmpfile");
	}
	lua_pop(LuaState, 1);
}

bool CLuaScriptContext::InitializeLua()
{
	// 创建自定义分配器的Lua状态
	if (bMemoryLimitEnabled)
	{
		LuaState = lua_newstate(LuaAllocator, this);
	}
	else
	{
		LuaState = luaL_newstate();
	}

	if (!LuaState)
	{
		return false;
	}

	// 设置panic函数
	lua_atpanic(LuaState, LuaPanic);

	// 打开标准库
	luaL_openlibs(LuaState);

	return true;
}

void CLuaScriptContext::ShutdownLua()
{
	if (LuaState)
	{
		lua_close(LuaState);
		LuaState = nullptr;
	}

	AllocatedMemory = 0;
}

SScriptExecutionResult CLuaScriptContext::HandleLuaError(int LuaResult, const TString& Operation)
{
	TString ErrorMessage;
	if (lua_isstring(LuaState, -1))
	{
		ErrorMessage = TString(lua_tostring(LuaState, -1));
	}
	else
	{
		ErrorMessage = TString(TEXT("Unknown error during ")) + Operation;
	}

	lua_pop(LuaState, 1);

	EScriptResult ScriptResult = EScriptResult::RuntimeError;
	switch (LuaResult)
	{
	case LUA_ERRRUN:
		ScriptResult = EScriptResult::RuntimeError;
		break;
	case LUA_ERRMEM:
		ScriptResult = EScriptResult::MemoryError;
		break;
	case LUA_ERRSYNTAX:
		ScriptResult = EScriptResult::CompileError;
		break;
	default:
		ScriptResult = EScriptResult::RuntimeError;
		break;
	}

	NLOG_SCRIPT(Error, "Lua error during {}: {}", Operation.GetData(), ErrorMessage.GetData());

	return SScriptExecutionResult(ScriptResult, ErrorMessage);
}

void CLuaScriptContext::RegisterNLibAPI()
{
	if (!LuaState)
		return;

	// 创建NLib表
	lua_newtable(LuaState);

	// TODO: 注册NLib API函数到表中

	lua_setglobal(LuaState, "NLib");
}

void* CLuaScriptContext::LuaAllocator(void* UserData, void* Ptr, size_t OldSize, size_t NewSize)
{
	CLuaScriptContext* Context = static_cast<CLuaScriptContext*>(UserData);
	auto& MemMgr = CMemoryManager::GetInstance();

	if (NewSize == 0)
	{
		// 释放内存
		if (Ptr)
		{
			Context->AllocatedMemory -= OldSize;
			MemMgr.Deallocate(Ptr);
		}
		return nullptr;
	}

	// 检查内存限制
	if (Context->bMemoryLimitEnabled)
	{
		uint64_t NewTotal = Context->AllocatedMemory - OldSize + NewSize;
		if (NewTotal > Context->Config.MemoryLimitBytes)
		{
			return nullptr; // 内存不足
		}
	}

	// 分配或重新分配内存
	void* NewPtr;
	if (Ptr)
	{
		NewPtr = MemMgr.Reallocate(Ptr, NewSize);
	}
	else
	{
		NewPtr = MemMgr.Allocate(NewSize);
	}

	if (NewPtr)
	{
		Context->AllocatedMemory = Context->AllocatedMemory - OldSize + NewSize;
	}

	return NewPtr;
}

void CLuaScriptContext::LuaHook(lua_State* L, lua_Debug* ar)
{
	// 获取上下文指针（需要从Lua状态中恢复）
	// 这里简化实现，实际需要更复杂的上下文管理

	// 检查超时
	uint64_t CurrentTime = CTime::GetCurrentTimeMilliseconds();
	// TODO: 从Lua状态中获取上下文以检查超时

	// 如果超时，抛出错误
	// luaL_error(L, "Script execution timeout");
}

int CLuaScriptContext::LuaPanic(lua_State* L)
{
	const char* Message = lua_tostring(L, -1);
	NLOG_SCRIPT(Error, "Lua panic: {}", Message ? Message : "Unknown error");
	return 0;
}

} // namespace NLib