#include "TypeScriptEngine.h"

#include "IO/Path.h"
#include "Logging/LogCategory.h"
#include "Memory/MemoryManager.h"

#include <v8.h>

#include <libplatform/libplatform.h>

namespace NLib
{
// === 静态成员初始化 ===

bool CTypeScriptEngine::bV8PlatformInitialized = false;
std::unique_ptr<v8::Platform> CTypeScriptEngine::V8Platform = nullptr;

// === CTypeScriptValue 实现 ===

CTypeScriptValue::CTypeScriptValue()
    : Isolate(nullptr),
      PersistentValue(nullptr),
      CachedType(EScriptValueType::Null)
{}

CTypeScriptValue::CTypeScriptValue(v8::Isolate* InIsolate, v8::Local<v8::Value> InValue)
    : Isolate(InIsolate),
      PersistentValue(nullptr),
      CachedType(EScriptValueType::Null)
{
	CreatePersistent(InIsolate, InValue);
}

CTypeScriptValue::CTypeScriptValue(const CTypeScriptValue& Other)
    : Isolate(nullptr),
      PersistentValue(nullptr),
      CachedType(EScriptValueType::Null)
{
	CopyFrom(Other);
}

CTypeScriptValue& CTypeScriptValue::operator=(const CTypeScriptValue& Other)
{
	if (this != &Other)
	{
		ReleasePersistent();
		CopyFrom(Other);
	}
	return *this;
}

CTypeScriptValue::~CTypeScriptValue()
{
	ReleasePersistent();
}

EScriptValueType CTypeScriptValue::GetType() const
{
	if (!IsValid() || !Isolate)
		return EScriptValueType::Null;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Value> Value = GetV8Value();

	if (Value.IsEmpty() || Value->IsNull() || Value->IsUndefined())
		return EScriptValueType::Null;
	if (Value->IsBoolean())
		return EScriptValueType::Boolean;
	if (Value->IsNumber())
		return EScriptValueType::Number;
	if (Value->IsString())
		return EScriptValueType::String;
	if (Value->IsArray())
		return EScriptValueType::Array;
	if (Value->IsFunction())
		return EScriptValueType::Function;
	if (Value->IsObject())
		return EScriptValueType::Object;

	return EScriptValueType::UserData;
}

bool CTypeScriptValue::IsNull() const
{
	if (!IsValid())
		return true;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Value> Value = GetV8Value();
	return Value.IsEmpty() || Value->IsNull() || Value->IsUndefined();
}

bool CTypeScriptValue::IsBoolean() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	return GetV8Value()->IsBoolean();
}

bool CTypeScriptValue::IsNumber() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	return GetV8Value()->IsNumber();
}

bool CTypeScriptValue::IsString() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	return GetV8Value()->IsString();
}

bool CTypeScriptValue::IsArray() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	return GetV8Value()->IsArray();
}

bool CTypeScriptValue::IsObject() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Value> Value = GetV8Value();
	return Value->IsObject() && !Value->IsFunction() && !Value->IsArray();
}

bool CTypeScriptValue::IsFunction() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	return GetV8Value()->IsFunction();
}

bool CTypeScriptValue::IsUserData() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Value> Value = GetV8Value();
	return Value->IsExternal() || (Value->IsObject() && !Value->IsArray() && !Value->IsFunction());
}

bool CTypeScriptValue::ToBool() const
{
	if (!IsValid())
		return false;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	return GetV8Value()->BooleanValue(Isolate);
}

int32_t CTypeScriptValue::ToInt32() const
{
	if (!IsValid())
		return 0;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	auto MaybeInt = GetV8Value()->Int32Value(Context);
	return MaybeInt.IsJust() ? MaybeInt.FromJust() : 0;
}

int64_t CTypeScriptValue::ToInt64() const
{
	if (!IsValid())
		return 0;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	auto MaybeInt = GetV8Value()->IntegerValue(Context);
	return MaybeInt.IsJust() ? MaybeInt.FromJust() : 0;
}

float CTypeScriptValue::ToFloat() const
{
	return static_cast<float>(ToDouble());
}

double CTypeScriptValue::ToDouble() const
{
	if (!IsValid())
		return 0.0;
	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	auto MaybeDouble = GetV8Value()->NumberValue(Context);
	return MaybeDouble.IsJust() ? MaybeDouble.FromJust() : 0.0;
}

CString CTypeScriptValue::ToString() const
{
	if (!IsValid())
		return CString();

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Value> Value = GetV8Value();

	v8::Local<v8::String> StringValue;
	if (!Value->ToString(Context).ToLocal(&StringValue))
		return CString();

	v8::String::Utf8Value Utf8Value(Isolate, StringValue);
	return CString(*Utf8Value);
}

int32_t CTypeScriptValue::GetArrayLength() const
{
	if (!IsArray())
		return 0;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Array> Array = v8::Local<v8::Array>::Cast(GetV8Value());

	return Array->Length();
}

CScriptValue CTypeScriptValue::GetArrayElement(int32_t Index) const
{
	if (!IsArray())
		return CScriptValue();

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Array> Array = v8::Local<v8::Array>::Cast(GetV8Value());

	v8::Local<v8::Value> Element;
	if (Array->Get(Context, Index).ToLocal(&Element))
	{
		return CTypeScriptValue(Isolate, Element);
	}

	return CScriptValue();
}

void CTypeScriptValue::SetArrayElement(int32_t Index, const CScriptValue& Value)
{
	if (!IsArray())
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Array> Array = v8::Local<v8::Array>::Cast(GetV8Value());

	// 转换CScriptValue为V8值
	v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ToV8Value(Isolate, Value);
	Array->Set(Context, Index, V8Value);
}

TArray<CString, CMemoryManager> CTypeScriptValue::GetObjectKeys() const
{
	TArray<CString, CMemoryManager> Keys;
	if (!IsObject())
		return Keys;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Object> Object = v8::Local<v8::Object>::Cast(GetV8Value());

	v8::Local<v8::Array> PropertyNames;
	if (Object->GetPropertyNames(Context).ToLocal(&PropertyNames))
	{
		uint32_t Length = PropertyNames->Length();
		for (uint32_t i = 0; i < Length; ++i)
		{
			v8::Local<v8::Value> Key;
			if (PropertyNames->Get(Context, i).ToLocal(&Key))
			{
				v8::String::Utf8Value Utf8Key(Isolate, Key);
				Keys.Add(CString(*Utf8Key));
			}
		}
	}

	return Keys;
}

CScriptValue CTypeScriptValue::GetObjectProperty(const CString& Key) const
{
	if (!IsObject())
		return CScriptValue();

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Object> Object = v8::Local<v8::Object>::Cast(GetV8Value());

	v8::Local<v8::String> KeyString = v8::String::NewFromUtf8(Isolate, Key.GetData()).ToLocalChecked();
	v8::Local<v8::Value> Property;

	if (Object->Get(Context, KeyString).ToLocal(&Property))
	{
		return CTypeScriptValue(Isolate, Property);
	}

	return CScriptValue();
}

void CTypeScriptValue::SetObjectProperty(const CString& Key, const CScriptValue& Value)
{
	if (!IsObject())
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Object> Object = v8::Local<v8::Object>::Cast(GetV8Value());

	v8::Local<v8::String> KeyString = v8::String::NewFromUtf8(Isolate, Key.GetData()).ToLocalChecked();
	v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ToV8Value(Isolate, Value);

	Object->Set(Context, KeyString, V8Value);
}

bool CTypeScriptValue::HasObjectProperty(const CString& Key) const
{
	if (!IsObject())
		return false;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Object> Object = v8::Local<v8::Object>::Cast(GetV8Value());

	v8::Local<v8::String> KeyString = v8::String::NewFromUtf8(Isolate, Key.GetData()).ToLocalChecked();
	auto MaybeHas = Object->Has(Context, KeyString);

	return MaybeHas.IsJust() && MaybeHas.FromJust();
}

SScriptExecutionResult CTypeScriptValue::CallFunction(const TArray<CScriptValue, CMemoryManager>& Args)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!IsFunction())
	{
		Result.ErrorMessage = TEXT("Value is not a function");
		return Result;
	}

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = Isolate->GetCurrentContext();
	v8::Local<v8::Function> Function = v8::Local<v8::Function>::Cast(GetV8Value());

	// 转换参数
	TArray<v8::Local<v8::Value>, CMemoryManager> V8Args;
	for (const auto& Arg : Args)
	{
		V8Args.Add(CTypeScriptTypeConverter::ToV8Value(Isolate, Arg));
	}

	// 调用函数
	v8::Local<v8::Value> CallResult;
	if (Function->Call(Context, Context->Global(), V8Args.Size(), V8Args.GetData()).ToLocal(&CallResult))
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CTypeScriptValue(Isolate, CallResult);
	}
	else
	{
		Result.ErrorMessage = TEXT("Function call failed");
	}

	return Result;
}

CConfigValue CTypeScriptValue::ToConfigValue() const
{
	if (!IsValid())
		return CConfigValue();

	v8::HandleScope HandleScope(Isolate);
	return CTypeScriptTypeConverter::V8ToConfigValue(Isolate, GetV8Value());
}

void CTypeScriptValue::FromConfigValue(const CConfigValue& Config)
{
	if (!Isolate)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ConfigValueToV8(Isolate, Config);
	CreatePersistent(Isolate, V8Value);
}

v8::Local<v8::Value> CTypeScriptValue::GetV8Value() const
{
	if (!PersistentValue || !Isolate)
		return v8::Local<v8::Value>();

	return PersistentValue->Get(Isolate);
}

bool CTypeScriptValue::IsValid() const
{
	return Isolate != nullptr && PersistentValue != nullptr && !PersistentValue->IsEmpty();
}

void CTypeScriptValue::CreatePersistent(v8::Isolate* InIsolate, v8::Local<v8::Value> InValue)
{
	Isolate = InIsolate;
	if (PersistentValue)
	{
		PersistentValue->Reset();
		auto& MemMgr = CMemoryManager::GetInstance();
		MemMgr.Delete(PersistentValue);
	}

	auto& MemMgr = CMemoryManager::GetInstance();
	PersistentValue = MemMgr.New<v8::Persistent<v8::Value>>(Isolate, InValue);
}

void CTypeScriptValue::ReleasePersistent()
{
	if (PersistentValue)
	{
		PersistentValue->Reset();
		auto& MemMgr = CMemoryManager::GetInstance();
		MemMgr.Delete(PersistentValue);
		PersistentValue = nullptr;
	}
	Isolate = nullptr;
}

void CTypeScriptValue::CopyFrom(const CTypeScriptValue& Other)
{
	if (Other.IsValid())
	{
		Isolate = Other.Isolate;
		v8::HandleScope HandleScope(Isolate);
		CreatePersistent(Isolate, Other.GetV8Value());
	}
	CachedType = Other.CachedType;
}

// === CTypeScriptModule 实现 ===

CTypeScriptModule::CTypeScriptModule(v8::Isolate* InIsolate, const CString& InName)
    : Isolate(InIsolate),
      ModuleName(InName),
      bLoaded(false),
      ModuleContext(nullptr)
{}

CTypeScriptModule::~CTypeScriptModule()
{
	Unload();
}

SScriptExecutionResult CTypeScriptModule::Load(const CString& ModulePath)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!Isolate)
	{
		Result.ErrorMessage = TEXT("Invalid V8 isolate");
		return Result;
	}

	// 读取文件内容
	// TODO: 使用NLib的文件系统读取文件

	bLoaded = true;
	Result.Result = EScriptResult::Success;
	return Result;
}

SScriptExecutionResult CTypeScriptModule::Unload()
{
	if (ModuleContext)
	{
		ModuleContext->Reset();
		auto& MemMgr = CMemoryManager::GetInstance();
		MemMgr.Delete(ModuleContext);
		ModuleContext = nullptr;
	}

	bLoaded = false;

	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Success;
	return Result;
}

CScriptValue CTypeScriptModule::GetGlobal(const CString& Name) const
{
	if (!bLoaded || !Isolate)
		return CScriptValue();

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = ModuleContext ? ModuleContext->Get(Isolate) : Isolate->GetCurrentContext();

	v8::Local<v8::String> NameString = v8::String::NewFromUtf8(Isolate, Name.GetData()).ToLocalChecked();
	v8::Local<v8::Value> Global;

	if (Context->Global()->Get(Context, NameString).ToLocal(&Global))
	{
		return CTypeScriptValue(Isolate, Global);
	}

	return CScriptValue();
}

void CTypeScriptModule::SetGlobal(const CString& Name, const CScriptValue& Value)
{
	if (!bLoaded || !Isolate)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = ModuleContext ? ModuleContext->Get(Isolate) : Isolate->GetCurrentContext();

	v8::Local<v8::String> NameString = v8::String::NewFromUtf8(Isolate, Name.GetData()).ToLocalChecked();
	v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ToV8Value(Isolate, Value);

	Context->Global()->Set(Context, NameString, V8Value);
}

SScriptExecutionResult CTypeScriptModule::ExecuteString(const CString& Code)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!Isolate)
	{
		Result.ErrorMessage = TEXT("Invalid V8 isolate");
		return Result;
	}

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = ModuleContext ? ModuleContext->Get(Isolate) : Isolate->GetCurrentContext();
	v8::Context::Scope ContextScope(Context);

	// 编译代码
	v8::Local<v8::String> Source = v8::String::NewFromUtf8(Isolate, Code.GetData()).ToLocalChecked();
	v8::Local<v8::Script> Script;

	if (!v8::Script::Compile(Context, Source).ToLocal(&Script))
	{
		Result.ErrorMessage = TEXT("Failed to compile TypeScript/JavaScript code");
		return Result;
	}

	// 执行脚本
	v8::Local<v8::Value> ExecutionResult;
	if (Script->Run(Context).ToLocal(&ExecutionResult))
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CTypeScriptValue(Isolate, ExecutionResult);
	}
	else
	{
		Result.ErrorMessage = TEXT("Script execution failed");
	}

	return Result;
}

SScriptExecutionResult CTypeScriptModule::ExecuteFile(const CString& FilePath)
{
	// TODO: 读取文件并执行
	return ExecuteString(TEXT("// File execution not implemented"));
}

void CTypeScriptModule::RegisterFunction(const CString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 注册C++函数到V8
	NLOG_SCRIPT(Warning, "RegisterFunction not implemented for TypeScript module");
}

void CTypeScriptModule::RegisterObject(const CString& Name, const CScriptValue& Object)
{
	SetGlobal(Name, Object);
}

CString CTypeScriptModule::CompileTypeScript(const CString& TypeScriptCode)
{
	// TODO: 集成TypeScript编译器
	// 现在只返回原代码作为JavaScript
	return TypeScriptCode;
}

void CTypeScriptModule::SetModuleExports(const CString& Name, const CScriptValue& Value)
{
	if (!bLoaded || !Isolate)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = ModuleContext ? ModuleContext->Get(Isolate) : Isolate->GetCurrentContext();

	// 设置module.exports[Name] = Value
	v8::Local<v8::String> ModuleName = v8::String::NewFromUtf8(Isolate, "module").ToLocalChecked();
	v8::Local<v8::Object> Module;

	if (Context->Global()->Get(Context, ModuleName).ToLocal(reinterpret_cast<v8::Local<v8::Value>*>(&Module)) &&
	    Module->IsObject())
	{
		v8::Local<v8::String> ExportsName = v8::String::NewFromUtf8(Isolate, "exports").ToLocalChecked();
		v8::Local<v8::Object> Exports;

		if (Module->Get(Context, ExportsName).ToLocal(reinterpret_cast<v8::Local<v8::Value>*>(&Exports)) &&
		    Exports->IsObject())
		{
			v8::Local<v8::String> PropertyName = v8::String::NewFromUtf8(Isolate, Name.GetData()).ToLocalChecked();
			v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ToV8Value(Isolate, Value);
			Exports->Set(Context, PropertyName, V8Value);
		}
	}
}

SScriptExecutionResult CTypeScriptModule::HandleV8Error(const CString& Operation)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = CString::Format(TEXT("V8 Error in operation: {}"), Operation.GetData());

	// TODO: 获取V8具体错误信息

	return Result;
}

void CTypeScriptModule::SetupModuleEnvironment()
{
	if (!Isolate)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = ModuleContext ? ModuleContext->Get(Isolate) : Isolate->GetCurrentContext();

	// 设置基础的模块环境
	v8::Local<v8::Object> Module = v8::Object::New(Isolate);
	v8::Local<v8::Object> Exports = v8::Object::New(Isolate);

	v8::Local<v8::String> ExportsName = v8::String::NewFromUtf8(Isolate, "exports").ToLocalChecked();
	Module->Set(Context, ExportsName, Exports);

	v8::Local<v8::String> ModuleName = v8::String::NewFromUtf8(Isolate, "module").ToLocalChecked();
	Context->Global()->Set(Context, ModuleName, Module);
}

// === CTypeScriptContext 实现 ===

CTypeScriptContext::CTypeScriptContext()
    : Isolate(nullptr),
      GlobalContext(nullptr),
      StartTime(0),
      bTimeoutEnabled(false)
{}

CTypeScriptContext::~CTypeScriptContext()
{
	Shutdown();
}

bool CTypeScriptContext::Initialize(const SScriptConfig& InConfig)
{
	Config = InConfig;

	if (!InitializeV8())
	{
		NLOG_SCRIPT(Error, "Failed to initialize V8 for TypeScript context");
		return false;
	}

	RegisterNLibAPI();

	StartTime = GetCurrentTimeMilliseconds();
	bTimeoutEnabled = Config.TimeoutMilliseconds > 0;

	NLOG_SCRIPT(Info, "TypeScript context initialized successfully");
	return true;
}

void CTypeScriptContext::Shutdown()
{
	Modules.Clear();

	if (GlobalContext)
	{
		GlobalContext->Reset();
		auto& MemMgr = CMemoryManager::GetInstance();
		MemMgr.Delete(GlobalContext);
		GlobalContext = nullptr;
	}

	ShutdownV8();

	NLOG_SCRIPT(Info, "TypeScript context shut down");
}

TSharedPtr<CScriptModule> CTypeScriptContext::CreateModule(const CString& Name)
{
	if (!Isolate)
		return nullptr;

	if (Modules.Contains(Name))
	{
		NLOG_SCRIPT(Warning, "Module '{}' already exists", Name.GetData());
		return Modules[Name];
	}

	auto Module = MakeShared<CTypeScriptModule>(Isolate, Name);
	Modules.Add(Name, Module);

	return Module;
}

TSharedPtr<CScriptModule> CTypeScriptContext::GetModule(const CString& Name) const
{
	if (Modules.Contains(Name))
		return Modules[Name];

	return nullptr;
}

void CTypeScriptContext::DestroyModule(const CString& Name)
{
	if (Modules.Contains(Name))
	{
		Modules[Name]->Unload();
		Modules.Remove(Name);
	}
}

SScriptExecutionResult CTypeScriptContext::ExecuteString(const CString& Code, const CString& ModuleName)
{
	if (ModuleName.IsEmpty() || ModuleName == TEXT("__main__"))
	{
		return ExecuteTypeScript(Code);
	}

	auto Module = GetModule(ModuleName);
	if (!Module)
	{
		Module = CreateModule(ModuleName);
	}

	return Module->ExecuteString(Code);
}

SScriptExecutionResult CTypeScriptContext::ExecuteFile(const CString& FilePath, const CString& ModuleName)
{
	// TODO: 读取文件并执行
	return ExecuteString(TEXT("// File execution not implemented"), ModuleName);
}

void CTypeScriptContext::CollectGarbage()
{
	if (Isolate)
	{
		v8::HandleScope HandleScope(Isolate);
		Isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
	}
}

uint64_t CTypeScriptContext::GetMemoryUsage() const
{
	if (!Isolate)
		return 0;

	v8::HeapStatistics HeapStats;
	Isolate->GetHeapStatistics(&HeapStats);
	return HeapStats.used_heap_size();
}

void CTypeScriptContext::ResetTimeout()
{
	StartTime = GetCurrentTimeMilliseconds();
}

void CTypeScriptContext::RegisterGlobalFunction(const CString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 注册全局函数到V8
	NLOG_SCRIPT(Warning, "RegisterGlobalFunction not implemented for TypeScript context");
}

void CTypeScriptContext::RegisterGlobalObject(const CString& Name, const CScriptValue& Object)
{
	if (!Isolate || !GlobalContext)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = GlobalContext->Get(Isolate);
	v8::Context::Scope ContextScope(Context);

	v8::Local<v8::String> NameString = v8::String::NewFromUtf8(Isolate, Name.GetData()).ToLocalChecked();
	v8::Local<v8::Value> V8Value = CTypeScriptTypeConverter::ToV8Value(Isolate, Object);

	Context->Global()->Set(Context, NameString, V8Value);
}

void CTypeScriptContext::RegisterGlobalConstant(const CString& Name, const CScriptValue& Value)
{
	RegisterGlobalObject(Name, Value);
}

v8::Local<v8::Context> CTypeScriptContext::GetGlobalContext() const
{
	if (GlobalContext)
		return GlobalContext->Get(Isolate);

	return v8::Local<v8::Context>();
}

SScriptExecutionResult CTypeScriptContext::ExecuteTypeScript(const CString& TypeScriptCode, const CString& ModuleName)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!Isolate || !GlobalContext)
	{
		Result.ErrorMessage = TEXT("TypeScript context not initialized");
		return Result;
	}

	// 编译TypeScript到JavaScript
	CString JavaScriptCode = CompileTypeScriptToJavaScript(TypeScriptCode);

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = GlobalContext->Get(Isolate);
	v8::Context::Scope ContextScope(Context);

	// 编译JavaScript
	v8::Local<v8::String> Source = v8::String::NewFromUtf8(Isolate, JavaScriptCode.GetData()).ToLocalChecked();
	v8::Local<v8::Script> Script;

	if (!v8::Script::Compile(Context, Source).ToLocal(&Script))
	{
		Result.ErrorMessage = TEXT("Failed to compile JavaScript code");
		return Result;
	}

	// 执行脚本
	v8::Local<v8::Value> ExecutionResult;
	if (Script->Run(Context).ToLocal(&ExecutionResult))
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CTypeScriptValue(Isolate, ExecutionResult);
	}
	else
	{
		Result.ErrorMessage = TEXT("Script execution failed");
	}

	return Result;
}

void CTypeScriptContext::SetTypeScriptCompilerOptions(const THashMap<CString, CString, CMemoryManager>& Options)
{
	CompilerOptions = Options;
}

bool CTypeScriptContext::InitializeV8()
{
	// 创建V8 Isolate
	v8::Isolate::CreateParams CreateParams;
	CreateParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

	Isolate = v8::Isolate::New(CreateParams);
	if (!Isolate)
	{
		NLOG_SCRIPT(Error, "Failed to create V8 isolate");
		return false;
	}

	// 设置错误回调
	Isolate->SetFatalErrorHandler(FatalErrorCallback);
	Isolate->AddMessageListener(MessageCallback);

	// 创建全局上下文
	{
		v8::Isolate::Scope IsolateScope(Isolate);
		v8::HandleScope HandleScope(Isolate);

		v8::Local<v8::Context> Context = v8::Context::New(Isolate);
		if (Context.IsEmpty())
		{
			NLOG_SCRIPT(Error, "Failed to create V8 context");
			return false;
		}

		auto& MemMgr = CMemoryManager::GetInstance();
		GlobalContext = MemMgr.New<v8::Persistent<v8::Context>>(Isolate, Context);
	}

	return true;
}

void CTypeScriptContext::ShutdownV8()
{
	if (Isolate)
	{
		Isolate->Dispose();
		Isolate = nullptr;
	}
}

SScriptExecutionResult CTypeScriptContext::HandleV8Error(const CString& Operation)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = CString::Format(TEXT("V8 Error in operation: {}"), Operation.GetData());

	// TODO: 获取V8具体错误信息

	return Result;
}

void CTypeScriptContext::RegisterNLibAPI()
{
	if (!Isolate || !GlobalContext)
		return;

	v8::HandleScope HandleScope(Isolate);
	v8::Local<v8::Context> Context = GlobalContext->Get(Isolate);
	v8::Context::Scope ContextScope(Context);

	// 创建NLib命名空间对象
	v8::Local<v8::Object> NLibObject = v8::Object::New(Isolate);

	// 注册基础函数
	// TODO: 添加具体的NLib API绑定

	v8::Local<v8::String> NLibName = v8::String::NewFromUtf8(Isolate, "NLib").ToLocalChecked();
	Context->Global()->Set(Context, NLibName, NLibObject);
}

CString CTypeScriptContext::CompileTypeScriptToJavaScript(const CString& TypeScriptCode)
{
	// TODO: 集成TypeScript编译器（可能通过Node.js或独立编译器）
	// 现在简单返回原代码作为JavaScript
	return TypeScriptCode;
}

void CTypeScriptContext::MessageCallback(v8::Local<v8::Message> message, v8::Local<v8::Value> error)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::String::Utf8Value msg(isolate, message->Get());
	NLOG_SCRIPT(Error, "V8 Message: {}", *msg);
}

void CTypeScriptContext::FatalErrorCallback(const char* location, const char* message)
{
	NLOG_SCRIPT(Error, "V8 Fatal Error at {}: {}", location ? location : "unknown", message ? message : "unknown");
}

// === CTypeScriptEngine 实现 ===

CTypeScriptEngine::CTypeScriptEngine()
    : bInitialized(false)
{}

CTypeScriptEngine::~CTypeScriptEngine()
{
	Shutdown();
}

CString CTypeScriptEngine::GetVersion() const
{
	return GetV8VersionString();
}

bool CTypeScriptEngine::IsSupported() const
{
	return IsV8Available();
}

TSharedPtr<CScriptContext> CTypeScriptEngine::CreateContext(const SScriptConfig& Config)
{
	if (!bInitialized)
	{
		NLOG_SCRIPT(Error, "TypeScript engine not initialized");
		return nullptr;
	}

	auto Context = MakeShared<CTypeScriptContext>();
	if (!Context->Initialize(Config))
	{
		NLOG_SCRIPT(Error, "Failed to initialize TypeScript context");
		return nullptr;
	}

	ActiveContexts.Add(Context);
	return Context;
}

void CTypeScriptEngine::DestroyContext(TSharedPtr<CScriptContext> Context)
{
	if (Context)
	{
		Context->Shutdown();
		ActiveContexts.RemoveSwap(Context);
	}
}

bool CTypeScriptEngine::Initialize()
{
	if (bInitialized)
		return true;

	if (!InitializeV8Platform())
	{
		NLOG_SCRIPT(Error, "Failed to initialize V8 platform");
		return false;
	}

	InitializeV8Flags();
	RegisterStandardLibraries();

	bInitialized = true;
	NLOG_SCRIPT(Info, "TypeScript engine initialized successfully");
	return true;
}

void CTypeScriptEngine::Shutdown()
{
	if (!bInitialized)
		return;

	// 清理所有活跃的上下文
	for (auto& Context : ActiveContexts)
	{
		Context->Shutdown();
	}
	ActiveContexts.Clear();

	ShutdownV8Platform();

	bInitialized = false;
	NLOG_SCRIPT(Info, "TypeScript engine shut down");
}

CScriptValue CTypeScriptEngine::CreateValue()
{
	// 需要在具体的上下文中创建值
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateNull()
{
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateBool(bool Value)
{
	// 需要V8 Isolate来创建值
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateInt(int32_t Value)
{
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateFloat(float Value)
{
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateString(const CString& Value)
{
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateArray()
{
	return CScriptValue();
}

CScriptValue CTypeScriptEngine::CreateObject()
{
	return CScriptValue();
}

SScriptExecutionResult CTypeScriptEngine::CheckSyntax(const CString& Code)
{
	// TODO: 语法检查实现
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Success;
	return Result;
}

SScriptExecutionResult CTypeScriptEngine::CompileFile(const CString& FilePath, const CString& OutputPath)
{
	return CompileTypeScriptFile(FilePath, OutputPath);
}

CString CTypeScriptEngine::GetV8VersionString()
{
	return CString(v8::V8::GetVersion());
}

bool CTypeScriptEngine::IsV8Available()
{
	return true; // 如果编译了V8支持，就是可用的
}

bool CTypeScriptEngine::InitializeV8Platform()
{
	if (bV8PlatformInitialized)
		return true;

	v8::V8::InitializeICUDefaultLocation("");
	v8::V8::InitializeExternalStartupData("");

	V8Platform = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(V8Platform.get());
	v8::V8::Initialize();

	bV8PlatformInitialized = true;
	return true;
}

void CTypeScriptEngine::ShutdownV8Platform()
{
	if (!bV8PlatformInitialized)
		return;

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	V8Platform.reset();

	bV8PlatformInitialized = false;
}

SScriptExecutionResult CTypeScriptEngine::CompileTypeScriptFile(const CString& InputPath, const CString& OutputPath)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = TEXT("TypeScript file compilation not implemented");

	// TODO: 实现TypeScript文件编译

	return Result;
}

void CTypeScriptEngine::RegisterStandardLibraries()
{
	// TODO: 注册标准库函数
}

void CTypeScriptEngine::InitializeV8Flags()
{
	// 设置V8标志
	v8::V8::SetFlagsFromString("--expose-gc");
}

// === CTypeScriptTypeConverter 实现 ===

template <>
v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value<bool>(v8::Isolate* isolate, const bool& Value)
{
	return v8::Boolean::New(isolate, Value);
}

template <>
v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value<int32_t>(v8::Isolate* isolate, const int32_t& Value)
{
	return v8::Integer::New(isolate, Value);
}

template <>
v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value<float>(v8::Isolate* isolate, const float& Value)
{
	return v8::Number::New(isolate, Value);
}

template <>
v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value<double>(v8::Isolate* isolate, const double& Value)
{
	return v8::Number::New(isolate, Value);
}

template <>
v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value<CString>(v8::Isolate* isolate, const CString& Value)
{
	return v8::String::NewFromUtf8(isolate, Value.GetData()).ToLocalChecked();
}

template <>
bool CTypeScriptTypeConverter::FromV8Value<bool>(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	return Value->BooleanValue(isolate);
}

template <>
int32_t CTypeScriptTypeConverter::FromV8Value<int32_t>(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	v8::Local<v8::Context> Context = isolate->GetCurrentContext();
	auto MaybeInt = Value->Int32Value(Context);
	return MaybeInt.IsJust() ? MaybeInt.FromJust() : 0;
}

template <>
float CTypeScriptTypeConverter::FromV8Value<float>(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	v8::Local<v8::Context> Context = isolate->GetCurrentContext();
	auto MaybeDouble = Value->NumberValue(Context);
	return MaybeDouble.IsJust() ? static_cast<float>(MaybeDouble.FromJust()) : 0.0f;
}

template <>
double CTypeScriptTypeConverter::FromV8Value<double>(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	v8::Local<v8::Context> Context = isolate->GetCurrentContext();
	auto MaybeDouble = Value->NumberValue(Context);
	return MaybeDouble.IsJust() ? MaybeDouble.FromJust() : 0.0;
}

template <>
CString CTypeScriptTypeConverter::FromV8Value<CString>(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	v8::String::Utf8Value Utf8Value(isolate, Value);
	return CString(*Utf8Value);
}

v8::Local<v8::Value> CTypeScriptTypeConverter::ToV8Value(v8::Isolate* isolate, const CScriptValue& ScriptValue)
{
	// 尝试转换为CTypeScriptValue
	if (const CTypeScriptValue* TSValue = dynamic_cast<const CTypeScriptValue*>(&ScriptValue))
	{
		return TSValue->GetV8Value();
	}

	// 根据类型创建V8值
	switch (ScriptValue.GetType())
	{
	case EScriptValueType::Null:
		return v8::Null(isolate);
	case EScriptValueType::Boolean:
		return v8::Boolean::New(isolate, ScriptValue.ToBool());
	case EScriptValueType::Number:
		return v8::Number::New(isolate, ScriptValue.ToDouble());
	case EScriptValueType::String:
		return v8::String::NewFromUtf8(isolate, ScriptValue.ToString().GetData()).ToLocalChecked();
	case EScriptValueType::Array: {
		v8::Local<v8::Array> Array = v8::Array::New(isolate, ScriptValue.GetArrayLength());
		v8::Local<v8::Context> Context = isolate->GetCurrentContext();

		for (int32_t i = 0; i < ScriptValue.GetArrayLength(); ++i)
		{
			v8::Local<v8::Value> Element = ToV8Value(isolate, ScriptValue.GetArrayElement(i));
			Array->Set(Context, i, Element);
		}

		return Array;
	}
	case EScriptValueType::Object: {
		v8::Local<v8::Object> Object = v8::Object::New(isolate);
		v8::Local<v8::Context> Context = isolate->GetCurrentContext();

		auto Keys = ScriptValue.GetObjectKeys();
		for (const auto& Key : Keys)
		{
			v8::Local<v8::String> KeyString = v8::String::NewFromUtf8(isolate, Key.GetData()).ToLocalChecked();
			v8::Local<v8::Value> Value = ToV8Value(isolate, ScriptValue.GetObjectProperty(Key));
			Object->Set(Context, KeyString, Value);
		}

		return Object;
	}
	default:
		return v8::Undefined(isolate);
	}
}

CTypeScriptValue CTypeScriptTypeConverter::ToTypeScriptValue(const CScriptValue& ScriptValue, v8::Isolate* isolate)
{
	v8::Local<v8::Value> V8Value = ToV8Value(isolate, ScriptValue);
	return CTypeScriptValue(isolate, V8Value);
}

CScriptValue CTypeScriptTypeConverter::FromTypeScriptValue(const CTypeScriptValue& TypeScriptValue)
{
	return TypeScriptValue;
}

v8::Local<v8::Value> CTypeScriptTypeConverter::ConfigValueToV8(v8::Isolate* isolate, const CConfigValue& Config)
{
	// TODO: 实现ConfigValue到V8值的转换
	return v8::Undefined(isolate);
}

CConfigValue CTypeScriptTypeConverter::V8ToConfigValue(v8::Isolate* isolate, v8::Local<v8::Value> Value)
{
	// TODO: 实现V8值到ConfigValue的转换
	return CConfigValue();
}

} // namespace NLib