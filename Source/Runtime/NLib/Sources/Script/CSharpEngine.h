#pragma once

#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Memory/Memory.h"
#include "ScriptEngine.h"

#include <memory>

// .NET Core运行时前向声明
#ifdef _WIN32
#include <windows.h>
#endif

// hostfxr和coreclr前向声明
struct hostfxr_handle;
typedef hostfxr_handle* hostfxr_handle_t;

namespace NLib
{
/**
 * @brief C#/.NET脚本值包装器
 * 基于.NET Core运行时实现
 */
class CCSharpValue : public CScriptValue
{
	GENERATED_BODY()

public:
	CCSharpValue();
	CCSharpValue(void* InDotNetObject, const char* InTypeName);
	CCSharpValue(const CCSharpValue& Other);
	CCSharpValue& operator=(const CCSharpValue& Other);
	~CCSharpValue() override;

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

	int32_t GetArrayLength() const override;:
	CScriptValue GetArrayElement(int32_t Index) const override;
	void SetArrayElement(int32_t Index, const CScriptValue& Value) override;

	TArray<CString, CMemoryManager> GetObjectKeys() const override;
	CScriptValue GetObjectProperty(const CString& Key) const override;
	void SetObjectProperty(const CString& Key, const CScriptValue& Value) override;
	bool HasObjectProperty(const CString& Key) const override;

	SScriptExecutionResult CallFunction(const TArray<CScriptValue, CMemoryManager>& Args) override;

	CConfigValue ToConfigValue() const override;
	void FromConfigValue(const CConfigValue& Config) override;

	// === C#/.NET特有方法 ===

	/**
	 * @brief 获取
	 */
	void* GetDotNetObject() const
	{
		return DotNetObject;
	}

	/**
	 * @brief 获取.NET类型名称
	 */
	const char* GetDotNetTypeName() const
	{
		return DotNetTypeName.GetData();
	}

	/**
	 * @brief 检查值是否有效
	 */
	bool IsValid() const;

	/**
	 * @brief 调用.NET方法
	 */
	CCSharpValue CallMethod(const CString& MethodName, const TArray<CCSharpValue, CMemoryManager>& Args);

	/**
	 * @brief 获取.NET属性值
	 */
	CCSharpValue GetProperty(const CString& PropertyName);

	/**
	 * @brief 设置.NET属性值
	 */
	void SetProperty(const CString& PropertyName, const CCSharpValue& Value);

private:
	void CreateReference(void* InDotNetObject, const char* InTypeName);
	void ReleaseReference();
	void CopyFrom(const CCSharpValue& Other);

private:
	void* DotNetObject = nullptr;
	CString DotNetTypeName;
	EScriptValueType CachedType = EScriptValueType::Null;
	uint32_t ReferenceCount = 0;
};

/**
 * @brief C#脚本模块
 */
class CCSharpModule : public CScriptModule
{
	GENERATED_BODY()

public:
	explicit CCSharpModule(void* InAssemblyContext, const CString& InName);
	~CCSharpModule() override;

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
		return EScriptLanguage::CSharp;
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

	// === C#特有方法 ===

	/**
	 * @brief 获取程序集上下文
	 */
	void* GetAssemblyContext() const
	{
		return AssemblyContext;
	}

	/**
	 * @brief 编译C#代码到程序集
	 * @param SourceCode C#源代码
	 * @param AssemblyName 程序集名称
	 * @return 是否编译成功
	 */
	bool CompileCSharpCode(const CString& SourceCode, const CString& AssemblyName);

	/**
	 * @brief 创建.NET类型实例
	 */
	CCSharpValue CreateInstance(const CString& TypeName, const TArray<CCSharpValue, CMemoryManager>& Args = {});

	/**
	 * @brief 调用静态方法
	 */
	CCSharpValue CallStaticMethod(const CString& TypeName,
	                              const CString& MethodName,
	                              const TArray<CCSharpValue, CMemoryManager>& Args = {});

private:
	SScriptExecutionResult HandleDotNetError(const CString& Operation, const CString& ErrorMessage = CString());
	void SetupModuleEnvironment();

private:
	void* AssemblyContext = nullptr;
	CString ModuleName;
	bool bLoaded = false;
	void* LoadedAssembly = nullptr;
	THashMap<CString, CCSharpValue, CMemoryManager> GlobalObjects;
};

/**
 * @brief C#脚本上下文
 */
class CCSharpContext : public CScriptContext
{
	GENERATED_BODY()

public:
	CCSharpContext();
	~CCSharpContext() override;

	// === CScriptContext接口实现 ===

	bool Initialize(const SScriptConfig& Config) override;
	void Shutdown() override;
	bool IsInitialized() const override
	{
		return HostHandle != nullptr;
	}

	SScriptConfig GetConfig() const override
	{
		return Config;
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::CSharp;
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

	// === C#特有方法 ===

	/**
	 * @brief 获取.NET运行时句柄
	 */
	hostfxr_handle_t GetHostHandle() const
	{
		return HostHandle;
	}

	/**
	 * @brief 获取程序集加载上下文
	 */
	void* GetAssemblyLoadContext() const
	{
		return AssemblyLoadContext;
	}

	/**
	 * @brief 编译并执行C#代码
	 * @param CSharpCode C#源代码
	 * @param AssemblyName 临时程序集名称
	 * @return 执行结果
	 */
	SScriptExecutionResult ExecuteCSharp(const CString& CSharpCode, const CString& AssemblyName = TEXT(""));

	/**
	 * @brief 加载.NET程序集
	 * @param AssemblyPath 程序集文件路径
	 * @return 程序集句柄
	 */
	void* LoadAssembly(const CString& AssemblyPath);

	/**
	 * @brief 设置.NET运行时配置
	 */
	void SetRuntimeConfig(const THashMap<CString, CString, CMemoryManager>& Config);

private:
	bool InitializeDotNet();
	void ShutdownDotNet();
	SScriptExecutionResult HandleDotNetError(const CString& Operation, const CString& ErrorMessage = CString());
	void RegisterNLibAPI();

	// .NET运行时回调函数
	static void ErrorCallback(const char* message);

private:
	hostfxr_handle_t HostHandle = nullptr;
	void* AssemblyLoadContext = nullptr;
	SScriptConfig Config;
	THashMap<CString, TSharedPtr<CCSharpModule>, CMemoryManager> Modules;
	THashMap<CString, CString, CMemoryManager> RuntimeConfig;

	// 状态跟踪
	uint64_t StartTime = 0;
	bool bTimeoutEnabled = false;

	// .NET运行时函数指针
	struct DotNetFunctionPointers
	{
		void* (*LoadAssemblyAndGetFunctionPointer)(const char* assemblyPath,
		                                           const char* typeName,
		                                           const char* methodName,
		                                           const char* delegateTypeName);
		int32_t (*GetManagedFunction)(const char* assemblyName,
		                              const char* typeName,
		                              const char* methodName,
		                              void** methodPtr);
		void (*InvokeMethod)(void* methodPtr, void** args, void** result);
		void (*ReleaseObject)(void* object);
	} DotNetFunctions;
};

/**
 * @brief C#脚本引擎
 */
class CCSharpEngine : public CScriptEngine
{
	GENERATED_BODY()

public:
	CCSharpEngine();
	~CCSharpEngine() override;

	// === CScriptEngine接口实现 ===

	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::CSharp;
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

	// === C#特有方法 ===

	/**
	 * @brief 获取.NET运行时版本信息
	 */
	static CString GetDotNetVersionString();

	/**
	 * @brief 检查.NET运行时是否可用
	 */
	static bool IsDotNetAvailable();

	/**
	 * @brief 初始化全局.NET运行时
	 */
	static bool InitializeDotNetRuntime();

	/**
	 * @brief 清理全局.NET运行时
	 */
	static void ShutdownDotNetRuntime();

	/**
	 * @brief 编译C#文件到程序集
	 */
	SScriptExecutionResult CompileCSharpFile(const CString& InputPath,
	                                         const CString& OutputPath,
	                                         const TArray<CString, CMemoryManager>& References = {});

	/**
	 * @brief 设置全局编译选项
	 */
	void SetCompilerOptions(const THashMap<CString, CString, CMemoryManager>& Options);

private:
	void RegisterStandardLibraries();
	static bool LoadDotNetRuntimeLibraries();

private:
	bool bInitialized = false;
	TArray<TSharedPtr<CCSharpContext>, CMemoryManager> ActiveContexts;
	THashMap<CString, CString, CMemoryManager> CompilerOptions;

	static bool bDotNetRuntimeInitialized;
	static void* HostfxrLibrary;
	static void* CoreCLRLibrary;
};

/**
 * @brief C#类型转换器
 */
class CCSharpTypeConverter
{
public:
	/**
	 * @brief 将C++值转换为.NET值
	 */
	template <typename T>
	static void* ToDotNetValue(const T& Value);

	/**
	 * @brief 从.NET值获取C++值
	 */
	template <typename T>
	static T FromDotNetValue(void* DotNetValue);

	/**
	 * @brief 检查.NET值是否为指定类型
	 */
	template <typename T>
	static bool IsDotNetType(void* DotNetValue);

	/**
	 * @brief 转换CScriptValue为CCSharpValue
	 */
	static CCSharpValue ToCSharpValue(const CScriptValue& ScriptValue);

	/**
	 * @brief 转换CCSharpValue为CScriptValue
	 */
	static CScriptValue FromCSharpValue(const CCSharpValue& CSharpValue);

	/**
	 * @brief 转换ConfigValue为.NET值
	 */
	static void* ConfigValueToDotNet(const CConfigValue& Config);

	/**
	 * @brief 转换.NET值为ConfigValue
	 */
	static CConfigValue DotNetToConfigValue(void* DotNetValue);

	/**
	 * @brief 获取.NET类型的名称
	 */
	static CString GetDotNetTypeName(void* DotNetValue);

	/**
	 * @brief 创建.NET数组
	 */
	static void* CreateDotNetArray(const TArray<void*, CMemoryManager>& Elements, const char* ElementTypeName);

	/**
	 * @brief 从.NET数组获取元素
	 */
	static TArray<void*, CMemoryManager> GetDotNetArrayElements(void* DotNetArray);
};

// === 类型别名 ===
using FCSharpEngine = CCSharpEngine;
using FCSharpContext = CCSharpContext;
using FCSharpModule = CCSharpModule;
using FCSharpValue = CCSharpValue;

} // namespace NLib

// === C#绑定辅助宏 ===

/**
 * @brief 注册C++函数到C#
 */
#define CS_BIND_FUNCTION(Context, Name, Function) /* TODO: 实现C#函数绑定 */

/**
 * @brief 注册C++对象到C#
 */
#define CS_BIND_OBJECT(Context, Name, Object) /* TODO: 实现C#对象绑定 */

/**
 * @brief 检查C#方法参数数量
 */
#define CS_CHECK_ARGS(Args, Expected)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if (Args.Size() != Expected)                                                                                   \
		{                                                                                                              \
			/* TODO: 抛出C#异常 */                                                                                     \
		}                                                                                                              \
	} while (0)

/*
使用示例：

```cpp
// 初始化C#引擎
auto CSEngine = MakeShared<CCSharpEngine>();
CSEngine->Initialize();

// 创建上下文
SScriptConfig Config;
Config.bEnableSandbox = false; // C#通常不需要沙箱
auto Context = CSEngine->CreateContext(Config);

// 执行C#代码
CString CSCode = TEXT(R"(
    using System;

    public class Player
    {
        public string Name { get; set; }
        public int Health { get; set; } = 100;

        public Player(string name)
        {
            Name = name;
        }

        public void TakeDamage(int amount)
        {
            Health -= amount;
            Console.WriteLine($"{Name} took {amount} damage, health: {Health}");
        }

        public static void Main()
        {
            var player = new Player("TestPlayer");
            player.TakeDamage(25);
        }
    }
)");

auto Result = Context->ExecuteCSharp(CSCode, "TestAssembly");
if (Result.Result == EScriptResult::Success) {
    Console.WriteLine("C# code executed successfully");
}
```

注意：
1. 这个实现需要.NET Core运行时（.NET 6+推荐）
2. 需要链接hostfxr和coreclr库
3. C#编译需要Roslyn编译器支持
4. 跨平台支持需要适当的运行时检测
5. 内存管理需要正确处理.NET GC与C++内存的交互

构建要求：
- Windows: 需要.NET Core/5+运行时
- Linux: 需要dotnet运行时包
- macOS: 需要.NET for macOS

依赖库：
- hostfxr (用于启动.NET运行时)
- coreclr (核心CLR运行时)
- System.Runtime.Loader (程序集加载)
*/