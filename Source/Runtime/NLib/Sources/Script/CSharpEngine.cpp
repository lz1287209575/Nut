#include "CSharpEngine.h"

#include "IO/Path.h"
#include "Logging/LogCategory.h"
#include "Memory/MemoryManager.h"

#ifdef _WIN32
#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>
#include <windows.h>
#else
#include <coreclr_delegates.h>
#include <dlfcn.h>
#include <hostfxr.h>
#include <nethost.h>
#endif

namespace NLib
{
// === 静态成员初始化 ===

bool CCSharpEngine::bDotNetRuntimeInitialized = false;
void* CCSharpEngine::HostfxrLibrary = nullptr;
void* CCSharpEngine::CoreCLRLibrary = nullptr;

// === hostfxr函数指针类型定义 ===

typedef int32_t(HOSTFXR_CALLTYPE* hostfxr_initialize_for_runtime_config_fn)(
    const char_t* runtime_config_path,
    const struct hostfxr_initialize_parameters* parameters,
    hostfxr_handle* host_context_handle);

typedef int32_t(HOSTFXR_CALLTYPE* hostfxr_get_runtime_delegate_fn)(const hostfxr_handle host_context_handle,
                                                                   enum hostfxr_delegate_type type,
                                                                   void** delegate);

typedef int32_t(HOSTFXR_CALLTYPE* hostfxr_close_fn)(const hostfxr_handle host_context_handle);

// === CCSharpValue 实现 ===

CCSharpValue::CCSharpValue()
    : DotNetObject(nullptr),
      CachedType(EScriptValueType::Null),
      ReferenceCount(0)
{}

CCSharpValue::CCSharpValue(void* InDotNetObject, const char* InTypeName)
    : DotNetObject(nullptr),
      CachedType(EScriptValueType::Null),
      ReferenceCount(0)
{
	CreateReference(InDotNetObject, InTypeName);
}

CCSharpValue::CCSharpValue(const CCSharpValue& Other)
    : DotNetObject(nullptr),
      CachedType(EScriptValueType::Null),
      ReferenceCount(0)
{
	CopyFrom(Other);
}

CCSharpValue& CCSharpValue::operator=(const CCSharpValue& Other)
{
	if (this != &Other)
	{
		ReleaseReference();
		CopyFrom(Other);
	}
	return *this;
}

CCSharpValue::~CCSharpValue()
{
	ReleaseReference();
}

EScriptValueType CCSharpValue::GetType() const
{
	if (!IsValid())
		return EScriptValueType::Null;

	// 根据.NET类型名称判断类型
	if (DotNetTypeName.Contains(TEXT("System.Boolean")))
		return EScriptValueType::Boolean;
	if (DotNetTypeName.Contains(TEXT("System.Int32")) || DotNetTypeName.Contains(TEXT("System.Int64")) ||
	    DotNetTypeName.Contains(TEXT("System.Single")) || DotNetTypeName.Contains(TEXT("System.Double")))
		return EScriptValueType::Number;
	if (DotNetTypeName.Contains(TEXT("System.String")))
		return EScriptValueType::String;
	if (DotNetTypeName.Contains(TEXT("System.Array")) || DotNetTypeName.Contains(TEXT("[]")))
		return EScriptValueType::Array;
	if (DotNetTypeName.Contains(TEXT("System.Func")) || DotNetTypeName.Contains(TEXT("System.Action")))
		return EScriptValueType::Function;

	return EScriptValueType::Object;
}

bool CCSharpValue::IsNull() const
{
	return DotNetObject == nullptr;
}

bool CCSharpValue::IsBoolean() const
{
	return GetType() == EScriptValueType::Boolean;
}

bool CCSharpValue::IsNumber() const
{
	return GetType() == EScriptValueType::Number;
}

bool CCSharpValue::IsString() const
{
	return GetType() == EScriptValueType::String;
}

bool CCSharpValue::IsArray() const
{
	return GetType() == EScriptValueType::Array;
}

bool CCSharpValue::IsObject() const
{
	return GetType() == EScriptValueType::Object;
}

bool CCSharpValue::IsFunction() const
{
	return GetType() == EScriptValueType::Function;
}

bool CCSharpValue::IsUserData() const
{
	return IsObject() || IsFunction();
}

bool CCSharpValue::ToBool() const
{
	if (!IsValid())
		return false;

	// TODO: 调用.NET方法获取bool值
	// 现在简单判断对象是否为null
	return DotNetObject != nullptr;
}

int32_t CCSharpValue::ToInt32() const
{
	if (!IsValid())
		return 0;

	// TODO: 调用.NET方法转换为int32
	return 0;
}

int64_t CCSharpValue::ToInt64() const
{
	if (!IsValid())
		return 0;

	// TODO: 调用.NET方法转换为int64
	return 0;
}

float CCSharpValue::ToFloat() const
{
	if (!IsValid())
		return 0.0f;

	// TODO: 调用.NET方法转换为float
	return 0.0f;
}

double CCSharpValue::ToDouble() const
{
	if (!IsValid())
		return 0.0;

	// TODO: 调用.NET方法转换为double
	return 0.0;
}

CString CCSharpValue::ToString() const
{
	if (!IsValid())
		return CString();

	// TODO: 调用.NET对象的ToString方法
	return CString::Format(TEXT("C# Object: {}"), DotNetTypeName.GetData());
}

int32_t CCSharpValue::GetArrayLength() const
{
	if (!IsArray())
		return 0;

	// TODO: 调用.NET数组的Length属性
	return 0;
}

CScriptValue CCSharpValue::GetArrayElement(int32_t Index) const
{
	if (!IsArray())
		return CScriptValue();

	// TODO: 调用.NET数组的索引器
	return CScriptValue();
}

void CCSharpValue::SetArrayElement(int32_t Index, const CScriptValue& Value)
{
	if (!IsArray())
		return;

	// TODO: 设置.NET数组元素
}

TArray<CString, CMemoryManager> CCSharpValue::GetObjectKeys() const
{
	TArray<CString, CMemoryManager> Keys;
	if (!IsObject())
		return Keys;

	// TODO: 使用反射获取对象属性名称

	return Keys;
}

CScriptValue CCSharpValue::GetObjectProperty(const CString& Key) const
{
	if (!IsObject())
		return CScriptValue();

	// TODO: 使用反射获取属性值
	return GetProperty(Key);
}

void CCSharpValue::SetObjectProperty(const CString& Key, const CScriptValue& Value)
{
	if (!IsObject())
		return;

	// TODO: 使用反射设置属性值
	if (const CCSharpValue* CSValue = dynamic_cast<const CCSharpValue*>(&Value))
	{
		SetProperty(Key, *CSValue);
	}
}

bool CCSharpValue::HasObjectProperty(const CString& Key) const
{
	if (!IsObject())
		return false;

	// TODO: 使用反射检查属性是否存在
	return false;
}

SScriptExecutionResult CCSharpValue::CallFunction(const TArray<CScriptValue, CMemoryManager>& Args)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!IsFunction())
	{
		Result.ErrorMessage = TEXT("Value is not a function");
		return Result;
	}

	// TODO: 调用.NET委托或方法
	Result.ErrorMessage = TEXT("C# function call not implemented");
	return Result;
}

CConfigValue CCSharpValue::ToConfigValue() const
{
	// TODO: 转换.NET对象为ConfigValue
	return CConfigValue();
}

void CCSharpValue::FromConfigValue(const CConfigValue& Config)
{
	// TODO: 从ConfigValue创建.NET对象
}

bool CCSharpValue::IsValid() const
{
	return DotNetObject != nullptr && !DotNetTypeName.IsEmpty();
}

CCSharpValue CCSharpValue::CallMethod(const CString& MethodName, const TArray<CCSharpValue, CMemoryManager>& Args)
{
	if (!IsValid())
		return CCSharpValue();

	// TODO: 使用反射调用.NET方法
	NLOG_SCRIPT(Warning, "CallMethod not implemented for C# objects");
	return CCSharpValue();
}

CCSharpValue CCSharpValue::GetProperty(const CString& PropertyName)
{
	if (!IsValid())
		return CCSharpValue();

	// TODO: 使用反射获取.NET属性
	NLOG_SCRIPT(Warning, "GetProperty not implemented for C# objects");
	return CCSharpValue();
}

void CCSharpValue::SetProperty(const CString& PropertyName, const CCSharpValue& Value)
{
	if (!IsValid())
		return;

	// TODO: 使用反射设置.NET属性
	NLOG_SCRIPT(Warning, "SetProperty not implemented for C# objects");
}

void CCSharpValue::CreateReference(void* InDotNetObject, const char* InTypeName)
{
	DotNetObject = InDotNetObject;
	DotNetTypeName = CString(InTypeName ? InTypeName : "System.Object");
	ReferenceCount = 1;

	// TODO: 增加.NET对象引用计数或GC句柄
}

void CCSharpValue::ReleaseReference()
{
	if (DotNetObject && ReferenceCount > 0)
	{
		ReferenceCount--;
		if (ReferenceCount == 0)
		{
			// TODO: 释放.NET对象引用或GC句柄
			DotNetObject = nullptr;
		}
	}
	DotNetTypeName.Clear();
}

void CCSharpValue::CopyFrom(const CCSharpValue& Other)
{
	if (Other.IsValid())
	{
		DotNetObject = Other.DotNetObject;
		DotNetTypeName = Other.DotNetTypeName;
		CachedType = Other.CachedType;
		ReferenceCount = Other.ReferenceCount + 1;

		// TODO: 增加.NET对象引用计数
	}
}

// === CCSharpModule 实现 ===

CCSharpModule::CCSharpModule(void* InAssemblyContext, const CString& InName)
    : AssemblyContext(InAssemblyContext),
      ModuleName(InName),
      bLoaded(false),
      LoadedAssembly(nullptr)
{}

CCSharpModule::~CCSharpModule()
{
	Unload();
}

SScriptExecutionResult CCSharpModule::Load(const CString& ModulePath)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!AssemblyContext)
	{
		Result.ErrorMessage = TEXT("Invalid assembly context");
		return Result;
	}

	// TODO: 加载.NET程序集
	// 使用AssemblyLoadContext.LoadFromAssemblyPath或类似方法

	bLoaded = true;
	Result.Result = EScriptResult::Success;
	NLOG_SCRIPT(Info, "C# module '{}' loaded from: {}", ModuleName.GetData(), ModulePath.GetData());

	return Result;
}

SScriptExecutionResult CCSharpModule::Unload()
{
	if (LoadedAssembly)
	{
		// TODO: 卸载.NET程序集
		LoadedAssembly = nullptr;
	}

	GlobalObjects.Clear();
	bLoaded = false;

	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Success;
	return Result;
}

CScriptValue CCSharpModule::GetGlobal(const CString& Name) const
{
	if (GlobalObjects.Contains(Name))
		return GlobalObjects[Name];

	return CScriptValue();
}

void CCSharpModule::SetGlobal(const CString& Name, const CScriptValue& Value)
{
	if (const CCSharpValue* CSValue = dynamic_cast<const CCSharpValue*>(&Value))
	{
		GlobalObjects.Add(Name, *CSValue);
	}
	else
	{
		// 转换其他类型的ScriptValue为CCSharpValue
		CCSharpValue ConvertedValue = CCSharpTypeConverter::ToCSharpValue(Value);
		GlobalObjects.Add(Name, ConvertedValue);
	}
}

SScriptExecutionResult CCSharpModule::ExecuteString(const CString& Code)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!bLoaded)
	{
		Result.ErrorMessage = TEXT("Module not loaded");
		return Result;
	}

	// TODO: 编译并执行C#代码
	// 可能需要使用Roslyn编译器或类似技术

	Result.ErrorMessage = TEXT("C# code execution not implemented");
	return Result;
}

SScriptExecutionResult CCSharpModule::ExecuteFile(const CString& FilePath)
{
	// TODO: 读取文件并执行
	return ExecuteString(TEXT("// File execution not implemented"));
}

void CCSharpModule::RegisterFunction(const CString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 注册C++函数到.NET
	NLOG_SCRIPT(Warning, "RegisterFunction not implemented for C# module");
}

void CCSharpModule::RegisterObject(const CString& Name, const CScriptValue& Object)
{
	SetGlobal(Name, Object);
}

bool CCSharpModule::CompileCSharpCode(const CString& SourceCode, const CString& AssemblyName)
{
	// TODO: 使用Roslyn编译器编译C#代码
	NLOG_SCRIPT(Warning, "C# code compilation not implemented");
	return false;
}

CCSharpValue CCSharpModule::CreateInstance(const CString& TypeName, const TArray<CCSharpValue, CMemoryManager>& Args)
{
	if (!bLoaded || !LoadedAssembly)
		return CCSharpValue();

	// TODO: 使用反射创建.NET类型实例
	NLOG_SCRIPT(Warning, "CreateInstance not implemented");
	return CCSharpValue();
}

CCSharpValue CCSharpModule::CallStaticMethod(const CString& TypeName,
                                             const CString& MethodName,
                                             const TArray<CCSharpValue, CMemoryManager>& Args)
{
	if (!bLoaded || !LoadedAssembly)
		return CCSharpValue();

	// TODO: 使用反射调用静态方法
	NLOG_SCRIPT(Warning, "CallStaticMethod not implemented");
	return CCSharpValue();
}

SScriptExecutionResult CCSharpModule::HandleDotNetError(const CString& Operation, const CString& ErrorMessage)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = CString::Format(TEXT(".NET Error in operation '{}': {}"),
	                                      Operation.GetData(),
	                                      ErrorMessage.IsEmpty() ? TEXT("Unknown error") : ErrorMessage.GetData());

	NLOG_SCRIPT(Error, "C# Module Error: {}", Result.ErrorMessage.GetData());
	return Result;
}

void CCSharpModule::SetupModuleEnvironment()
{
	// 设置模块环境，如全局变量、引用等
}

// === CCSharpContext 实现 ===

CCSharpContext::CCSharpContext()
    : HostHandle(nullptr),
      AssemblyLoadContext(nullptr),
      StartTime(0),
      bTimeoutEnabled(false)
{
	// 清零函数指针结构
	memset(&DotNetFunctions, 0, sizeof(DotNetFunctions));
}

CCSharpContext::~CCSharpContext()
{
	Shutdown();
}

bool CCSharpContext::Initialize(const SScriptConfig& InConfig)
{
	Config = InConfig;

	if (!InitializeDotNet())
	{
		NLOG_SCRIPT(Error, "Failed to initialize .NET runtime for C# context");
		return false;
	}

	RegisterNLibAPI();

	StartTime = GetCurrentTimeMilliseconds();
	bTimeoutEnabled = Config.TimeoutMilliseconds > 0;

	NLOG_SCRIPT(Info, "C# context initialized successfully");
	return true;
}

void CCSharpContext::Shutdown()
{
	Modules.Clear();

	if (AssemblyLoadContext)
	{
		// TODO: 清理AssemblyLoadContext
		AssemblyLoadContext = nullptr;
	}

	ShutdownDotNet();

	NLOG_SCRIPT(Info, "C# context shut down");
}

TSharedPtr<CScriptModule> CCSharpContext::CreateModule(const CString& Name)
{
	if (!HostHandle)
	{
		NLOG_SCRIPT(Error, "C# context not initialized");
		return nullptr;
	}

	if (Modules.Contains(Name))
	{
		NLOG_SCRIPT(Warning, "Module '{}' already exists", Name.GetData());
		return Modules[Name];
	}

	auto Module = MakeShared<CCSharpModule>(AssemblyLoadContext, Name);
	Modules.Add(Name, Module);

	return Module;
}

TSharedPtr<CScriptModule> CCSharpContext::GetModule(const CString& Name) const
{
	if (Modules.Contains(Name))
		return Modules[Name];

	return nullptr;
}

void CCSharpContext::DestroyModule(const CString& Name)
{
	if (Modules.Contains(Name))
	{
		Modules[Name]->Unload();
		Modules.Remove(Name);
	}
}

SScriptExecutionResult CCSharpContext::ExecuteString(const CString& Code, const CString& ModuleName)
{
	if (ModuleName.IsEmpty() || ModuleName == TEXT("__main__"))
	{
		return ExecuteCSharp(Code);
	}

	auto Module = GetModule(ModuleName);
	if (!Module)
	{
		Module = CreateModule(ModuleName);
	}

	return Module->ExecuteString(Code);
}

SScriptExecutionResult CCSharpContext::ExecuteFile(const CString& FilePath, const CString& ModuleName)
{
	// TODO: 读取文件并执行
	return ExecuteString(TEXT("// File execution not implemented"), ModuleName);
}

void CCSharpContext::CollectGarbage()
{
	// TODO: 调用.NET垃圾回收
	// 可能通过P/Invoke调用GC.Collect()
}

uint64_t CCSharpContext::GetMemoryUsage() const
{
	// TODO: 获取.NET托管内存使用量
	// 可能通过P/Invoke调用GC.GetTotalMemory()
	return 0;
}

void CCSharpContext::ResetTimeout()
{
	StartTime = GetCurrentTimeMilliseconds();
}

void CCSharpContext::RegisterGlobalFunction(const CString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 注册全局函数到.NET
	NLOG_SCRIPT(Warning, "RegisterGlobalFunction not implemented for C# context");
}

void CCSharpContext::RegisterGlobalObject(const CString& Name, const CScriptValue& Object)
{
	// TODO: 注册全局对象到.NET
	NLOG_SCRIPT(Warning, "RegisterGlobalObject not implemented for C# context");
}

void CCSharpContext::RegisterGlobalConstant(const CString& Name, const CScriptValue& Value)
{
	RegisterGlobalObject(Name, Value);
}

SScriptExecutionResult CCSharpContext::ExecuteCSharp(const CString& CSharpCode, const CString& AssemblyName)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!HostHandle)
	{
		Result.ErrorMessage = TEXT("C# context not initialized");
		return Result;
	}

	// TODO: 编译并执行C#代码
	// 1. 使用Roslyn编译器编译C#代码到内存程序集
	// 2. 加载程序集到AssemblyLoadContext
	// 3. 查找并调用入口点方法

	Result.ErrorMessage = TEXT("C# code execution not implemented");
	return Result;
}

void* CCSharpContext::LoadAssembly(const CString& AssemblyPath)
{
	if (!AssemblyLoadContext)
		return nullptr;

	// TODO: 使用AssemblyLoadContext加载程序集
	NLOG_SCRIPT(Warning, "LoadAssembly not implemented");
	return nullptr;
}

void CCSharpContext::SetRuntimeConfig(const THashMap<CString, CString, CMemoryManager>& InConfig)
{
	RuntimeConfig = InConfig;
}

bool CCSharpContext::InitializeDotNet()
{
	// 获取.NET运行时路径
	char_t buffer[MAX_PATH];
	size_t buffer_size = sizeof(buffer) / sizeof(char_t);

	int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
	if (rc != 0)
	{
		NLOG_SCRIPT(Error, "Failed to get hostfxr path: {}", rc);
		return false;
	}

	// 加载hostfxr库
#ifdef _WIN32
	HMODULE lib = LoadLibraryW(buffer);
#else
	void* lib = dlopen(buffer, RTLD_LAZY | RTLD_LOCAL);
#endif

	if (!lib)
	{
		NLOG_SCRIPT(Error, "Failed to load hostfxr library");
		return false;
	}

	// 获取hostfxr函数指针
#ifdef _WIN32
	hostfxr_initialize_for_runtime_config_fn init_fptr = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(
	    lib, "hostfxr_initialize_for_runtime_config");
	hostfxr_get_runtime_delegate_fn get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GetProcAddress(
	    lib, "hostfxr_get_runtime_delegate");
	hostfxr_close_fn close_fptr = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");
#else
	hostfxr_initialize_for_runtime_config_fn init_fptr = (hostfxr_initialize_for_runtime_config_fn)dlsym(
	    lib, "hostfxr_initialize_for_runtime_config");
	hostfxr_get_runtime_delegate_fn get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)dlsym(
	    lib, "hostfxr_get_runtime_delegate");
	hostfxr_close_fn close_fptr = (hostfxr_close_fn)dlsym(lib, "hostfxr_close");
#endif

	if (!init_fptr || !get_delegate_fptr || !close_fptr)
	{
		NLOG_SCRIPT(Error, "Failed to get hostfxr function pointers");
		return false;
	}

	// 初始化.NET运行时
	// TODO: 创建或使用运行时配置文件
	const char_t* config_path = nullptr; // 使用默认配置

	rc = init_fptr(config_path, nullptr, &HostHandle);
	if (rc != 0 || !HostHandle)
	{
		NLOG_SCRIPT(Error, "Failed to initialize .NET runtime: {}", rc);
		return false;
	}

	// 获取运行时委托
	void* load_assembly_and_get_function_pointer = nullptr;
	rc = get_delegate_fptr(
	    HostHandle, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_function_pointer);
	if (rc != 0 || !load_assembly_and_get_function_pointer)
	{
		NLOG_SCRIPT(Error, "Failed to get load_assembly_and_get_function_pointer delegate: {}", rc);
		return false;
	}

	DotNetFunctions.LoadAssemblyAndGetFunctionPointer = (void* (*)(const char*, const char*, const char*, const char*))
	    load_assembly_and_get_function_pointer;

	NLOG_SCRIPT(Info, ".NET runtime initialized successfully");
	return true;
}

void CCSharpContext::ShutdownDotNet()
{
	if (HostHandle)
	{
		// TODO: 调用hostfxr_close关闭运行时句柄
		HostHandle = nullptr;
	}

	// 清零函数指针
	memset(&DotNetFunctions, 0, sizeof(DotNetFunctions));
}

SScriptExecutionResult CCSharpContext::HandleDotNetError(const CString& Operation, const CString& ErrorMessage)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = CString::Format(TEXT(".NET Error in operation '{}': {}"),
	                                      Operation.GetData(),
	                                      ErrorMessage.IsEmpty() ? TEXT("Unknown error") : ErrorMessage.GetData());

	NLOG_SCRIPT(Error, "C# Context Error: {}", Result.ErrorMessage.GetData());
	return Result;
}

void CCSharpContext::RegisterNLibAPI()
{
	// TODO: 注册NLib API到.NET运行时
	// 可能需要创建P/Invoke声明或使用其他互操作机制
}

void CCSharpContext::ErrorCallback(const char* message)
{
	NLOG_SCRIPT(Error, ".NET Runtime Error: {}", message ? message : "Unknown error");
}

// === CCSharpEngine 实现 ===

CCSharpEngine::CCSharpEngine()
    : bInitialized(false)
{}

CCSharpEngine::~CCSharpEngine()
{
	Shutdown();
}

CString CCSharpEngine::GetVersion() const
{
	return GetDotNetVersionString();
}

bool CCSharpEngine::IsSupported() const
{
	return IsDotNetAvailable();
}

TSharedPtr<CScriptContext> CCSharpEngine::CreateContext(const SScriptConfig& Config)
{
	if (!bInitialized)
	{
		NLOG_SCRIPT(Error, "C# engine not initialized");
		return nullptr;
	}

	auto Context = MakeShared<CCSharpContext>();
	if (!Context->Initialize(Config))
	{
		NLOG_SCRIPT(Error, "Failed to initialize C# context");
		return nullptr;
	}

	ActiveContexts.Add(Context);
	return Context;
}

void CCSharpEngine::DestroyContext(TSharedPtr<CScriptContext> Context)
{
	if (Context)
	{
		Context->Shutdown();
		ActiveContexts.RemoveSwap(Context);
	}
}

bool CCSharpEngine::Initialize()
{
	if (bInitialized)
		return true;

	if (!InitializeDotNetRuntime())
	{
		NLOG_SCRIPT(Error, "Failed to initialize .NET runtime");
		return false;
	}

	if (!LoadDotNetRuntimeLibraries())
	{
		NLOG_SCRIPT(Error, "Failed to load .NET runtime libraries");
		return false;
	}

	RegisterStandardLibraries();

	bInitialized = true;
	NLOG_SCRIPT(Info, "C# engine initialized successfully");
	return true;
}

void CCSharpEngine::Shutdown()
{
	if (!bInitialized)
		return;

	// 清理所有活跃的上下文
	for (auto& Context : ActiveContexts)
	{
		Context->Shutdown();
	}
	ActiveContexts.Clear();

	ShutdownDotNetRuntime();

	bInitialized = false;
	NLOG_SCRIPT(Info, "C# engine shut down");
}

CScriptValue CCSharpEngine::CreateValue()
{
	// 需要在具体的上下文中创建值
	return CScriptValue();
}

CScriptValue CCSharpEngine::CreateNull()
{
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateBool(bool Value)
{
	// TODO: 创建.NET Boolean对象
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateInt(int32_t Value)
{
	// TODO: 创建.NET Int32对象
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateFloat(float Value)
{
	// TODO: 创建.NET Single对象
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateString(const CString& Value)
{
	// TODO: 创建.NET String对象
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateArray()
{
	// TODO: 创建.NET数组对象
	return CCSharpValue();
}

CScriptValue CCSharpEngine::CreateObject()
{
	// TODO: 创建.NET对象
	return CCSharpValue();
}

SScriptExecutionResult CCSharpEngine::CheckSyntax(const CString& Code)
{
	// TODO: 使用Roslyn进行语法检查
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Success;
	return Result;
}

SScriptExecutionResult CCSharpEngine::CompileFile(const CString& FilePath, const CString& OutputPath)
{
	return CompileCSharpFile(FilePath, OutputPath);
}

CString CCSharpEngine::GetDotNetVersionString()
{
	// TODO: 获取.NET运行时版本
	return TEXT(".NET Core 6.0+");
}

bool CCSharpEngine::IsDotNetAvailable()
{
	// 检查系统是否安装了.NET运行时
	char_t buffer[MAX_PATH];
	size_t buffer_size = sizeof(buffer) / sizeof(char_t);

	int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
	return rc == 0;
}

bool CCSharpEngine::InitializeDotNetRuntime()
{
	if (bDotNetRuntimeInitialized)
		return true;

	// 全局.NET运行时初始化
	bDotNetRuntimeInitialized = true;
	return true;
}

void CCSharpEngine::ShutdownDotNetRuntime()
{
	if (!bDotNetRuntimeInitialized)
		return;

	// 全局.NET运行时清理
	bDotNetRuntimeInitialized = false;
}

SScriptExecutionResult CCSharpEngine::CompileCSharpFile(const CString& InputPath,
                                                        const CString& OutputPath,
                                                        const TArray<CString, CMemoryManager>& References)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = TEXT("C# file compilation not implemented");

	// TODO: 使用Roslyn编译器编译C#文件

	return Result;
}

void CCSharpEngine::SetCompilerOptions(const THashMap<CString, CString, CMemoryManager>& Options)
{
	CompilerOptions = Options;
}

void CCSharpEngine::RegisterStandardLibraries()
{
	// TODO: 注册标准.NET库和类型
}

bool CCSharpEngine::LoadDotNetRuntimeLibraries()
{
	// TODO: 加载必要的.NET运行时库
	return true;
}

// === CCSharpTypeConverter 实现 ===

template <>
void* CCSharpTypeConverter::ToDotNetValue<bool>(const bool& Value)
{
	// TODO: 创建.NET Boolean对象
	return nullptr;
}

template <>
void* CCSharpTypeConverter::ToDotNetValue<int32_t>(const int32_t& Value)
{
	// TODO: 创建.NET Int32对象
	return nullptr;
}

template <>
void* CCSharpTypeConverter::ToDotNetValue<float>(const float& Value)
{
	// TODO: 创建.NET Single对象
	return nullptr;
}

template <>
void* CCSharpTypeConverter::ToDotNetValue<CString>(const CString& Value)
{
	// TODO: 创建.NET String对象
	return nullptr;
}

template <>
bool CCSharpTypeConverter::FromDotNetValue<bool>(void* DotNetValue)
{
	// TODO: 从.NET Boolean对象获取值
	return false;
}

template <>
int32_t CCSharpTypeConverter::FromDotNetValue<int32_t>(void* DotNetValue)
{
	// TODO: 从.NET Int32对象获取值
	return 0;
}

template <>
float CCSharpTypeConverter::FromDotNetValue<float>(void* DotNetValue)
{
	// TODO: 从.NET Single对象获取值
	return 0.0f;
}

template <>
CString CCSharpTypeConverter::FromDotNetValue<CString>(void* DotNetValue)
{
	// TODO: 从.NET String对象获取值
	return CString();
}

CCSharpValue CCSharpTypeConverter::ToCSharpValue(const CScriptValue& ScriptValue)
{
	// 如果已经是CCSharpValue，直接返回
	if (const CCSharpValue* CSValue = dynamic_cast<const CCSharpValue*>(&ScriptValue))
	{
		return *CSValue;
	}

	// 根据类型转换其他ScriptValue
	switch (ScriptValue.GetType())
	{
	case EScriptValueType::Null:
		return CCSharpValue();
	case EScriptValueType::Boolean:
		return CCSharpValue(ToDotNetValue(ScriptValue.ToBool()), "System.Boolean");
	case EScriptValueType::Number:
		return CCSharpValue(ToDotNetValue(ScriptValue.ToDouble()), "System.Double");
	case EScriptValueType::String:
		return CCSharpValue(ToDotNetValue(ScriptValue.ToString()), "System.String");
	default:
		return CCSharpValue();
	}
}

CScriptValue CCSharpTypeConverter::FromCSharpValue(const CCSharpValue& CSharpValue)
{
	return CSharpValue;
}

void* CCSharpTypeConverter::ConfigValueToDotNet(const CConfigValue& Config)
{
	// TODO: 转换ConfigValue为.NET对象
	return nullptr;
}

CConfigValue CCSharpTypeConverter::DotNetToConfigValue(void* DotNetValue)
{
	// TODO: 转换.NET对象为ConfigValue
	return CConfigValue();
}

CString CCSharpTypeConverter::GetDotNetTypeName(void* DotNetValue)
{
	// TODO: 获取.NET对象的类型名称
	return TEXT("System.Object");
}

void* CCSharpTypeConverter::CreateDotNetArray(const TArray<void*, CMemoryManager>& Elements,
                                              const char* ElementTypeName)
{
	// TODO: 创建.NET数组
	return nullptr;
}

TArray<void*, CMemoryManager> CCSharpTypeConverter::GetDotNetArrayElements(void* DotNetArray)
{
	// TODO: 获取.NET数组元素
	TArray<void*, CMemoryManager> Elements;
	return Elements;
}

} // namespace NLib