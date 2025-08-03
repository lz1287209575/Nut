#pragma once

/**
 * @file NPythonEngine.h
 * @brief Python脚本引擎实现
 * 
 * 基于CPython 3.11+的Python脚本引擎，支持完整的Python生态
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"

// Python headers
#include "Python.h"
#include "frameobject.h"

namespace NLib
{

/**
 * @brief Python环境配置
 */
struct LIBNLIB_API NPythonConfig
{
    CString PythonHome;                     // Python安装路径
    CString PythonPath;                     // Python路径
    CArray<CString> ModulePaths;            // 模块搜索路径
    bool bIsolated = false;                 // 是否隔离环境
    bool bUseEnvironmentVariables = true;   // 是否使用环境变量
    bool bBufferedStdio = true;             // 是否缓冲标准IO
    bool bOptimize = true;                  // 是否启用优化
    int32_t VerboseLevel = 0;               // 详细程度 (0-3)
    bool bInteractive = false;              // 是否交互模式
    bool bInspect = false;                  // 是否检查模式
    CString StartupScript;                  // 启动脚本
    
    NPythonConfig();
    void ApplyToPythonConfig(PyConfig* Config) const;
};

/**
 * @brief Python类型映射器
 */
class LIBNLIB_API NPythonTypeMapper
{
public:
    // C++类型到Python类型
    static CString CppTypeToPython(const CString& CppType);
    static CString GenerateTypeHint(const CString& CppType, bool bIsArray = false);
    
    // 生成Python类型存根
    static CString GenerateClassStub(const CString& ClassName, const NScriptClassMeta& Meta);
    static CString GenerateFunctionStub(const CString& FunctionName, const NScriptFunctionMeta& Meta);
    static CString GeneratePropertyStub(const CString& PropertyName, const NScriptPropertyMeta& Meta);
    
    // 生成完整的.pyi文件
    static CString GenerateStubFile(const CArray<CString>& ClassNames);
    
private:
    static CString SanitizePythonIdentifier(const CString& Identifier);
    static CString GenerateDocstring(const CString& Description, const CArray<CString>& Parameters = {});
};

/**
 * @brief Python上下文实现
 */
class LIBNLIB_API NPythonContext : public IScriptContext
{
public:
    explicit NPythonContext(PyObject* InGlobalDict = nullptr);
    virtual ~NPythonContext();

    // IScriptContext接口实现
    virtual void SetGlobal(const CString& Name, const CScriptValue& Value) override;
    virtual CScriptValue GetGlobal(const CString& Name) const override;
    virtual bool HasGlobal(const CString& Name) const override;

    virtual void BindObject(const CString& Name, CObject* Object) override;
    virtual void UnbindObject(const CString& Name) override;

    virtual void BindFunction(const CString& Name, const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Function) override;
    virtual void UnbindFunction(const CString& Name) override;

    virtual bool LoadModule(const CString& ModuleName, const CString& ModulePath) override;
    virtual bool UnloadModule(const CString& ModuleName) override;
    virtual CArray<CString> GetLoadedModules() const override;

    virtual NScriptResult Execute(const CString& Code) override;
    virtual NScriptResult ExecuteFile(const CString& FilePath) override;
    virtual NScriptResult CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args = {}) override;

    virtual void SetBreakpoint(const CString& FilePath, int32_t Line) override;
    virtual void RemoveBreakpoint(const CString& FilePath, int32_t Line) override;
    virtual void SetDebugMode(bool bEnabled) override;

    virtual void CollectGarbage() override;
    virtual size_t GetMemoryUsage() const override;

    // Python特定接口
    PyObject* GetGlobalDict() const { return GlobalDict; }
    PyObject* GetLocalDict() const { return LocalDict; }
    
    // 模块管理
    PyObject* ImportModule(const CString& ModuleName);
    bool ReloadModule(const CString& ModuleName);
    PyObject* GetModule(const CString& ModuleName) const;
    
    // 对象创建和操作
    PyObject* CreatePythonObject(const CString& ClassName, const CArray<CScriptValue>& Args = {});
    CScriptValue CallPythonMethod(PyObject* Object, const CString& MethodName, const CArray<CScriptValue>& Args = {});
    CScriptValue GetPythonAttribute(PyObject* Object, const CString& AttributeName);
    void SetPythonAttribute(PyObject* Object, const CString& AttributeName, const CScriptValue& Value);
    
    // 异常处理
    bool HasPythonError() const;
    CString GetPythonError() const;
    void ClearPythonError();
    
    // 值转换
    PyObject* ScriptValueToPython(const CScriptValue& Value);
    CScriptValue PythonValueToScript(PyObject* Value);
    
    // 调试支持
    void SetTraceFunction(const NFunction<void(const CString&, int32_t, const CString&)>& TraceFunc);
    void EnableProfiling(bool bEnabled = true);
    CHashMap<CString, double> GetProfilingData() const;

private:
    PyObject* GlobalDict;
    PyObject* LocalDict;
    
    // 绑定的对象和函数
    CHashMap<CString, CObject*> BoundObjects;
    CHashMap<CString, NFunction<CScriptValue(const CArray<CScriptValue>&)>> BoundFunctions;
    CArray<CString> LoadedModules;
    
    // 调试和分析
    bool bDebugMode;
    bool bProfilingEnabled;
    NFunction<void(const CString&, int32_t, const CString&)> TraceFunction;
    
    // Python回调函数
    static int PythonTraceCallback(PyObject* obj, PyFrameObject* frame, int what, PyObject* arg);
    static PyObject* PythonFunctionCallback(PyObject* self, PyObject* args);
    static PyObject* PythonObjectGetAttr(PyObject* self, PyObject* name);
    static int PythonObjectSetAttr(PyObject* self, PyObject* name, PyObject* value);
    
    // 错误处理
    NScriptResult HandlePythonException(const CString& Context = "");
    
    // 对象包装
    void RegisterObjectWrapper(CObject* Object, const CString& Name);
    PyObject* CreateObjectWrapper(CObject* Object);
    
    mutable NMutex ContextMutex;
};

/**
 * @brief Python引擎实现
 */
class LIBNLIB_API NPythonEngine : public IScriptEngine
{
public:
    NPythonEngine();
    virtual ~NPythonEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::Python; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib Python Engine"; }

    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual bool IsInitialized() const override { return bInitialized; }

    virtual TSharedPtr<IScriptContext> CreateContext() override;
    virtual void DestroyContext(TSharedPtr<IScriptContext> Context) override;
    virtual TSharedPtr<IScriptContext> GetMainContext() override;

    virtual bool RegisterClass(const CString& ClassName) override;
    virtual bool UnregisterClass(const CString& ClassName) override;
    virtual bool IsClassRegistered(const CString& ClassName) const override;

    virtual bool AutoBindClasses() override;
    virtual bool AutoBindClass(const CString& ClassName) override;

    virtual bool EnableHotReload(const CString& WatchDirectory) override;
    virtual void DisableHotReload() override;
    virtual bool IsHotReloadEnabled() const override { return bHotReloadEnabled; }

    virtual void ResetStatistics() override;
    virtual CHashMap<CString, double> GetStatistics() const override;

    // Python特定功能
    bool SetConfig(const NPythonConfig& Config);
    const NPythonConfig& GetConfig() const { return PythonConfig; }
    
    // 包管理
    bool InstallPackage(const CString& PackageName, const CString& Version = "");
    bool UninstallPackage(const CString& PackageName);
    CArray<CString> GetInstalledPackages() const;
    bool UpdatePackage(const CString& PackageName);
    
    // 虚拟环境
    bool CreateVirtualEnvironment(const CString& EnvPath);
    bool ActivateVirtualEnvironment(const CString& EnvPath);
    bool DeactivateVirtualEnvironment();
    
    // Jupyter集成
    bool StartJupyterKernel(int32_t Port = 8888);
    bool StopJupyterKernel();
    bool ExecuteNotebook(const CString& NotebookPath);
    
    // 类型存根生成
    bool GenerateTypeStubs(const CString& OutputPath);
    bool UpdateTypeStubs();
    
    // 性能优化
    bool EnableJIT(bool bEnabled = true);           // PyPy JIT或Numba
    bool PrecompileModules(const CArray<CString>& ModulePaths);

private:
    bool bInitialized;
    TSharedPtr<NPythonContext> MainContext;
    CArray<TWeakPtr<NPythonContext>> CreatedContexts;
    
    // Python配置
    NPythonConfig PythonConfig;
    
    // 类注册信息
    NHashSet<CString> RegisteredClasses;
    
    // 热重载
    bool bHotReloadEnabled;
    CString WatchDirectory;
    TSharedPtr<CThread> HotReloadThread;
    NFileSystemWatcher FileWatcher;
    
    // 性能统计
    mutable NMutex StatsMutex;
    CHashMap<CString, double> Statistics;
    
    // 初始化辅助
    bool InitializePython();
    bool SetupPythonPath();
    void RegisterBuiltinModules();
    void SetupErrorHandling();
    
    // 类绑定辅助
    bool BindPythonClass(const CString& ClassName, const NScriptClassMeta& ClassMeta);
    CString GenerateClassBinding(const CString& ClassName, const NScriptClassMeta& Meta);
    PyObject* CreateClassType(const CString& ClassName, const NScriptClassMeta& Meta);
    
    // 热重载实现
    void HotReloadThreadFunction();
    void OnFileChanged(const CString& FilePath);
    bool ReloadPythonModule(const CString& FilePath);
    
    // 包管理实现
    bool ExecutePipCommand(const CArray<CString>& Args);
    
    mutable NMutex EngineMutex;
};

/**
 * @brief Python对象包装器
 */
class LIBNLIB_API NPythonObjectWrapper
{
public:
    static PyTypeObject* CreateObjectType(const CString& ClassName, const NScriptClassMeta& Meta);
    static PyMethodDef* CreateMethodTable(const CString& ClassName, const NScriptClassMeta& Meta);
    static PyGetSetDef* CreatePropertyTable(const CString& ClassName, const NScriptClassMeta& Meta);
    
    // 方法包装器
    static PyObject* PropertyGetter(PyObject* self, void* closure);
    static int PropertySetter(PyObject* self, PyObject* value, void* closure);
    static PyObject* MethodWrapper(PyObject* self, PyObject* args, PyObject* kwargs);
    
    // 对象生命周期
    static PyObject* ObjectNew(PyTypeObject* type, PyObject* args, PyObject* kwargs);
    static void ObjectDealloc(PyObject* self);
    static int ObjectInit(PyObject* self, PyObject* args, PyObject* kwargs);
    
private:
    struct ObjectWrapperData
    {
        PyObject_HEAD
        CObject* CppObject;
        CString ClassName;
    };
};

/**
 * @brief Python代码生成器
 */
class LIBNLIB_API NPythonCodeGenerator
{
public:
    // 生成Python类绑定
    static CString GenerateClassBinding(const CString& ClassName, const NScriptClassMeta& Meta);
    static CString GeneratePropertyBinding(const CString& PropertyName, const NScriptPropertyMeta& Meta);
    static CString GenerateFunctionBinding(const CString& FunctionName, const NScriptFunctionMeta& Meta);
    
    // 生成模块代码
    static CString GenerateModuleInit(const CArray<CString>& ClassNames);
    static CString GenerateSetupPy(const CString& ModuleName, const CArray<CString>& SourceFiles);
    
    // 生成包装代码
    static CString GenerateCExtensionModule(const CString& ModuleName, const CArray<CString>& Classes);
    static CString GeneratePybind11Module(const CString& ModuleName, const CArray<CString>& Classes);
    
private:
    static CString IndentPythonCode(const CString& Code, int32_t IndentLevel = 1);
    static CString GeneratePythonDocstring(const CString& Description, const CArray<CString>& Parameters = {});
};

/**
 * @brief Python示例代码
 */
namespace PythonExamples
{
    // 基本类使用
    extern const char* BasicClassExample;
    
    // 异步编程
    extern const char* AsyncExample;
    
    // 装饰器使用
    extern const char* DecoratorExample;
    
    // 数据类使用
    extern const char* DataClassExample;
    
    // NumPy集成
    extern const char* NumPyExample;
    
    // 网络编程
    extern const char* NetworkExample;
}

/**
 * @brief 生成的Python代码示例
 * 
 * 从C++类：
 * NCLASS(ScriptCreatable, Category="Gameplay")
 * class CPlayer : public NGameObject {
 * public:
 *     NPROPERTY(ScriptReadWrite) float Health = 100.0f;
 *     NFUNCTION(ScriptCallable) void TakeDamage(float Amount);
 * };
 * 
 * 生成Python绑定：
 * import nlib
 * from typing import Optional
 * 
 * class Player(nlib.GameObject):
 *     """Player character
 *     
 *     Category: Gameplay
 *     """
 *     
 *     def __init__(self) -> None:
 *         """Initialize a new Player instance."""
 *         super().__init__()
 *         self._health: float = 100.0
 *     
 *     @property
 *     def health(self) -> float:
 *         """Player health"""
 *         return self._health
 *     
 *     @health.setter
 *     def health(self, value: float) -> None:
 *         """Set player health"""
 *         self._health = max(0.0, min(100.0, value))
 *     
 *     def take_damage(self, amount: float) -> None:
 *         """Take damage
 *         
 *         Args:
 *             amount: Damage amount to take
 *         """
 *         self._health -= amount
 *         if self._health <= 0:
 *             self._on_death()
 *     
 *     def _on_death(self) -> None:
 *         """Called when player dies"""
 *         print(f"Player {self.name} has died")
 * 
 * 以及类型存根(.pyi)文件：
 * from nlib import GameObject
 * from typing import Optional
 * 
 * class Player(GameObject):
 *     health: float
 *     def __init__(self) -> None: ...
 *     def take_damage(self, amount: float) -> None: ...
 */

} // namespace NLib