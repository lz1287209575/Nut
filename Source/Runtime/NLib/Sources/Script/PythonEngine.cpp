#include "PythonEngine.h"

#include "IO/Path.h"
#include "Logging/LogCategory.h"
#include "Memory/MemoryManager.h"

// Python C API
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <frameobject.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace NLib
{
// === 静态成员初始化 ===

bool CPythonEngine::bPythonInterpreterInitialized = false;
void* CPythonEngine::PythonLibrary = nullptr;

// === CPythonValue 实现 ===

CPythonValue::CPythonValue()
    : PyObjectPtr(nullptr),
      CachedType(EScriptValueType::Null)
{}

CPythonValue::CPythonValue(PyObject* InPyObject)
    : PyObjectPtr(InPyObject),
      CachedType(EScriptValueType::Null)
{
	IncRef();
}

CPythonValue::CPythonValue(const CPythonValue& Other)
    : PyObjectPtr(nullptr),
      CachedType(EScriptValueType::Null)
{
	CopyFrom(Other);
}

CPythonValue& CPythonValue::operator=(const CPythonValue& Other)
{
	if (this != &Other)
	{
		DecRef();
		CopyFrom(Other);
	}
	return *this;
}

CPythonValue::~CPythonValue()
{
	DecRef();
}

EScriptValueType CPythonValue::GetType() const
{
	if (!IsValid())
		return EScriptValueType::Null;

	if (PyObjectPtr == Py_None)
		return EScriptValueType::Null;
	if (PyBool_Check(PyObjectPtr))
		return EScriptValueType::Boolean;
	if (PyLong_Check(PyObjectPtr) || PyFloat_Check(PyObjectPtr))
		return EScriptValueType::Number;
	if (PyUnicode_Check(PyObjectPtr))
		return EScriptValueType::String;
	if (PyList_Check(PyObjectPtr) || PyTuple_Check(PyObjectPtr))
		return EScriptValueType::Array;
	if (PyCallable_Check(PyObjectPtr))
		return EScriptValueType::Function;
	if (PyDict_Check(PyObjectPtr) || PyObject_HasAttrString(PyObjectPtr, "__dict__"))
		return EScriptValueType::Object;

	return EScriptValueType::UserData;
}

bool CPythonValue::IsNull() const
{
	return !IsValid() || PyObjectPtr == Py_None;
}

bool CPythonValue::IsBoolean() const
{
	return IsValid() && PyBool_Check(PyObjectPtr);
}

bool CPythonValue::IsNumber() const
{
	return IsValid() && (PyLong_Check(PyObjectPtr) || PyFloat_Check(PyObjectPtr));
}

bool CPythonValue::IsString() const
{
	return IsValid() && PyUnicode_Check(PyObjectPtr);
}

bool CPythonValue::IsArray() const
{
	return IsValid() && (PyList_Check(PyObjectPtr) || PyTuple_Check(PyObjectPtr));
}

bool CPythonValue::IsObject() const
{
	return IsValid() && (PyDict_Check(PyObjectPtr) || PyObject_HasAttrString(PyObjectPtr, "__dict__"));
}

bool CPythonValue::IsFunction() const
{
	return IsValid() && PyCallable_Check(PyObjectPtr);
}

bool CPythonValue::IsUserData() const
{
	return IsValid() && !IsNull() && !IsBoolean() && !IsNumber() && !IsString() && !IsArray() && !IsObject() &&
	       !IsFunction();
}

bool CPythonValue::ToBool() const
{
	if (!IsValid())
		return false;

	int result = PyObject_IsTrue(PyObjectPtr);
	return result == 1;
}

int32_t CPythonValue::ToInt32() const
{
	if (!IsValid())
		return 0;

	if (PyLong_Check(PyObjectPtr))
	{
		long value = PyLong_AsLong(PyObjectPtr);
		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return 0;
		}
		return static_cast<int32_t>(value);
	}

	if (PyFloat_Check(PyObjectPtr))
	{
		double value = PyFloat_AsDouble(PyObjectPtr);
		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return 0;
		}
		return static_cast<int32_t>(value);
	}

	return 0;
}

int64_t CPythonValue::ToInt64() const
{
	if (!IsValid())
		return 0;

	if (PyLong_Check(PyObjectPtr))
	{
		long long value = PyLong_AsLongLong(PyObjectPtr);
		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return 0;
		}
		return static_cast<int64_t>(value);
	}

	return static_cast<int64_t>(ToInt32());
}

float CPythonValue::ToFloat() const
{
	return static_cast<float>(ToDouble());
}

double CPythonValue::ToDouble() const
{
	if (!IsValid())
		return 0.0;

	if (PyFloat_Check(PyObjectPtr))
	{
		double value = PyFloat_AsDouble(PyObjectPtr);
		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return 0.0;
		}
		return value;
	}

	if (PyLong_Check(PyObjectPtr))
	{
		double value = PyLong_AsDouble(PyObjectPtr);
		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return 0.0;
		}
		return value;
	}

	return 0.0;
}

TString CPythonValue::ToString() const
{
	if (!IsValid())
		return TString();

	PyObject* str_obj = PyObject_Str(PyObjectPtr);
	if (!str_obj)
	{
		PyErr_Clear();
		return TString();
	}

	const char* utf8_str = PyUnicode_AsUTF8(str_obj);
	TString result;
	if (utf8_str)
	{
		result = TString(utf8_str);
	}

	Py_DECREF(str_obj);
	return result;
}

int32_t CPythonValue::GetArrayLength() const
{
	if (!IsArray())
		return 0;

	Py_ssize_t size = 0;
	if (PyList_Check(PyObjectPtr))
	{
		size = PyList_Size(PyObjectPtr);
	}
	else if (PyTuple_Check(PyObjectPtr))
	{
		size = PyTuple_Size(PyObjectPtr);
	}

	return static_cast<int32_t>(size);
}

CScriptValue CPythonValue::GetArrayElement(int32_t Index) const
{
	if (!IsArray())
		return CScriptValue();

	PyObject* item = nullptr;
	if (PyList_Check(PyObjectPtr))
	{
		item = PyList_GetItem(PyObjectPtr, Index); // 借用引用
	}
	else if (PyTuple_Check(PyObjectPtr))
	{
		item = PyTuple_GetItem(PyObjectPtr, Index); // 借用引用
	}

	if (item)
	{
		Py_INCREF(item); // 增加引用计数
		return CPythonValue(item);
	}

	return CScriptValue();
}

void CPythonValue::SetArrayElement(int32_t Index, const CScriptValue& Value)
{
	if (!IsArray() || !PyList_Check(PyObjectPtr))
		return;

	PyObject* py_value = CPythonTypeConverter::ToPyObject(Value);
	if (py_value)
	{
		PyList_SetItem(PyObjectPtr, Index, py_value); // 窃取引用
	}
}

TArray<TString, CMemoryManager> CPythonValue::GetObjectKeys() const
{
	TArray<TString, CMemoryManager> Keys;
	if (!IsObject())
		return Keys;

	PyObject* keys = nullptr;
	if (PyDict_Check(PyObjectPtr))
	{
		keys = PyDict_Keys(PyObjectPtr);
	}
	else
	{
		// 对于其他对象，获取其属性
		keys = PyObject_Dir(PyObjectPtr);
	}

	if (keys && PyList_Check(keys))
	{
		Py_ssize_t size = PyList_Size(keys);
		for (Py_ssize_t i = 0; i < size; ++i)
		{
			PyObject* key = PyList_GetItem(keys, i);
			if (PyUnicode_Check(key))
			{
				const char* key_str = PyUnicode_AsUTF8(key);
				if (key_str)
				{
					Keys.Add(TString(key_str));
				}
			}
		}
	}

	Py_XDECREF(keys);
	return Keys;
}

CScriptValue CPythonValue::GetObjectProperty(const TString& Key) const
{
	if (!IsObject())
		return CScriptValue();

	PyObject* value = nullptr;
	if (PyDict_Check(PyObjectPtr))
	{
		value = PyDict_GetItemString(PyObjectPtr, Key.GetData()); // 借用引用
		if (value)
		{
			Py_INCREF(value);
		}
	}
	else
	{
		value = PyObject_GetAttrString(PyObjectPtr, Key.GetData()); // 新引用
	}

	if (value)
	{
		return CPythonValue(value);
	}

	PyErr_Clear(); // 清除可能的AttributeError
	return CScriptValue();
}

void CPythonValue::SetObjectProperty(const TString& Key, const CScriptValue& Value)
{
	if (!IsObject())
		return;

	PyObject* py_value = CPythonTypeConverter::ToPyObject(Value);
	if (!py_value)
		return;

	int result = -1;
	if (PyDict_Check(PyObjectPtr))
	{
		result = PyDict_SetItemString(PyObjectPtr, Key.GetData(), py_value);
	}
	else
	{
		result = PyObject_SetAttrString(PyObjectPtr, Key.GetData(), py_value);
	}

	Py_DECREF(py_value);

	if (result != 0)
	{
		PyErr_Clear();
	}
}

bool CPythonValue::HasObjectProperty(const TString& Key) const
{
	if (!IsObject())
		return false;

	if (PyDict_Check(PyObjectPtr))
	{
		return PyDict_GetItemString(PyObjectPtr, Key.GetData()) != nullptr;
	}
	else
	{
		int result = PyObject_HasAttrString(PyObjectPtr, Key.GetData());
		return result == 1;
	}
}

SScriptExecutionResult CPythonValue::CallFunction(const TArray<CScriptValue, CMemoryManager>& Args)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!IsFunction())
	{
		Result.ErrorMessage = TEXT("Value is not callable");
		return Result;
	}

	// 创建参数元组
	PyObject* args_tuple = PyTuple_New(Args.Size());
	if (!args_tuple)
	{
		Result.ErrorMessage = TEXT("Failed to create arguments tuple");
		return Result;
	}

	for (int32_t i = 0; i < Args.Size(); ++i)
	{
		PyObject* arg = CPythonTypeConverter::ToPyObject(Args[i]);
		if (!arg)
		{
			Py_DECREF(args_tuple);
			Result.ErrorMessage = TEXT("Failed to convert argument");
			return Result;
		}
		PyTuple_SetItem(args_tuple, i, arg); // 窃取引用
	}

	// 调用函数
	PyObject* call_result = PyObject_CallObject(PyObjectPtr, args_tuple);
	Py_DECREF(args_tuple);

	if (call_result)
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CPythonValue(call_result);
	}
	else
	{
		// 获取Python异常信息
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		if (pvalue)
		{
			PyObject* str_exc = PyObject_Str(pvalue);
			if (str_exc)
			{
				const char* exc_str = PyUnicode_AsUTF8(str_exc);
				if (exc_str)
				{
					Result.ErrorMessage = TString(exc_str);
				}
				Py_DECREF(str_exc);
			}
		}

		Py_XDECREF(ptype);
		Py_XDECREF(pvalue);
		Py_XDECREF(ptraceback);

		if (Result.ErrorMessage.IsEmpty())
		{
			Result.ErrorMessage = TEXT("Function call failed");
		}
	}

	return Result;
}

CConfigValue CPythonValue::ToConfigValue() const
{
	return CPythonTypeConverter::PythonToConfigValue(PyObjectPtr);
}

void CPythonValue::FromConfigValue(const CConfigValue& Config)
{
	DecRef();
	PyObjectPtr = CPythonTypeConverter::ConfigValueToPython(Config);
	IncRef();
}

bool CPythonValue::IsValid() const
{
	return PyObjectPtr != nullptr;
}

CPythonValue CPythonValue::CallMethod(const TString& MethodName, const TArray<CPythonValue, CMemoryManager>& Args)
{
	if (!IsValid())
		return CPythonValue();

	PyObject* method = PyObject_GetAttrString(PyObjectPtr, MethodName.GetData());
	if (!method || !PyCallable_Check(method))
	{
		Py_XDECREF(method);
		PyErr_Clear();
		return CPythonValue();
	}

	// 创建参数元组
	PyObject* args_tuple = PyTuple_New(Args.Size());
	for (int32_t i = 0; i < Args.Size(); ++i)
	{
		PyObject* arg = Args[i].GetPyObject();
		Py_INCREF(arg);
		PyTuple_SetItem(args_tuple, i, arg);
	}

	// 调用方法
	PyObject* result = PyObject_CallObject(method, args_tuple);

	Py_DECREF(method);
	Py_DECREF(args_tuple);

	if (result)
	{
		return CPythonValue(result);
	}

	PyErr_Clear();
	return CPythonValue();
}

CPythonValue CPythonValue::GetAttribute(const TString& AttributeName)
{
	if (!IsValid())
		return CPythonValue();

	PyObject* attr = PyObject_GetAttrString(PyObjectPtr, AttributeName.GetData());
	if (attr)
	{
		return CPythonValue(attr);
	}

	PyErr_Clear();
	return CPythonValue();
}

void CPythonValue::SetAttribute(const TString& AttributeName, const CPythonValue& Value)
{
	if (!IsValid() || !Value.IsValid())
		return;

	int result = PyObject_SetAttrString(PyObjectPtr, AttributeName.GetData(), Value.GetPyObject());
	if (result != 0)
	{
		PyErr_Clear();
	}
}

bool CPythonValue::HasAttribute(const TString& AttributeName) const
{
	if (!IsValid())
		return false;

	return PyObject_HasAttrString(PyObjectPtr, AttributeName.GetData()) == 1;
}

void CPythonValue::IncRef()
{
	if (PyObjectPtr)
	{
		Py_INCREF(PyObjectPtr);
	}
}

void CPythonValue::DecRef()
{
	if (PyObjectPtr)
	{
		Py_DECREF(PyObjectPtr);
		PyObjectPtr = nullptr;
	}
}

void CPythonValue::CopyFrom(const CPythonValue& Other)
{
	PyObjectPtr = Other.PyObjectPtr;
	CachedType = Other.CachedType;
	IncRef();
}

// === CPythonModule 实现 ===

CPythonModule::CPythonModule(const TString& InName)
    : ModuleName(InName),
      bLoaded(false),
      ModuleObject(nullptr),
      ModuleDict(nullptr)
{}

CPythonModule::~CPythonModule()
{
	Unload();
}

SScriptExecutionResult CPythonModule::Load(const TString& ModulePath)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	// TODO: 读取并执行Python文件
	// 可以使用PyRun_SimpleFile或编译后执行

	bLoaded = true;
	Result.Result = EScriptResult::Success;
	NLOG_SCRIPT(Info, "Python module '{}' loaded from: {}", ModuleName.GetData(), ModulePath.GetData());

	return Result;
}

SScriptExecutionResult CPythonModule::Unload()
{
	GlobalObjects.Clear();

	Py_XDECREF(ModuleDict);
	ModuleDict = nullptr;

	Py_XDECREF(ModuleObject);
	ModuleObject = nullptr;

	bLoaded = false;

	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Success;
	return Result;
}

CScriptValue CPythonModule::GetGlobal(const TString& Name) const
{
	if (GlobalObjects.Contains(Name))
		return GlobalObjects[Name];

	if (ModuleDict)
	{
		PyObject* value = PyDict_GetItemString(ModuleDict, Name.GetData());
		if (value)
		{
			Py_INCREF(value);
			return CPythonValue(value);
		}
	}

	return CScriptValue();
}

void CPythonModule::SetGlobal(const TString& Name, const CScriptValue& Value)
{
	if (const CPythonValue* PyValue = dynamic_cast<const CPythonValue*>(&Value))
	{
		GlobalObjects.Add(Name, *PyValue);
	}
	else
	{
		CPythonValue ConvertedValue = CPythonTypeConverter::ToPythonValue(Value);
		GlobalObjects.Add(Name, ConvertedValue);
	}

	// 同时设置到模块字典
	if (ModuleDict)
	{
		PyObject* py_value = CPythonTypeConverter::ToPyObject(Value);
		if (py_value)
		{
			PyDict_SetItemString(ModuleDict, Name.GetData(), py_value);
			Py_DECREF(py_value);
		}
	}
}

SScriptExecutionResult CPythonModule::ExecuteString(const TString& Code)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!bLoaded)
	{
		Result.ErrorMessage = TEXT("Module not loaded");
		return Result;
	}

	// 编译代码
	PyObject* compiled_code = CompilePythonCode(Code);
	if (!compiled_code)
	{
		return HandlePythonError(TEXT("Code compilation"));
	}

	// 准备全局和局部命名空间
	PyObject* globals = ModuleDict ? ModuleDict : PyEval_GetGlobals();
	PyObject* locals = ModuleDict ? ModuleDict : PyEval_GetLocals();

	// 执行代码
	PyObject* exec_result = PyEval_EvalCode(compiled_code, globals, locals);
	Py_DECREF(compiled_code);

	if (exec_result)
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CPythonValue(exec_result);
	}
	else
	{
		Result = HandlePythonError(TEXT("Code execution"));
	}

	return Result;
}

SScriptExecutionResult CPythonModule::ExecuteFile(const TString& FilePath)
{
	// TODO: 读取文件内容并执行
	return ExecuteString(TEXT("# File execution not implemented"));
}

void CPythonModule::RegisterFunction(const TString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 创建Python函数包装器
	NLOG_SCRIPT(Warning, "RegisterFunction not implemented for Python module");
}

void CPythonModule::RegisterObject(const TString& Name, const CScriptValue& Object)
{
	SetGlobal(Name, Object);
}

bool CPythonModule::ImportModule(const TString& ModuleName)
{
	PyObject* imported_module = PyImport_ImportModule(ModuleName.GetData());
	if (imported_module)
	{
		// 将模块添加到当前命名空间
		if (ModuleDict)
		{
			PyDict_SetItemString(ModuleDict, ModuleName.GetData(), imported_module);
		}
		Py_DECREF(imported_module);
		return true;
	}

	PyErr_Clear();
	return false;
}

void CPythonModule::AddToSysPath(const TString& Path)
{
	PyObject* sys_path = PySys_GetObject("path");
	if (sys_path && PyList_Check(sys_path))
	{
		PyObject* path_str = PyUnicode_FromString(Path.GetData());
		if (path_str)
		{
			PyList_Append(sys_path, path_str);
			Py_DECREF(path_str);
		}
	}
}

PyObject* CPythonModule::CompilePythonCode(const TString& Code, const TString& Filename)
{
	return Py_CompileString(Code.GetData(), Filename.GetData(), Py_file_input);
}

SScriptExecutionResult CPythonModule::HandlePythonError(const TString& Operation)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (PyErr_Occurred())
	{
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		if (pvalue)
		{
			PyObject* str_exc = PyObject_Str(pvalue);
			if (str_exc)
			{
				const char* exc_str = PyUnicode_AsUTF8(str_exc);
				if (exc_str)
				{
					Result.ErrorMessage = TString::Format(TEXT("Python Error in {}: {}"), Operation.GetData(), exc_str);
				}
				Py_DECREF(str_exc);
			}
		}

		Py_XDECREF(ptype);
		Py_XDECREF(pvalue);
		Py_XDECREF(ptraceback);
	}

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = TString::Format(TEXT("Python Error in operation: {}"), Operation.GetData());
	}

	NLOG_SCRIPT(Error, "Python Module Error: {}", Result.ErrorMessage.GetData());
	return Result;
}

void CPythonModule::SetupModuleEnvironment()
{
	// 创建模块对象和字典
	ModuleObject = PyModule_New(ModuleName.GetData());
	if (ModuleObject)
	{
		ModuleDict = PyModule_GetDict(ModuleObject); // 借用引用
	}
}

// === CPythonContext 实现 ===

CPythonContext::CPythonContext()
    : bInitialized(false),
      MainThreadState(nullptr),
      GlobalNamespace(nullptr),
      LocalNamespace(nullptr),
      StartTime(0),
      bTimeoutEnabled(false)
{}

CPythonContext::~CPythonContext()
{
	Shutdown();
}

bool CPythonContext::Initialize(const SScriptConfig& InConfig)
{
	Config = InConfig;

	if (!InitializePython())
	{
		NLOG_SCRIPT(Error, "Failed to initialize Python for context");
		return false;
	}

	SetupBuiltinModules();
	RegisterNLibAPI();

	StartTime = GetCurrentTimeMilliseconds();
	bTimeoutEnabled = Config.TimeoutMilliseconds > 0;

	bInitialized = true;
	NLOG_SCRIPT(Info, "Python context initialized successfully");
	return true;
}

void CPythonContext::Shutdown()
{
	if (!bInitialized)
		return;

	Modules.Clear();

	Py_XDECREF(LocalNamespace);
	LocalNamespace = nullptr;

	Py_XDECREF(GlobalNamespace);
	GlobalNamespace = nullptr;

	ShutdownPython();

	bInitialized = false;
	NLOG_SCRIPT(Info, "Python context shut down");
}

TSharedPtr<CScriptModule> CPythonContext::CreateModule(const TString& Name)
{
	if (!bInitialized)
	{
		NLOG_SCRIPT(Error, "Python context not initialized");
		return nullptr;
	}

	if (Modules.Contains(Name))
	{
		NLOG_SCRIPT(Warning, "Module '{}' already exists", Name.GetData());
		return Modules[Name];
	}

	auto Module = MakeShared<CPythonModule>(Name);
	Module->SetupModuleEnvironment();
	Modules.Add(Name, Module);

	return Module;
}

TSharedPtr<CScriptModule> CPythonContext::GetModule(const TString& Name) const
{
	if (Modules.Contains(Name))
		return Modules[Name];

	return nullptr;
}

void CPythonContext::DestroyModule(const TString& Name)
{
	if (Modules.Contains(Name))
	{
		Modules[Name]->Unload();
		Modules.Remove(Name);
	}
}

SScriptExecutionResult CPythonContext::ExecuteString(const TString& Code, const TString& ModuleName)
{
	if (ModuleName.IsEmpty() || ModuleName == TEXT("__main__"))
	{
		return ExecutePython(Code);
	}

	auto Module = GetModule(ModuleName);
	if (!Module)
	{
		Module = CreateModule(ModuleName);
	}

	return Module->ExecuteString(Code);
}

SScriptExecutionResult CPythonContext::ExecuteFile(const TString& FilePath, const TString& ModuleName)
{
	// TODO: 读取文件并执行
	return ExecuteString(TEXT("# File execution not implemented"), ModuleName);
}

void CPythonContext::CollectGarbage()
{
	if (bInitialized)
	{
		// 调用Python垃圾回收
		PyObject* gc_module = PyImport_ImportModule("gc");
		if (gc_module)
		{
			PyObject* collect_func = PyObject_GetAttrString(gc_module, "collect");
			if (collect_func && PyCallable_Check(collect_func))
			{
				PyObject_CallObject(collect_func, nullptr);
			}
			Py_XDECREF(collect_func);
			Py_DECREF(gc_module);
		}
		PyErr_Clear();
	}
}

uint64_t CPythonContext::GetMemoryUsage() const
{
	// TODO: 获取Python内存使用量
	// 可能需要通过sys模块或其他方式获取
	return 0;
}

void CPythonContext::ResetTimeout()
{
	StartTime = GetCurrentTimeMilliseconds();
}

void CPythonContext::RegisterGlobalFunction(const TString& Name, TSharedPtr<CScriptFunction> Function)
{
	// TODO: 创建Python函数包装器并注册到全局命名空间
	NLOG_SCRIPT(Warning, "RegisterGlobalFunction not implemented for Python context");
}

void CPythonContext::RegisterGlobalObject(const TString& Name, const CScriptValue& Object)
{
	if (!GlobalNamespace)
		return;

	PyObject* py_object = CPythonTypeConverter::ToPyObject(Object);
	if (py_object)
	{
		PyDict_SetItemString(GlobalNamespace, Name.GetData(), py_object);
		Py_DECREF(py_object);
	}
}

void CPythonContext::RegisterGlobalConstant(const TString& Name, const CScriptValue& Value)
{
	RegisterGlobalObject(Name, Value);
}

void CPythonContext::SetPythonPath(const TArray<TString, CMemoryManager>& Paths)
{
	PythonPaths = Paths;

	PyObject* sys_path = PySys_GetObject("path");
	if (sys_path && PyList_Check(sys_path))
	{
		// 清除现有路径（可选）
		// PyList_SetSlice(sys_path, 0, PyList_Size(sys_path), nullptr);

		// 添加新路径
		for (const auto& Path : Paths)
		{
			PyObject* path_str = PyUnicode_FromString(Path.GetData());
			if (path_str)
			{
				PyList_Append(sys_path, path_str);
				Py_DECREF(path_str);
			}
		}
	}
}

SScriptExecutionResult CPythonContext::ExecutePython(const TString& PythonCode, PyObject* LocalNamespace)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (!bInitialized)
	{
		Result.ErrorMessage = TEXT("Python context not initialized");
		return Result;
	}

	PyObject* globals = GlobalNamespace ? GlobalNamespace : PyEval_GetGlobals();
	PyObject* locals = LocalNamespace ? LocalNamespace : (LocalNamespace ? LocalNamespace : globals);

	// 执行代码
	PyObject* exec_result = PyRun_String(PythonCode.GetData(), Py_file_input, globals, locals);

	if (exec_result)
	{
		Result.Result = EScriptResult::Success;
		Result.ReturnValue = CPythonValue(exec_result);
	}
	else
	{
		Result = HandlePythonError(TEXT("Code execution"));
	}

	return Result;
}

bool CPythonContext::ImportModule(const TString& ModuleName, const TString& Alias)
{
	PyObject* imported_module = PyImport_ImportModule(ModuleName.GetData());
	if (imported_module)
	{
		const TString& name = Alias.IsEmpty() ? ModuleName : Alias;
		if (GlobalNamespace)
		{
			PyDict_SetItemString(GlobalNamespace, name.GetData(), imported_module);
		}
		Py_DECREF(imported_module);
		return true;
	}

	PyErr_Clear();
	return false;
}

void CPythonContext::SetInterpreterFlags(int Flags)
{
	// TODO: 设置Python解释器标志
}

bool CPythonContext::InitializePython()
{
	// 获取线程状态（假设Python解释器已初始化）
	MainThreadState = PyThreadState_Get();
	if (!MainThreadState)
	{
		NLOG_SCRIPT(Error, "Failed to get Python thread state");
		return false;
	}

	// 创建全局和局部命名空间
	GlobalNamespace = PyDict_New();
	LocalNamespace = PyDict_New();

	if (!GlobalNamespace || !LocalNamespace)
	{
		NLOG_SCRIPT(Error, "Failed to create Python namespaces");
		return false;
	}

	// 添加内置模块到全局命名空间
	PyObject* builtins = PyEval_GetBuiltins();
	if (builtins)
	{
		PyDict_SetItemString(GlobalNamespace, "__builtins__", builtins);
	}

	return true;
}

void CPythonContext::ShutdownPython()
{
	// 不需要显式关闭Python解释器，由引擎管理
	MainThreadState = nullptr;
}

SScriptExecutionResult CPythonContext::HandlePythonError(const TString& Operation)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;

	if (PyErr_Occurred())
	{
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		if (pvalue)
		{
			PyObject* str_exc = PyObject_Str(pvalue);
			if (str_exc)
			{
				const char* exc_str = PyUnicode_AsUTF8(str_exc);
				if (exc_str)
				{
					Result.ErrorMessage = TString::Format(TEXT("Python Error in {}: {}"), Operation.GetData(), exc_str);
				}
				Py_DECREF(str_exc);
			}
		}

		Py_XDECREF(ptype);
		Py_XDECREF(pvalue);
		Py_XDECREF(ptraceback);
	}

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = TString::Format(TEXT("Python Error in operation: {}"), Operation.GetData());
	}

	NLOG_SCRIPT(Error, "Python Context Error: {}", Result.ErrorMessage.GetData());
	return Result;
}

void CPythonContext::RegisterNLibAPI()
{
	if (!GlobalNamespace)
		return;

	// 创建NLib模块对象
	PyObject* nlib_module = PyDict_New();
	if (nlib_module)
	{
		// TODO: 添加NLib API函数和对象

		PyDict_SetItemString(GlobalNamespace, "NLib", nlib_module);
		Py_DECREF(nlib_module);
	}
}

void CPythonContext::SetupBuiltinModules()
{
	// 导入常用的Python标准库模块
	ImportModule("sys");
	ImportModule("os");
	ImportModule("math");
	ImportModule("json");
	ImportModule("time");
	ImportModule("datetime");
}

void CPythonContext::PythonErrorHandler(const char* msg)
{
	NLOG_SCRIPT(Error, "Python Runtime Error: {}", msg ? msg : "Unknown error");
}

// === CPythonEngine 实现 ===

CPythonEngine::CPythonEngine()
    : bInitialized(false)
{}

CPythonEngine::~CPythonEngine()
{
	Shutdown();
}

TString CPythonEngine::GetVersion() const
{
	return GetPythonVersionString();
}

bool CPythonEngine::IsSupported() const
{
	return IsPythonAvailable();
}

TSharedPtr<CScriptContext> CPythonEngine::CreateContext(const SScriptConfig& Config)
{
	if (!bInitialized)
	{
		NLOG_SCRIPT(Error, "Python engine not initialized");
		return nullptr;
	}

	auto Context = MakeShared<CPythonContext>();
	if (!Context->Initialize(Config))
	{
		NLOG_SCRIPT(Error, "Failed to initialize Python context");
		return nullptr;
	}

	ActiveContexts.Add(Context);
	return Context;
}

void CPythonEngine::DestroyContext(TSharedPtr<CScriptContext> Context)
{
	if (Context)
	{
		Context->Shutdown();
		ActiveContexts.RemoveSwap(Context);
	}
}

bool CPythonEngine::Initialize()
{
	if (bInitialized)
		return true;

	if (!InitializePythonInterpreter())
	{
		NLOG_SCRIPT(Error, "Failed to initialize Python interpreter");
		return false;
	}

	if (!LoadPythonLibrary())
	{
		NLOG_SCRIPT(Error, "Failed to load Python library");
		return false;
	}

	RegisterStandardLibraries();

	bInitialized = true;
	NLOG_SCRIPT(Info, "Python engine initialized successfully");
	return true;
}

void CPythonEngine::Shutdown()
{
	if (!bInitialized)
		return;

	// 清理所有活跃的上下文
	for (auto& Context : ActiveContexts)
	{
		Context->Shutdown();
	}
	ActiveContexts.Clear();

	ShutdownPythonInterpreter();

	bInitialized = false;
	NLOG_SCRIPT(Info, "Python engine shut down");
}

CScriptValue CPythonEngine::CreateValue()
{
	return CPythonValue();
}

CScriptValue CPythonEngine::CreateNull()
{
	return CPythonValue(Py_None);
}

CScriptValue CPythonEngine::CreateBool(bool Value)
{
	PyObject* py_bool = Value ? Py_True : Py_False;
	Py_INCREF(py_bool);
	return CPythonValue(py_bool);
}

CScriptValue CPythonEngine::CreateInt(int32_t Value)
{
	return CPythonValue(PyLong_FromLong(Value));
}

CScriptValue CPythonEngine::CreateFloat(float Value)
{
	return CPythonValue(PyFloat_FromDouble(Value));
}

CScriptValue CPythonEngine::CreateString(const TString& Value)
{
	return CPythonValue(PyUnicode_FromString(Value.GetData()));
}

CScriptValue CPythonEngine::CreateArray()
{
	return CPythonValue(PyList_New(0));
}

CScriptValue CPythonEngine::CreateObject()
{
	return CPythonValue(PyDict_New());
}

SScriptExecutionResult CPythonEngine::CheckSyntax(const TString& Code)
{
	SScriptExecutionResult Result;

	PyObject* compiled = Py_CompileString(Code.GetData(), "<syntax_check>", Py_file_input);
	if (compiled)
	{
		Result.Result = EScriptResult::Success;
		Py_DECREF(compiled);
	}
	else
	{
		Result.Result = EScriptResult::CompileError;

		// 获取语法错误信息
		if (PyErr_Occurred())
		{
			PyObject *ptype, *pvalue, *ptraceback;
			PyErr_Fetch(&ptype, &pvalue, &ptraceback);

			if (pvalue)
			{
				PyObject* str_exc = PyObject_Str(pvalue);
				if (str_exc)
				{
					const char* exc_str = PyUnicode_AsUTF8(str_exc);
					if (exc_str)
					{
						Result.ErrorMessage = TString(exc_str);
					}
					Py_DECREF(str_exc);
				}
			}

			Py_XDECREF(ptype);
			Py_XDECREF(pvalue);
			Py_XDECREF(ptraceback);
		}
	}

	return Result;
}

SScriptExecutionResult CPythonEngine::CompileFile(const TString& FilePath, const TString& OutputPath)
{
	return CompilePythonFile(FilePath, OutputPath);
}

TString CPythonEngine::GetPythonVersionString()
{
	const char* version = Py_GetVersion();
	return TString(version ? version : "Unknown");
}

bool CPythonEngine::IsPythonAvailable()
{
	return Py_IsInitialized() != 0;
}

bool CPythonEngine::InitializePythonInterpreter()
{
	if (bPythonInterpreterInitialized)
		return true;

	if (Py_IsInitialized())
	{
		bPythonInterpreterInitialized = true;
		return true;
	}

	// 初始化Python解释器
	Py_Initialize();

	if (!Py_IsInitialized())
	{
		NLOG_SCRIPT(Error, "Failed to initialize Python interpreter");
		return false;
	}

	// 初始化线程支持
	if (!PyEval_ThreadsInitialized())
	{
		PyEval_InitThreads();
	}

	bPythonInterpreterInitialized = true;
	NLOG_SCRIPT(Info, "Python interpreter initialized");
	return true;
}

void CPythonEngine::ShutdownPythonInterpreter()
{
	if (!bPythonInterpreterInitialized)
		return;

	if (Py_IsInitialized())
	{
		Py_Finalize();
	}

	bPythonInterpreterInitialized = false;
	NLOG_SCRIPT(Info, "Python interpreter shut down");
}

SScriptExecutionResult CPythonEngine::CompilePythonFile(const TString& InputPath, const TString& OutputPath)
{
	SScriptExecutionResult Result;
	Result.Result = EScriptResult::Error;
	Result.ErrorMessage = TEXT("Python file compilation not implemented");

	// TODO: 实现Python文件编译到字节码

	return Result;
}

void CPythonEngine::SetInterpreterOptions(const THashMap<TString, TString, CMemoryManager>& Options)
{
	InterpreterOptions = Options;
}

bool CPythonEngine::InstallPythonPackage(const TString& PackageName, const TString& Version)
{
	// TODO: 通过pip安装Python包
	NLOG_SCRIPT(Warning, "InstallPythonPackage not implemented");
	return false;
}

void CPythonEngine::RegisterStandardLibraries()
{
	// TODO: 注册标准Python库
}

bool CPythonEngine::LoadPythonLibrary()
{
	// Python库通常在初始化时已加载
	return true;
}

// === CPythonTypeConverter 实现 ===

template <>
PyObject* CPythonTypeConverter::ToPyObject<bool>(const bool& Value)
{
	PyObject* py_bool = Value ? Py_True : Py_False;
	Py_INCREF(py_bool);
	return py_bool;
}

template <>
PyObject* CPythonTypeConverter::ToPyObject<int32_t>(const int32_t& Value)
{
	return PyLong_FromLong(Value);
}

template <>
PyObject* CPythonTypeConverter::ToPyObject<float>(const float& Value)
{
	return PyFloat_FromDouble(Value);
}

template <>
PyObject* CPythonTypeConverter::ToPyObject<double>(const double& Value)
{
	return PyFloat_FromDouble(Value);
}

template <>
PyObject* CPythonTypeConverter::ToPyObject<TString>(const TString& Value)
{
	return PyUnicode_FromString(Value.GetData());
}

template <>
bool CPythonTypeConverter::FromPyObject<bool>(PyObject* PyObj)
{
	if (!PyObj)
		return false;
	return PyObject_IsTrue(PyObj) == 1;
}

template <>
int32_t CPythonTypeConverter::FromPyObject<int32_t>(PyObject* PyObj)
{
	if (!PyObj || !PyLong_Check(PyObj))
		return 0;
	long value = PyLong_AsLong(PyObj);
	return PyErr_Occurred() ? 0 : static_cast<int32_t>(value);
}

template <>
float CPythonTypeConverter::FromPyObject<float>(PyObject* PyObj)
{
	if (!PyObj)
		return 0.0f;
	double value = PyFloat_AsDouble(PyObj);
	return PyErr_Occurred() ? 0.0f : static_cast<float>(value);
}

template <>
double CPythonTypeConverter::FromPyObject<double>(PyObject* PyObj)
{
	if (!PyObj)
		return 0.0;
	double value = PyFloat_AsDouble(PyObj);
	return PyErr_Occurred() ? 0.0 : value;
}

template <>
TString CPythonTypeConverter::FromPyObject<TString>(PyObject* PyObj)
{
	if (!PyObj)
		return TString();

	PyObject* str_obj = PyObject_Str(PyObj);
	if (!str_obj)
		return TString();

	const char* utf8_str = PyUnicode_AsUTF8(str_obj);
	TString result = utf8_str ? TString(utf8_str) : TString();

	Py_DECREF(str_obj);
	return result;
}

PyObject* CPythonTypeConverter::ToPyObject(const CScriptValue& ScriptValue)
{
	// 如果已经是CPythonValue，直接返回
	if (const CPythonValue* PyValue = dynamic_cast<const CPythonValue*>(&ScriptValue))
	{
		PyObject* obj = PyValue->GetPyObject();
		Py_XINCREF(obj);
		return obj;
	}

	// 根据类型转换
	switch (ScriptValue.GetType())
	{
	case EScriptValueType::Null:
		Py_INCREF(Py_None);
		return Py_None;
	case EScriptValueType::Boolean:
		return ToPyObject(ScriptValue.ToBool());
	case EScriptValueType::Number:
		return ToPyObject(ScriptValue.ToDouble());
	case EScriptValueType::String:
		return ToPyObject(ScriptValue.ToString());
	case EScriptValueType::Array: {
		PyObject* list = PyList_New(ScriptValue.GetArrayLength());
		for (int32_t i = 0; i < ScriptValue.GetArrayLength(); ++i)
		{
			PyObject* element = ToPyObject(ScriptValue.GetArrayElement(i));
			PyList_SetItem(list, i, element); // 窃取引用
		}
		return list;
	}
	case EScriptValueType::Object: {
		PyObject* dict = PyDict_New();
		auto keys = ScriptValue.GetObjectKeys();
		for (const auto& key : keys)
		{
			PyObject* value = ToPyObject(ScriptValue.GetObjectProperty(key));
			PyDict_SetItemString(dict, key.GetData(), value);
			Py_DECREF(value);
		}
		return dict;
	}
	default:
		Py_INCREF(Py_None);
		return Py_None;
	}
}

CPythonValue CPythonTypeConverter::ToPythonValue(const CScriptValue& ScriptValue)
{
	if (const CPythonValue* PyValue = dynamic_cast<const CPythonValue*>(&ScriptValue))
	{
		return *PyValue;
	}

	PyObject* py_obj = ToPyObject(ScriptValue);
	return CPythonValue(py_obj);
}

CScriptValue CPythonTypeConverter::FromPythonValue(const CPythonValue& PythonValue)
{
	return PythonValue;
}

PyObject* CPythonTypeConverter::ConfigValueToPython(const CConfigValue& Config)
{
	// TODO: 实现ConfigValue到Python对象的转换
	Py_INCREF(Py_None);
	return Py_None;
}

CConfigValue CPythonTypeConverter::PythonToConfigValue(PyObject* PyObj)
{
	// TODO: 实现Python对象到ConfigValue的转换
	return CConfigValue();
}

TString CPythonTypeConverter::GetPythonTypeName(PyObject* PyObj)
{
	if (!PyObj)
		return TEXT("NoneType");

	PyTypeObject* type = Py_TYPE(PyObj);
	if (type && type->tp_name)
	{
		return TString(type->tp_name);
	}

	return TEXT("Unknown");
}

PyObject* CPythonTypeConverter::CreatePythonList(const TArray<PyObject*, CMemoryManager>& Elements)
{
	PyObject* list = PyList_New(Elements.Size());
	for (int32_t i = 0; i < Elements.Size(); ++i)
	{
		Py_INCREF(Elements[i]);
		PyList_SetItem(list, i, Elements[i]);
	}
	return list;
}

PyObject* CPythonTypeConverter::CreatePythonDict(const THashMap<TString, PyObject*, CMemoryManager>& Elements)
{
	PyObject* dict = PyDict_New();
	for (const auto& pair : Elements)
	{
		Py_INCREF(pair.Value);
		PyDict_SetItemString(dict, pair.Key.GetData(), pair.Value);
	}
	return dict;
}

TArray<PyObject*, CMemoryManager> CPythonTypeConverter::GetPythonListElements(PyObject* PyList)
{
	TArray<PyObject*, CMemoryManager> Elements;

	if (PyList_Check(PyList))
	{
		Py_ssize_t size = PyList_Size(PyList);
		for (Py_ssize_t i = 0; i < size; ++i)
		{
			PyObject* item = PyList_GetItem(PyList, i); // 借用引用
			Py_INCREF(item);
			Elements.Add(item);
		}
	}

	return Elements;
}

THashMap<TString, PyObject*, CMemoryManager> CPythonTypeConverter::GetPythonDictItems(PyObject* PyDict)
{
	THashMap<TString, PyObject*, CMemoryManager> Items;

	if (PyDict_Check(PyDict))
	{
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(PyDict, &pos, &key, &value))
		{
			if (PyUnicode_Check(key))
			{
				const char* key_str = PyUnicode_AsUTF8(key);
				if (key_str)
				{
					Py_INCREF(value);
					Items.Add(TString(key_str), value);
				}
			}
		}
	}

	return Items;
}

} // namespace NLib