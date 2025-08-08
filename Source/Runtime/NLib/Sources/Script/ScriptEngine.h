#pragma once

#include "Config/ConfigValue.h"
#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Core/Result.h"
#include "Delegate/Delegate.h"
#include "Memory/Memory.h"

#include "ScriptEngine.generate.h"

namespace NLib
{
// 前向声明
class NScriptValue;
class NScriptFunction;
class NScriptModule;
class NScriptContext;

/**
 * @brief 脚本语言类型
 */
enum class EScriptLanguage : uint8_t
{
	None = 0,
	Lua,        // Lua 5.4
	LuaForge,   // 基于Lua5.4扩展的class语法
	Python,     // Python 3.x
	TypeScript, // TypeScript (通过Node.js/Deno)
	CSharp,     // C# (.NET 8.0 LTS)
	NBP         // Nut Binary Program (自定义二进制脚本)
};

/**
 * @brief 脚本执行结果
 */
enum class EScriptResult : uint8_t
{
	Success = 0,
	CompileError,    // 编译错误
	RuntimeError,    // 运行时错误
	TypeError,       // 类型错误
	MemoryError,     // 内存错误
	TimeoutError,    // 超时错误
	SecurityError,   // 安全错误
	NotSupported,    // 不支持的操作
	InvalidArgument, // 无效参数
	EngineNotFound,  // 引擎未找到
	ModuleNotFound,  // 模块未找到
	FunctionNotFound // 函数未找到
};

/**
 * @brief 脚本值类型
 */
enum class EScriptValueType : uint8_t
{
	Null = 0,
	Boolean,
	Integer,
	Float,
	String,
	Array,
	Object,
	Function,
	UserData,
	Thread
};

/**
 * @brief 脚本执行上下文标志
 */
enum class EScriptContextFlags : uint32_t
{
	None = 0,
	EnableDebug = 1 << 0,         // 启用调试
	EnableSandbox = 1 << 1,       // 启用沙箱
	EnableTimeout = 1 << 2,       // 启用超时
	EnableMemoryLimit = 1 << 3,   // 启用内存限制
	EnableFileAccess = 1 << 4,    // 允许文件访问
	EnableNetworkAccess = 1 << 5, // 允许网络访问
	EnableNativeBinding = 1 << 6, // 允许原生绑定
	EnableModuleImport = 1 << 7,  // 允许模块导入
	EnableReflection = 1 << 8     // 启用反射
};

NLIB_DEFINE_ENUM_FLAG_OPERATORS(EScriptContextFlags)

/**
 * @brief 脚本执行配置
 */
struct SScriptConfig
{
	EScriptLanguage Language = EScriptLanguage::None;
	EScriptContextFlags Flags = EScriptContextFlags::None;
	uint32_t TimeoutMs = 5000;                                       // 执行超时(毫秒)
	uint32_t MemoryLimitMB = 256;                                    // 内存限制(MB)
	uint32_t MaxStackDepth = 1000;                                   // 最大栈深度
	CString WorkingDirectory;                                        // 工作目录
	TArray<CString, CMemoryManager> ModulePaths;                     // 模块搜索路径
	THashMap<CString, CString, CMemoryManager> EnvironmentVariables; // 环境变量

	SScriptConfig() = default;
	SScriptConfig(EScriptLanguage InLanguage)
	    : Language(InLanguage)
	{}
};

/**
 * @brief 脚本执行结果
 */
struct SScriptExecutionResult
{
	EScriptResult Result = EScriptResult::Success;
	CString ErrorMessage;
	int32_t ErrorLine = -1;
	int32_t ErrorColumn = -1;
	CString StackTrace;
	NScriptValue ReturnValue;
	uint64_t ExecutionTimeMs = 0;
	uint64_t MemoryUsedBytes = 0;

	bool IsSuccess() const
	{
		return Result == EScriptResult::Success;
	}
	bool IsError() const
	{
		return Result != EScriptResult::Success;
	}

	SScriptExecutionResult() = default;
	SScriptExecutionResult(EScriptResult InResult, const CString& InErrorMessage = CString())
	    : Result(InResult),
	      ErrorMessage(InErrorMessage)
	{}
};

/**
 * @brief 脚本值包装器
 */
NCLASS()
class NScriptValue : public NObject
{
	GENERATED_BODY()

public:
	NScriptValue() = default;
	NScriptValue(const NScriptValue& Other) = default;
	NScriptValue& operator=(const NScriptValue& Other) = default;
	virtual ~NScriptValue() = default;

	// 类型检查
	virtual EScriptValueType GetType() const = 0;
	virtual bool IsNull() const = 0;
	virtual bool IsBoolean() const = 0;
	virtual bool IsNumber() const = 0;
	virtual bool IsString() const = 0;
	virtual bool IsArray() const = 0;
	virtual bool IsObject() const = 0;
	virtual bool IsFunction() const = 0;
	virtual bool IsUserData() const = 0;

	// 值转换
	virtual bool ToBool() const = 0;
	virtual int32_t ToInt32() const = 0;
	virtual int64_t ToInt64() const = 0;
	virtual float ToFloat() const = 0;
	virtual double ToDouble() const = 0;
	virtual CString ToString() const = 0;

	// 数组操作
	virtual int32_t GetArrayLength() const = 0;
	virtual NScriptValue GetArrayElement(int32_t Index) const = 0;
	virtual void SetArrayElement(int32_t Index, const NScriptValue& Value) = 0;

	// 对象操作
	virtual TArray<CString, CMemoryManager> GetObjectKeys() const = 0;
	virtual NScriptValue GetObjectProperty(const CString& Key) const = 0;
	virtual void SetObjectProperty(const CString& Key, const NScriptValue& Value) = 0;
	virtual bool HasObjectProperty(const CString& Key) const = 0;

	// 函数调用
	virtual SScriptExecutionResult CallFunction(const TArray<NScriptValue, CMemoryManager>& Args) = 0;

	// 序列化支持
	virtual CConfigValue ToConfigValue() const = 0;
	virtual void FromConfigValue(const CConfigValue& Config) = 0;

	// 实用函数
	template <typename T>
	T As() const;

	template <typename T>
	bool TryGet(T& OutValue) const;
};

/**
 * @brief 脚本函数绑定
 */
NCLASS()
class NScriptFunction : public NObject
{
	GENERATED_BODY()

public:
	using FNativeFunction = TDelegate<SScriptExecutionResult(const TArray<NScriptValue, CMemoryManager>&)>;

	NScriptFunction() = default;
	virtual ~NScriptFunction() = default;

	virtual SScriptExecutionResult Call(const TArray<NScriptValue, CMemoryManager>& Args) = 0;
	virtual CString GetSignature() const = 0;
	virtual CString GetDocumentation() const = 0;
};

/**
 * @brief 脚本模块
 */
NCLASS()
class NScriptModule : public NObject
{
	GENERATED_BODY()

public:
	NScriptModule() = default;
	virtual ~NScriptModule() = default;

	virtual CString GetName() const = 0;
	virtual CString GetVersion() const = 0;
	virtual EScriptLanguage GetLanguage() const = 0;

	virtual SScriptExecutionResult Load(const CString& ModulePath) = 0;
	virtual SScriptExecutionResult Unload() = 0;
	virtual bool IsLoaded() const = 0;

	virtual NScriptValue GetGlobal(const CString& Name) const = 0;
	virtual void SetGlobal(const CString& Name, const NScriptValue& Value) = 0;

	virtual SScriptExecutionResult ExecuteString(const CString& Code) = 0;
	virtual SScriptExecutionResult ExecuteFile(const CString& FilePath) = 0;

	virtual void RegisterFunction(const CString& Name, TSharedPtr<NScriptFunction> Function) = 0;
	virtual void RegisterObject(const CString& Name, const NScriptValue& Object) = 0;
};

/**
 * @brief 脚本上下文
 */
NCLASS()
class NScriptContext : public NObject
{
	GENERATED_BODY()

public:
	NScriptContext() = default;
	virtual ~NScriptContext() = default;

	virtual bool Initialize(const SScriptConfig& Config) = 0;
	virtual void Shutdown() = 0;
	virtual bool IsInitialized() const = 0;

	virtual SScriptConfig GetConfig() const = 0;
	virtual EScriptLanguage GetLanguage() const = 0;

	virtual TSharedPtr<NScriptModule> CreateModule(const CString& Name) = 0;
	virtual TSharedPtr<NScriptModule> GetModule(const CString& Name) const = 0;
	virtual void DestroyModule(const CString& Name) = 0;

	virtual SScriptExecutionResult ExecuteString(const CString& Code, const CString& ModuleName = TEXT("__main__")) = 0;
	virtual SScriptExecutionResult ExecuteFile(const CString& FilePath, const CString& ModuleName = TEXT("")) = 0;

	virtual void CollectGarbage() = 0;
	virtual uint64_t GetMemoryUsage() const = 0;
	virtual void ResetTimeout() = 0;

	// 全局绑定
	virtual void RegisterGlobalFunction(const CString& Name, TSharedPtr<NScriptFunction> Function) = 0;
	virtual void RegisterGlobalObject(const CString& Name, const NScriptValue& Object) = 0;
	virtual void RegisterGlobalConstant(const CString& Name, const NScriptValue& Value) = 0;

	// 事件处理
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnScriptError,
	                                     const CString& /*ErrorMessage*/,
	                                     const CString& /*StackTrace*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnScriptTimeout, uint32_t /*TimeoutMs*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMemoryLimitExceeded, uint64_t /*MemoryUsed*/);

	FOnScriptError OnScriptError;
	FOnScriptTimeout OnScriptTimeout;
	FOnMemoryLimitExceeded OnMemoryLimitExceeded;
};

/**
 * @brief 抽象脚本引擎基类
 */
NCLASS()
class NScriptEngine : public NObject
{
	GENERATED_BODY()

public:
	NScriptEngine() = default;
	virtual ~NScriptEngine() = default;

	virtual EScriptLanguage GetLanguage() const = 0;
	virtual CString GetVersion() const = 0;
	virtual bool IsSupported() const = 0;

	virtual TSharedPtr<NScriptContext> CreateContext(const SScriptConfig& Config) = 0;
	virtual void DestroyContext(TSharedPtr<NScriptContext> Context) = 0;

	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual bool IsInitialized() const = 0;

	// 实用函数
	virtual NScriptValue CreateValue() = 0;
	virtual NScriptValue CreateNull() = 0;
	virtual NScriptValue CreateBool(bool Value) = 0;
	virtual NScriptValue CreateInt(int32_t Value) = 0;
	virtual NScriptValue CreateFloat(float Value) = 0;
	virtual NScriptValue CreateString(const CString& Value) = 0;
	virtual NScriptValue CreateArray() = 0;
	virtual NScriptValue CreateObject() = 0;

	// 编译检查
	virtual SScriptExecutionResult CheckSyntax(const CString& Code) = 0;
	virtual SScriptExecutionResult CompileFile(const CString& FilePath, const CString& OutputPath = CString()) = 0;
};

// === 类型别名 ===
using FScriptEngine = NScriptEngine;
using FScriptContext = NScriptContext;
using FScriptModule = NScriptModule;
using FScriptValue = NScriptValue;
using FScriptFunction = NScriptFunction;

} // namespace NLib
