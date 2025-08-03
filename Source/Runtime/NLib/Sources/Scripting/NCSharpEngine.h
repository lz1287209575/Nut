#pragma once

/**
 * @file NCSharpEngine.h
 * @brief C#脚本引擎实现
 * 
 * 基于.NET 8.0 CoreCLR的C#脚本引擎，支持动态编译和执行C#代码
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"
#include "Processes/NProcess.h"

// .NET CoreCLR headers
#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "nethost.h"

namespace NLib
{

/**
 * @brief C#编译选项
 */
struct LIBNLIB_API NCSharpCompileOptions
{
    CString TargetFramework = "net8.0";         // 目标框架
    CString LanguageVersion = "latest";         // C#语言版本
    bool Nullable = true;                       // 启用可空引用类型
    bool TreatWarningsAsErrors = false;         // 将警告视为错误
    bool Optimize = true;                       // 启用优化
    bool GenerateDebugInfo = true;              // 生成调试信息
    CString Platform = "AnyCPU";                // 目标平台
    CString Configuration = "Release";           // 配置
    CArray<CString> References;                 // 引用的程序集
    CArray<CString> UsingNamespaces;            // 默认using命名空间
    CString OutputType = "Library";             // 输出类型 (Library, Exe)
    
    NCSharpCompileOptions();
    CString ToCompilerArguments() const;
};

/**
 * @brief C#代码生成器
 */
class LIBNLIB_API NCSharpCodeGenerator
{
public:
    // 从C++类生成C#代码
    static CString GenerateClassDefinition(const CString& ClassName, const NScriptClassMeta& Meta);
    static CString GenerateProperty(const CString& PropertyName, const NScriptPropertyMeta& Meta);
    static CString GenerateMethod(const CString& MethodName, const NScriptFunctionMeta& Meta);
    
    // 生成互操作代码
    static CString GenerateInteropClass(const CString& ClassName);
    static CString GeneratePInvokeDeclarations(const CArray<CString>& Functions);
    static CString GenerateMarshallingCode(const CString& Type);
    
    // 生成特性代码
    static CString GenerateAttributes(const NScriptMetaInfo& Meta);
    static CString GenerateXmlDoc(const CString& Description, const CArray<CString>& Parameters = {});
    
    // 类型映射
    static CString CppTypeToCSharp(const CString& CppType);
    static CString GenerateDefaultValue(const CString& Type);
    
private:
    static CString SanitizeIdentifier(const CString& Identifier);
    static CString FormatCode(const CString& Code);
};

/**
 * @brief C#上下文实现
 */
class LIBNLIB_API NCSharpContext : public IScriptContext
{
public:
    explicit NCSharpContext(void* InDotnetRuntime);
    virtual ~NCSharpContext();

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

    // C#特定接口
    void* GetDotnetRuntime() const { return DotnetRuntime; }
    
    // 动态编译和执行
    NScriptResult CompileAndExecute(const CString& CSharpCode, const CString& ClassName = "", const CString& MethodName = "Main");
    NScriptResult CompileAssembly(const CString& CSharpCode, const CString& AssemblyName);
    NScriptResult LoadAssembly(const CString& AssemblyPath);
    
    // 类型操作
    CScriptValue CreateInstance(const CString& TypeName, const CArray<CScriptValue>& Args = {});
    CScriptValue CallStaticMethod(const CString& TypeName, const CString& MethodName, const CArray<CScriptValue>& Args = {});
    CScriptValue CallInstanceMethod(const CScriptValue& Instance, const CString& MethodName, const CArray<CScriptValue>& Args = {});
    
    // 属性操作
    CScriptValue GetStaticProperty(const CString& TypeName, const CString& PropertyName);
    void SetStaticProperty(const CString& TypeName, const CString& PropertyName, const CScriptValue& Value);
    CScriptValue GetInstanceProperty(const CScriptValue& Instance, const CString& PropertyName);
    void SetInstanceProperty(const CScriptValue& Instance, const CString& PropertyName, const CScriptValue& Value);
    
    // 事件系统
    void SubscribeEvent(const CScriptValue& Instance, const CString& EventName, const NFunction<void(const CArray<CScriptValue>&)>& Handler);
    void UnsubscribeEvent(const CScriptValue& Instance, const CString& EventName);
    
    // 异步支持
    void SetTaskScheduler(const NFunction<void(const NFunction<void()>&)>& Scheduler);
    CScriptValue CreateTask(const NFunction<CScriptValue()>& TaskFunction);
    CScriptValue AwaitTask(const CScriptValue& Task);

private:
    void* DotnetRuntime;
    
    // .NET运行时函数指针
    load_assembly_and_get_function_pointer_fn LoadAssemblyAndGetFunctionPointer;
    get_function_pointer_fn GetFunctionPointer;
    
    // 编译相关
    NCSharpCompileOptions CompileOptions;
    CString TempDirectory;
    int32_t NextAssemblyId;
    
    // 程序集管理
    CHashMap<CString, void*> LoadedAssemblies;
    CHashMap<CString, CString> AssemblyPaths;
    
    // 全局变量和函数
    CHashMap<CString, CScriptValue> GlobalVariables;
    CHashMap<CString, NFunction<CScriptValue(const CArray<CScriptValue>&)>> BoundFunctions;
    
    // 值转换
    CScriptValue ManagedValueToScript(void* ManagedValue, const CString& TypeName);
    void* ScriptValueToManaged(const CScriptValue& Value, const CString& TypeName);
    
    // C#编译
    bool CompileCSharpCode(const CString& Code, const CString& OutputPath);
    CString GenerateWrapperClass(const CString& UserCode);
    
    // 程序集操作
    void* LoadManagedAssembly(const CString& AssemblyPath);
    void* GetManagedFunction(void* Assembly, const CString& TypeName, const CString& MethodName);
    
    // 错误处理
    NScriptResult HandleDotnetException(const CString& Context = "");
    
    mutable NMutex ContextMutex;
};

/**
 * @brief C#引擎实现
 */
class LIBNLIB_API NCSharpEngine : public IScriptEngine
{
public:
    NCSharpEngine();
    virtual ~NCSharpEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::CSharp; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib C# Engine"; }

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

    // C#特定功能
    bool SetCompileOptions(const NCSharpCompileOptions& Options);
    const NCSharpCompileOptions& GetCompileOptions() const { return CompileOptions; }
    
    // 项目管理
    bool CreateCSharpProject(const CString& ProjectPath, const CString& ProjectName);
    bool BuildProject(const CString& ProjectPath);
    bool PublishProject(const CString& ProjectPath, const CString& OutputPath);
    
    // NuGet包管理
    bool InstallPackage(const CString& PackageName, const CString& Version = "");
    bool UninstallPackage(const CString& PackageName);
    CArray<CString> GetInstalledPackages() const;
    
    // 调试支持
    bool EnableDebugging(bool bEnabled = true);
    bool AttachDebugger(int32_t ProcessId = 0);
    
    // 性能分析
    bool EnableProfiling(bool bEnabled = true);
    CHashMap<CString, double> GetProfilingData() const;

private:
    bool bInitialized;
    TSharedPtr<NCSharpContext> MainContext;
    CArray<TWeakPtr<NCSharpContext>> CreatedContexts;
    
    // .NET运行时
    void* DotnetRuntime;
    hostfxr_handle RuntimeHost;
    
    // 编译选项
    NCSharpCompileOptions CompileOptions;
    
    // 类注册信息
    NHashSet<CString> RegisteredClasses;
    
    // 热重载
    bool bHotReloadEnabled;
    CString WatchDirectory;
    TSharedPtr<CThread> HotReloadThread;
    NFileSystemWatcher FileWatcher;
    NProcess DotnetWatchProcess;            // dotnet watch进程
    
    // 性能统计
    mutable NMutex StatsMutex;
    CHashMap<CString, double> Statistics;
    
    // 初始化辅助
    bool InitializeDotnetRuntime();
    bool LoadHostFxr();
    bool CreateRuntimeHost();
    void SetupRuntimeConfig();
    
    // 类绑定辅助
    bool BindCSharpClass(const CString& ClassName, const NScriptClassMeta& ClassMeta);
    CString GenerateClassBinding(const CString& ClassName, const NScriptClassMeta& Meta);
    CString GenerateInteropAssembly();
    
    // 热重载实现
    void HotReloadThreadFunction();
    void OnFileChanged(const CString& FilePath);
    bool RecompileAndReload(const CString& FilePath);
    
    mutable NMutex EngineMutex;
};

/**
 * @brief C#互操作助手
 */
class LIBNLIB_API NCSharpInterop
{
public:
    // P/Invoke函数生成
    static CString GeneratePInvokeFunction(const CString& FunctionName, const NScriptFunctionMeta& Meta);
    static CString GenerateCallbackDelegate(const CString& DelegateName, const NScriptFunctionMeta& Meta);
    
    // 结构体封送
    static CString GenerateStructMarshalling(const CString& StructName, const CArray<CString>& Fields);
    static CString GenerateArrayMarshalling(const CString& ElementType);
    
    // 字符串转换
    static CString GenerateStringConversion(bool bIsUtf8 = true);
    
    // 错误处理
    static CString GenerateExceptionHandling();
    
private:
    static CString GetMarshalAsAttribute(const CString& CppType);
    static CString GetCallingConvention();
};

/**
 * @brief C#示例代码
 */
namespace CSharpExamples
{
    // 基本类定义
    extern const char* BasicClassExample;
    
    // 接口实现
    extern const char* InterfaceExample;
    
    // 泛型类
    extern const char* GenericClassExample;
    
    // 异步/Task
    extern const char* AsyncExample;
    
    // 特性使用
    extern const char* AttributeExample;
    
    // 互操作
    extern const char* InteropExample;
}

/**
 * @brief 生成的C#代码示例
 * 
 * 从C++类：
 * NCLASS(ScriptCreatable, Category="Gameplay")
 * class CPlayer : public NGameObject {
 * public:
 *     NPROPERTY(ScriptReadWrite) float Health = 100.0f;
 *     NFUNCTION(ScriptCallable) void TakeDamage(float Amount);
 * };
 * 
 * 生成C#代码：
 * using System;
 * using System.Runtime.InteropServices;
 * using NLib.Scripting;
 * 
 * namespace NLib.Gameplay
 * {
 *     /// <summary>
 *     /// Player character
 *     /// </summary>
 *     [ScriptCreatable, Category("Gameplay")]
 *     public class Player : GameObject
 *     {
 *         private float _health = 100.0f;
 *         
 *         /// <summary>
 *         /// Player health
 *         /// </summary>
 *         [ScriptReadWrite]
 *         public float Health
 *         {
 *             get => _health;
 *             set => _health = value;
 *         }
 *         
 *         /// <summary>
 *         /// Take damage
 *         /// </summary>
 *         /// <param name="amount">Damage amount</param>
 *         [ScriptCallable]
 *         public void TakeDamage(float amount)
 *         {
 *             _health -= amount;
 *             if (_health <= 0)
 *             {
 *                 OnDeath();
 *             }
 *         }
 *         
 *         protected virtual void OnDeath()
 *         {
 *             Console.WriteLine($"Player {Name} has died");
 *         }
 *     }
 * }
 * 
 * 以及P/Invoke互操作代码：
 * internal static class PlayerInterop
 * {
 *     [DllImport("NLib", CallingConvention = CallingConvention.Cdecl)]
 *     public static extern IntPtr CreatePlayer([MarshalAs(UnmanagedType.LPStr)] string name);
 *     
 *     [DllImport("NLib", CallingConvention = CallingConvention.Cdecl)]
 *     public static extern float GetPlayerHealth(IntPtr player);
 *     
 *     [DllImport("NLib", CallingConvention = CallingConvention.Cdecl)]
 *     public static extern void SetPlayerHealth(IntPtr player, float health);
 *     
 *     [DllImport("NLib", CallingConvention = CallingConvention.Cdecl)]
 *     public static extern void PlayerTakeDamage(IntPtr player, float amount);
 * }
 */

} // namespace NLib