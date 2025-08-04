#pragma once

#include "Containers/THashMap.h"
#include "Core/TString.h"
#include "Memory/Memory.h"
#include "ScriptEngine.h"

#include <memory>

// Python C API前向声明
struct _object;
typedef _object PyObject;
struct _ts;
typedef _ts PyThreadState;

namespace NLib
{
/**
 * @brief Python脚本值包装器
 * 基于CPython实现
 */
class CPythonValue : public CScriptValue
{
	GENERATED_BODY()

public:
	CPythonValue();
	CPythonValue(PyObject* InPyObject);
	CPythonValue(const CPythonValue& Other);
	CPythonValue& operator=(const CPythonValue& Other);
	~CPythonValue() override;

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

	// === Python特有方法 ===

	/**
	 * @brief 获取Python对象指针
	 */
	PyObject* GetPyObject() const
	{
		return PyObjectPtr;
	}

	/**
	 * @brief 检查值是否有效
	 */
	bool IsValid() const;

	/**
	 * @brief 调用Python方法
	 */
	CPythonValue CallMethod(const TString& MethodName, const TArray<CPythonValue, CMemoryManager>& Args = {});

	/**
	 * @brief 获取Python属性
	 */
	CPythonValue GetAttribute(const TString& AttributeName);

	/**
	 * @brief 设置Python属性
	 */
	void SetAttribute(const TString& AttributeName, const CPythonValue& Value);

	/**
	 * @brief 检查是否有指定属性
	 */
	bool HasAttribute(const TString& AttributeName) const;

private:
	void IncRef();
	void DecRef();
	void CopyFrom(const CPythonValue& Other);

private:
	PyObject* PyObjectPtr = nullptr;
	EScriptValueType CachedType = EScriptValueType::Null;
};

/**
 * @brief Python脚本模块
 */
class CPythonModule : public CScriptModule
{
	GENERATED_BODY()

public:
	explicit CPythonModule(const TString& InName);
	~CPythonModule() override;

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
		return EScriptLanguage::Python;
	}

	SScriptExecutionResult Load(const TString& ModulePath) override;
	SScriptExecutionResult Unload() override;
	bool IsLoaded() const override
	{
		return bLoaded && ModuleObject != nullptr;
	}

	CScriptValue GetGlobal(const TString& Name) const override;
	void SetGlobal(const TString& Name, const CScriptValue& Value) override;

	SScriptExecutionResult ExecuteString(const TString& Code) override;
	SScriptExecutionResult ExecuteFile(const TString& FilePath) override;

	void RegisterFunction(const TString& Name, TSharedPtr<CScriptFunction> Function) override;
	void RegisterObject(const TString& Name, const CScriptValue& Object) override;

	// === Python特有方法 ===

	/**
	 * @brief 获取Python模块对象
	 */
	PyObject* GetModuleObject() const
	{
		return ModuleObject;
	}

	/**
	 * @brief 导入Python模块
	 * @param ModuleName Python模块名（如"math", "os"等）
	 * @return 是否导入成功
	 */
	bool ImportModule(const TString& ModuleName);

	/**
	 * @brief 添加到sys.path
	 */
	void AddToSysPath(const TString& Path);

	/**
	 * @brief 编译Python代码为字节码
	 */
	PyObject* CompilePythonCode(const TString& Code, const TString& Filename = TEXT("<string>"));

private:
	SScriptExecutionResult HandlePythonError(const TString& Operation);
	void SetupModuleEnvironment();

private:
	TString ModuleName;
	bool bLoaded = false;
	PyObject* ModuleObject = nullptr;
	PyObject* ModuleDict = nullptr;
	THashMap<TString, CPythonValue, CMemoryManager> GlobalObjects;
};

/**
 * @brief Python脚本上下文
 */
class CPythonContext : public CScriptContext
{
	GENERATED_BODY()

public:
	CPythonContext();
	~CPythonContext() override;

	// === CScriptContext接口实现 ===

	bool Initialize(const SScriptConfig& Config) override;
	void Shutdown() override;
	bool IsInitialized() const override
	{
		return bInitialized && MainThreadState != nullptr;
	}

	SScriptConfig GetConfig() const override
	{
		return Config;
	}
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Python;
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

	// === Python特有方法 ===

	/**
	 * @brief 获取主线程状态
	 */
	PyThreadState* GetMainThreadState() const
	{
		return MainThreadState;
	}

	/**
	 * @brief 获取全局命名空间
	 */
	PyObject* GetGlobalNamespace() const
	{
		return GlobalNamespace;
	}

	/**
	 * @brief 设置Python路径
	 */
	void SetPythonPath(const TArray<TString, CMemoryManager>& Paths);

	/**
	 * @brief 执行Python脚本并获取结果
	 * @param PythonCode Python源代码
	 * @param LocalNamespace 局部命名空间（可选）
	 * @return 执行结果
	 */
	SScriptExecutionResult ExecutePython(const TString& PythonCode, PyObject* LocalNamespace = nullptr);

	/**
	 * @brief 导入Python模块到全局命名空间
	 */
	bool ImportModule(const TString& ModuleName, const TString& Alias = TString());

	/**
	 * @brief 设置Python解释器标志
	 */
	void SetInterpreterFlags(int Flags);

private:
	bool InitializePython();
	void ShutdownPython();
	SScriptExecutionResult HandlePythonError(const TString& Operation);
	void RegisterNLibAPI();
	void SetupBuiltinModules();

	// Python异常处理
	static void PythonErrorHandler(const char* msg);

private:
	bool bInitialized = false;
	PyThreadState* MainThreadState = nullptr;
	PyObject* GlobalNamespace = nullptr;
	PyObject* LocalNamespace = nullptr;
	SScriptConfig Config;
	THashMap<TString, TSharedPtr<CPythonModule>, CMemoryManager> Modules;

	// 状态跟踪
	uint64_t StartTime = 0;
	bool bTimeoutEnabled = false;

	// Python路径和模块管理
	TArray<TString, CMemoryManager> PythonPaths;
};

/**
 * @brief Python脚本引擎
 */
class CPythonEngine : public CScriptEngine
{
	GENERATED_BODY()

public:
	CPythonEngine();
	~CPythonEngine() override;

	// === CScriptEngine接口实现 ===

	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Python;
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

	// === Python特有方法 ===

	/**
	 * @brief 获取Python版本信息
	 */
	static TString GetPythonVersionString();

	/**
	 * @brief 检查Python是否可用
	 */
	static bool IsPythonAvailable();

	/**
	 * @brief 初始化全局Python解释器
	 */
	static bool InitializePythonInterpreter();

	/**
	 * @brief 清理全局Python解释器
	 */
	static void ShutdownPythonInterpreter();

	/**
	 * @brief 编译Python文件到字节码
	 */
	SScriptExecutionResult CompilePythonFile(const TString& InputPath, const TString& OutputPath);

	/**
	 * @brief 设置Python解释器选项
	 */
	void SetInterpreterOptions(const THashMap<TString, TString, CMemoryManager>& Options);

	/**
	 * @brief 安装Python包（通过pip）
	 */
	bool InstallPythonPackage(const TString& PackageName, const TString& Version = TString());

private:
	void RegisterStandardLibraries();
	static bool LoadPythonLibrary();

private:
	bool bInitialized = false;
	TArray<TSharedPtr<CPythonContext>, CMemoryManager> ActiveContexts;
	THashMap<TString, TString, CMemoryManager> InterpreterOptions;

	static bool bPythonInterpreterInitialized;
	static void* PythonLibrary;
};

/**
 * @brief Python类型转换器
 */
class CPythonTypeConverter
{
public:
	/**
	 * @brief 将C++值转换为Python对象
	 */
	template <typename T>
	static PyObject* ToPyObject(const T& Value);

	/**
	 * @brief 从Python对象获取C++值
	 */
	template <typename T>
	static T FromPyObject(PyObject* PyObj);

	/**
	 * @brief 检查Python对象是否为指定类型
	 */
	template <typename T>
	static bool IsPyType(PyObject* PyObj);

	/**
	 * @brief 转换CScriptValue为CPythonValue
	 */
	static CPythonValue ToPythonValue(const CScriptValue& ScriptValue);

	/**
	 * @brief 转换CPythonValue为CScriptValue
	 */
	static CScriptValue FromPythonValue(const CPythonValue& PythonValue);

	/**
	 * @brief 转换ConfigValue为Python对象
	 */
	static PyObject* ConfigValueToPython(const CConfigValue& Config);

	/**
	 * @brief 转换Python对象为ConfigValue
	 */
	static CConfigValue PythonToConfigValue(PyObject* PyObj);

	/**
	 * @brief 获取Python对象的类型名称
	 */
	static TString GetPythonTypeName(PyObject* PyObj);

	/**
	 * @brief 创建Python列表
	 */
	static PyObject* CreatePythonList(const TArray<PyObject*, CMemoryManager>& Elements);

	/**
	 * @brief 创建Python字典
	 */
	static PyObject* CreatePythonDict(const THashMap<TString, PyObject*, CMemoryManager>& Elements);

	/**
	 * @brief 从Python列表获取元素
	 */
	static TArray<PyObject*, CMemoryManager> GetPythonListElements(PyObject* PyList);

	/**
	 * @brief 从Python字典获取键值对
	 */
	static THashMap<TString, PyObject*, CMemoryManager> GetPythonDictItems(PyObject* PyDict);
};

// === 类型别名 ===
using FPythonEngine = CPythonEngine;
using FPythonContext = CPythonContext;
using FPythonModule = CPythonModule;
using FPythonValue = CPythonValue;

} // namespace NLib

// === Python绑定辅助宏 ===

/**
 * @brief 注册C++函数到Python
 */
#define PY_BIND_FUNCTION(Context, Name, Function) /* TODO: 实现Python函数绑定 */

/**
 * @brief 注册C++对象到Python
 */
#define PY_BIND_OBJECT(Context, Name, Object) /* TODO: 实现Python对象绑定 */

/**
 * @brief 检查Python函数参数数量
 */
#define PY_CHECK_ARGS(Args, Expected)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if (PyTuple_Size(Args) != Expected)                                                                            \
		{                                                                                                              \
			PyErr_SetString(PyExc_TypeError, "Invalid number of arguments");                                           \
			return nullptr;                                                                                            \
		}                                                                                                              \
	} while (0)

/**
 * @brief Python异常安全包装
 */
#define PY_EXCEPTION_SAFE(code)                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		try                                                                                                            \
		{                                                                                                              \
			code                                                                                                       \
		}                                                                                                              \
		catch (const std::exception& e)                                                                                \
		{                                                                                                              \
			PyErr_SetString(PyExc_RuntimeError, e.what());                                                             \
			return nullptr;                                                                                            \
		}                                                                                                              \
	} while (0)

/*
使用示例：

```cpp
// 初始化Python引擎
auto PyEngine = MakeShared<CPythonEngine>();
PyEngine->Initialize();

// 创建上下文
SScriptConfig Config;
Config.bEnableSandbox = true;
Config.TimeoutMilliseconds = 10000;
auto Context = PyEngine->CreateContext(Config);

// 执行Python代码
TString PythonCode = TEXT(R"(
import sys
import math

class GameAI:
    def __init__(self, name):
        self.name = name
        self.health = 100
        self.level = 1

    def calculate_damage(self, base_damage, target_defense):
        # 使用复杂的伤害计算公式
        damage_multiplier = math.pow(self.level, 1.2)
        effective_damage = base_damage * damage_multiplier
        final_damage = max(1, effective_damage - target_defense)
        return int(final_damage)

    def make_decision(self, game_state):
        # AI决策逻辑
        if game_state['player_health'] < 30:
            return 'attack'
        elif self.health < 50:
            return 'heal'
        else:
            return 'defend'

# 创建AI实例
ai = GameAI("SmartAI")
print(f"Created AI: {ai.name}")

# 计算伤害
damage = ai.calculate_damage(50, 10)
print(f"Calculated damage: {damage}")

# 返回AI对象供C++使用
ai
)");

auto Result = Context->ExecutePython(PythonCode);
if (Result.Result == EScriptResult::Success) {
    auto AIObject = Result.ReturnValue;
    // 在C++中调用Python对象的方法
    auto DecisionResult = AIObject.CallMethod("make_decision", {
        PyEngine->CreateObject() // 游戏状态对象
    });
}
```

注意：
1. 这个实现需要链接Python开发库（python3-dev）
2. 支持Python 3.8+版本
3. 包含完整的异常处理和内存管理
4. 支持Python包管理和模块导入
5. 提供沙箱和安全特性
6. 包含类型转换和C++互操作功能

构建要求：
- Python 3.8+开发库
- Python C API头文件
- 适当的链接器设置

依赖库：
- libpython3.x-dev (开发库)
- python3-distutils (包管理)
- python3-pip (包安装器)
*/