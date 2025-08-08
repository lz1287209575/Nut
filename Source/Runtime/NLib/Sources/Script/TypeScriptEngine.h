#pragma once

#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Memory/Memory.h"
#include "ScriptEngine.h"

#include <memory>

// V8 JavaScript引擎前向声明
namespace v8
{
class Isolate;
class Context;
class Value;
class Object;
template <typename T>
class Local;
template <typename T>
class Global;
template <typename T>
class Persistent;
} // namespace v8

namespace NLib
{
/**
 * @brief TypeScript/JavaScript脚本值包装器
 * 基于V8引擎实现
 */
class CTypeScriptValue : public CScriptValue
{
	GENERATED_BODY()

public:
	CTypeScriptValue();
	CTypeScriptValue(v8::Isolate* InIsolate, v8::Local<v8::Value> InValue);
	CTypeScriptValue(const CTypeScriptValue& Other);
	CTypeScriptValue& operator=(const CTypeScriptValue& Other);
	~CTypeScriptValue() override;

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
	CString ToString() const override;

	int32_t GetArrayLength() const override;
	CScriptValue GetArrayElement(int32_t Index) const override;
	void SetArrayElement(int32_t Index, const CScriptValue& Value) override;

	TArray<CString, CMemoryManager> GetObjectKeys() const override;
	CScriptValue GetObjectProperty(const CString& Key) const override;
	void SetObjectProperty(const CString& Key, const CScriptValue& Value) override;
	bool HasObjectProperty(const CString& Key) const override;

	SScriptExecutionResult CallFunction(const TArray<CScriptValue, CMemoryManager>& Args) override;

	CConfigValue ToConfigValue() const override;
	void FromConfigValue(const CConfigValue& Config) override;

	// === TypeScript/V8特有方法 ===

	/**
	 * @brief 获取V8 Isolate
	 */
	v8::Isolate* GetIsolate() const
	{
		return Isolate;
	}

	/**
	 * @brief 获取V8值的本地句柄
	 */
	v8::Local<v8::Value> GetV8Value() const;

	/**
	 * @brief 检查值是否有效
	 */
	bool IsValid() const;

private:
	void CreatePersistent(v8::Isolate* InIsolate, v8::Local<v8::Value> InValue);
	void ReleasePersistent();
	void CopyFrom(const CTypeScriptValue& Other);

private:
	v8::Isolate* Isolate = nullptr;
	v8::Persistent<v8::Value>* PersistentValue = nullptr;
	EScriptValueType CachedType = EScriptValueType::Null;
};

/**
 * @brief TypeScript脚本模块
 */
class CTypeScriptModule : public CScriptModule
{
	GENERATED_BODY()

public:
	explicit CTypeScriptModule(v8::Isolate* InIsolate, const CString& InName);
	~CTypeScriptModule() override;

	// === CScriptModule接口实现 ===

	CString GetName() const override
	{
		return ModuleName;
	}
	CString GetVersion() const override
	{
		return TEXT("1.0");
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::TypeScript;
	}

	SScriptExecutionResult Load(const CString& ModulePath) override;
	SScriptExecutionResult Unload() override;
	bool IsLoaded() const override
	{
		return bLoaded;
	}

	CScriptValue GetGlobal(const CString& Name) const override;
	void SetGlobal(const CString& Name, const CScriptValue& Value) override;

	SScriptExecutionResult ExecuteString(const CString& Code) override;
	SScriptExecutionResult ExecuteFile(const CString& FilePath) override;

	void RegisterFunction(const CString& Name, TSharedPtr<CScriptFunction> Function) override;
	void RegisterObject(const CString& Name, const CScriptValue& Object) override;

	// === TypeScript特有方法 ===

	/**
	 * @brief 获取V8 Isolate
	 */
	v8::Isolate* GetIsolate() const
	{
		return Isolate;
	}

	/**
	 * @brief 编译TypeScript代码到JavaScript
	 * @param TypeScriptCode TypeScript源代码
	 * @return 编译后的JavaScript代码
	 */
	CString CompileTypeScript(const CString& TypeScriptCode);

	/**
	 * @brief 设置模块导出
	 */
	void SetModuleExports(const CString& Name, const CScriptValue& Value);

private:
	SScriptExecutionResult HandleV8Error(const CString& Operation);
	void SetupModuleEnvironment();

private:
	v8::Isolate* Isolate = nullptr;
	CString ModuleName;
	bool bLoaded = false;
	v8::Persistent<v8::Context>* ModuleContext = nullptr;
};

/**
 * @brief TypeScript脚本上下文
 */
class CTypeScriptContext : public CScriptContext
{
	GENERATED_BODY()

public:
	CTypeScriptContext();
	~CTypeScriptContext() override;

	// === CScriptContext接口实现 ===

	bool Initialize(const SScriptConfig& Config) override;
	void Shutdown() override;
	bool IsInitialized() const override
	{
		return Isolate != nullptr;
	}

	SScriptConfig GetConfig() const override
	{
		return Config;
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::TypeScript;
	}

	TSharedPtr<CScriptModule> CreateModule(const CString& Name) override;
	TSharedPtr<CScriptModule> GetModule(const CString& Name) const override;
	void DestroyModule(const CString& Name) override;

	SScriptExecutionResult ExecuteString(const CString& Code, const CString& ModuleName = TEXT("__main__")) override;
	SScriptExecutionResult ExecuteFile(const CString& FilePath, const CString& ModuleName = TEXT("")) override;

	void CollectGarbage() override;
	uint64_t GetMemoryUsage() const override;
	void ResetTimeout() override;

	void RegisterGlobalFunction(const CString& Name, TSharedPtr<CScriptFunction> Function) override;
	void RegisterGlobalObject(const CString& Name, const CScriptValue& Object) override;
	void RegisterGlobalConstant(const CString& Name, const CScriptValue& Value) override;

	// === TypeScript特有方法 ===

	/**
	 * @brief 获取V8 Isolate
	 */
	v8::Isolate* GetIsolate() const
	{
		return Isolate;
	}

	/**
	 * @brief 获取全局上下文
	 */
	v8::Local<v8::Context> GetGlobalContext() const;

	/**
	 * @brief 编译并执行TypeScript代码
	 * @param TypeScriptCode TypeScript源代码
	 * @param ModuleName 模块名称
	 * @return 执行结果
	 */
	SScriptExecutionResult ExecuteTypeScript(const CString& TypeScriptCode, const CString& ModuleName = TEXT(""));

	/**
	 * @brief 设置TypeScript编译选项
	 */
	void SetTypeScriptCompilerOptions(const THashMap<CString, CString, CMemoryManager>& Options);

private:
	bool InitializeV8();
	void ShutdownV8();
	SScriptExecutionResult HandleV8Error(const CString& Operation);
	void RegisterNLibAPI();
	CString CompileTypeScriptToJavaScript(const CString& TypeScriptCode);

	// V8回调函数
	static void MessageCallback(v8::Local<v8::Message> message, v8::Local<v8::Value> error);
	static void FatalErrorCallback(const char* location, const char* message);

private:
	v8::Isolate* Isolate = nullptr;
	v8::Persistent<v8::Context>* GlobalContext = nullptr;
	SScriptConfig Config;
	THashMap<CString, TSharedPtr<CTypeScriptModule>, CMemoryManager> Modules;
	THashMap<CString, CString, CMemoryManager> CompilerOptions;

	// 状态跟踪
	uint64_t StartTime = 0;
	bool bTimeoutEnabled = false;
};

/**
 * @brief TypeScript脚本引擎
 */
class CTypeScriptEngine : public CScriptEngine
{
	GENERATED_BODY()

public:
	CTypeScriptEngine();
	~CTypeScriptEngine() override;

	// === CScriptEngine接口实现 ===

	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::TypeScript;
	}
	CString GetVersion() const override;
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
	CScriptValue CreateString(const CString& Value) override;
	CScriptValue CreateArray() override;
	CScriptValue CreateObject() override;

	SScriptExecutionResult CheckSyntax(const CString& Code) override;
	SScriptExecutionResult CompileFile(const CString& FilePath, const CString& OutputPath = CString()) override;

	// === TypeScript特有方法 ===

	/**
	 * @brief 获取V8版本信息
	 */
	static CString GetV8VersionString();

	/**
	 * @brief 检查V8是否可用
	 */
	static bool IsV8Available();

	/**
	 * @brief 初始化全局V8平台
	 */
	static bool InitializeV8Platform();

	/**
	 * @brief 清理全局V8平台
	 */
	static void ShutdownV8Platform();

	/**
	 * @brief 编译TypeScript文件到JavaScript
	 */
	SScriptExecutionResult CompileTypeScriptFile(const CString& InputPath, const CString& OutputPath);

private:
	void RegisterStandardLibraries();
	static void InitializeV8Flags();

private:
	bool bInitialized = false;
	TArray<TSharedPtr<CTypeScriptContext>, CMemoryManager> ActiveContexts;

	static bool bV8PlatformInitialized;
	static std::unique_ptr<v8::Platform> V8Platform;
};

/**
 * @brief TypeScript类型转换器
 */
class CTypeScriptTypeConverter
{
public:
	/**
	 * @brief 将C++值转换为V8值
	 */
	template <typename T>
	static v8::Local<v8::Value> ToV8Value(v8::Isolate* isolate, const T& Value);

	/**
	 * @brief 从V8值获取C++值
	 */
	template <typename T>
	static T FromV8Value(v8::Isolate* isolate, v8::Local<v8::Value> Value);

	/**
	 * @brief 检查V8值是否为指定类型
	 */
	template <typename T>
	static bool IsV8Type(v8::Local<v8::Value> Value);

	/**
	 * @brief 转换CScriptValue为CTypeScriptValue
	 */
	static CTypeScriptValue ToTypeScriptValue(const CScriptValue& ScriptValue, v8::Isolate* isolate);

	/**
	 * @brief 转换CTypeScriptValue为CScriptValue
	 */
	static CScriptValue FromTypeScriptValue(const CTypeScriptValue& TypeScriptValue);

	/**
	 * @brief 转换ConfigValue为V8值
	 */
	static v8::Local<v8::Value> ConfigValueToV8(v8::Isolate* isolate, const CConfigValue& Config);

	/**
	 * @brief 转换V8值为ConfigValue
	 */
	static CConfigValue V8ToConfigValue(v8::Isolate* isolate, v8::Local<v8::Value> Value);
};

// === 类型别名 ===
using FTypeScriptEngine = CTypeScriptEngine;
using FTypeScriptContext = CTypeScriptContext;
using FTypeScriptModule = CTypeScriptModule;
using FTypeScriptValue = CTypeScriptValue;

} // namespace NLib

// === TypeScript绑定辅助宏 ===

/**
 * @brief 注册C++函数到TypeScript
 */
#define TS_BIND_FUNCTION(Isolate, Context, Name, Function) /* TODO: 实现TypeScript函数绑定 */

/**
 * @brief 注册C++对象到TypeScript
 */
#define TS_BIND_OBJECT(Isolate, Context, Name, Object) /* TODO: 实现TypeScript对象绑定 */

/**
 * @brief 检查TypeScript函数参数数量
 */
#define TS_CHECK_ARGS(Args, Expected)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if (Args.Length() != Expected)                                                                                 \
		{                                                                                                              \
			/* TODO: 抛出TypeScript错误 */                                                                             \
		}                                                                                                              \
	} while (0)

/*
使用示例：

```cpp
// 初始化TypeScript引擎
auto TSEngine = MakeShared<CTypeScriptEngine>();
TSEngine->Initialize();

// 创建上下文
SScriptConfig Config;
Config.bEnableSandbox = true;
auto Context = TSEngine->CreateContext(Config);

// 执行TypeScript代码
CString TSCode = TEXT(R"(
    class Player {
        name: string;
        health: number = 100;

        constructor(name: string) {
            this.name = name;
        }

        takeDamage(amount: number): void {
            this.health -= amount;
            console.log(`${this.name} took ${amount} damage, health: ${this.health}`);
        }
    }

    const player = new Player("TestPlayer");
    player.takeDamage(25);
)");

auto Result = Context->ExecuteTypeScript(TSCode);
if (Result.Result == EScriptResult::Success) {
    console.log("TypeScript executed successfully");
}
```

注意：
1. 这个实现需要链接V8引擎库
2. V8是一个大型依赖，需要适当的构建配置
3. TypeScript编译需要TypeScript编译器支持（可能通过Node.js或嵌入式编译器）
4. 实际的函数调用绑定需要更多的V8 API调用实现
*/
