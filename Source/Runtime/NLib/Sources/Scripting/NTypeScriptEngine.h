#pragma once

/**
 * @file NTypeScriptEngine.h
 * @brief TypeScript脚本引擎实现
 * 
 * 基于V8引擎和TypeScript编译器的实现，支持类型安全的脚本编程
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"
#include "Processes/NProcess.h"

// V8 headers
#include "v8.h"
#include "libplatform/libplatform.h"

namespace NLib
{

/**
 * @brief TypeScript编译选项
 */
struct LIBNLIB_API NTypeScriptCompileOptions
{
    CString Target = "ES2020";                  // 编译目标 (ES5, ES2015, ES2020, etc.)
    CString Module = "CommonJS";                // 模块系统 (CommonJS, ESNext, AMD, etc.)
    bool Strict = true;                         // 严格模式
    bool StrictNullChecks = true;               // 空值检查
    bool StrictFunctionTypes = true;            // 函数类型检查
    bool NoImplicitAny = true;                  // 禁止隐式any
    bool NoImplicitReturns = true;              // 禁止隐式返回
    bool NoUnusedLocals = false;                // 检查未使用的局部变量
    bool NoUnusedParameters = false;            // 检查未使用的参数
    bool ExactOptionalPropertyTypes = true;     // 精确可选属性类型
    bool NoImplicitOverride = true;             // 需要显式override
    CString Lib = "ES2020";                     // 使用的库
    CString ModuleResolution = "node";          // 模块解析策略
    bool AllowJs = true;                        // 允许JavaScript文件
    bool Declaration = true;                    // 生成.d.ts文件
    bool SourceMap = true;                      // 生成source map
    CString OutDir = "compiled";                // 输出目录
    CString RootDir = "src";                    // 根目录
    
    NTypeScriptCompileOptions();
    CString ToCompilerOptions() const;
};

/**
 * @brief TypeScript类型定义生成器
 */
class LIBNLIB_API NTypeScriptTypeGenerator
{
public:
    // 从C++类生成TypeScript类型定义
    static CString GenerateTypeDefinition(const CString& ClassName, const NScriptClassMeta& Meta);
    static CString GenerateInterface(const CString& InterfaceName, const CArray<CString>& Methods);
    static CString GenerateEnum(const CString& EnumName, const CArray<CString>& Values);
    
    // 生成模块定义
    static CString GenerateModuleDefinition(const CString& ModuleName, const CArray<CString>& Classes);
    static CString GenerateGlobalDefinitions(const CArray<CString>& GlobalFunctions);
    
    // 类型映射
    static CString CppTypeToTypeScript(const CString& CppType);
    static CString GenerateParameterList(const CArray<CString>& ParamTypes, const CArray<CString>& ParamNames);
    
private:
    static CString SanitizeTypeName(const CString& TypeName);
    static CString GenerateJSDoc(const CString& Description, const CArray<CString>& Tags = {});
};

/**
 * @brief TypeScript上下文实现
 */
class LIBNLIB_API NTypeScriptContext : public IScriptContext
{
public:
    explicit NTypeScriptContext(v8::Isolate* InIsolate);
    virtual ~NTypeScriptContext();

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

    // TypeScript特定接口
    v8::Isolate* GetIsolate() const { return Isolate; }
    v8::Local<v8::Context> GetV8Context() const;
    
    // TypeScript编译
    NScriptResult CompileTypeScript(const CString& TSCode, const CString& FileName = "");
    NScriptResult CompileAndExecute(const CString& TSCode, const CString& FileName = "");
    
    // 模块系统
    bool RegisterModule(const CString& ModuleName, const CString& ModuleCode);
    bool ImportModule(const CString& ModuleName);
    CScriptValue RequireModule(const CString& ModulePath);
    
    // 类型检查
    bool EnableTypeChecking(bool bEnabled = true);
    CArray<CString> GetTypeErrors() const;
    
    // Promise/async支持
    void SetPromiseRejectCallback(const NFunction<void(const CString&)>& Callback);
    void ProcessPromiseQueue();

private:
    v8::Isolate* Isolate;
    v8::Global<v8::Context> Context;
    
    // TypeScript编译器状态
    NTypeScriptCompileOptions CompileOptions;
    CString TSCompilerPath;                     // TypeScript编译器路径
    bool bTypeCheckingEnabled;
    
    // 模块管理
    CHashMap<CString, CString> RegisteredModules;
    NHashSet<CString> LoadedModules;
    
    // V8值转换
    v8::Local<v8::Value> ScriptValueToV8(const CScriptValue& Value);
    CScriptValue V8ValueToScript(v8::Local<v8::Value> Value);
    
    // TypeScript编译
    CString CompileTypeScriptToJS(const CString& TSCode, const CString& FileName);
    bool InvokeTypeScriptCompiler(const CString& InputFile, const CString& OutputFile);
    
    // 错误处理
    NScriptResult HandleV8Exception(v8::TryCatch& TryCatch, const CString& Context = "");
    
    // Promise处理
    NFunction<void(const CString&)> PromiseRejectCallback;
    std::queue<v8::Global<v8::Promise>> PendingPromises;
    
    mutable NMutex ContextMutex;
};

/**
 * @brief TypeScript引擎实现
 */
class LIBNLIB_API NTypeScriptEngine : public IScriptEngine
{
public:
    NTypeScriptEngine();
    virtual ~NTypeScriptEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::TypeScript; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib TypeScript Engine"; }

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

    // TypeScript特定功能
    bool SetCompileOptions(const NTypeScriptCompileOptions& Options);
    const NTypeScriptCompileOptions& GetCompileOptions() const { return CompileOptions; }
    
    // 类型定义生成
    bool GenerateTypeDefinitions(const CString& OutputPath);
    bool GenerateModuleDefinition(const CString& ModuleName, const CString& OutputPath);
    
    // 项目管理
    bool CreateTSProject(const CString& ProjectPath);
    bool CompileProject(const CString& ProjectPath);
    bool WatchProject(const CString& ProjectPath);
    
    // 类型检查
    bool TypeCheckFile(const CString& FilePath);
    CArray<CString> GetTypeCheckErrors() const;
    
    // 调试支持
    bool EnableSourceMaps(bool bEnabled = true);
    bool SetDebugPort(int32_t Port = 9229);

private:
    bool bInitialized;
    TSharedPtr<NTypeScriptContext> MainContext;
    CArray<TWeakPtr<NTypeScriptContext>> CreatedContexts;
    
    // V8相关
    v8::Isolate* Isolate;
    std::unique_ptr<v8::Platform> Platform;
    
    // 编译选项
    NTypeScriptCompileOptions CompileOptions;
    
    // 类注册信息
    NHashSet<CString> RegisteredClasses;
    
    // 热重载
    bool bHotReloadEnabled;
    CString WatchDirectory;
    TSharedPtr<CThread> HotReloadThread;
    NFileSystemWatcher FileWatcher;
    NProcess TypeScriptWatcher;                 // tsc --watch进程
    
    // 性能统计
    mutable NMutex StatsMutex;
    CHashMap<CString, double> Statistics;
    
    // 初始化辅助
    bool InitializeV8();
    bool InitializeTypeScriptCompiler();
    void SetupV8Context(v8::Local<v8::Context> Context);
    
    // 类绑定辅助
    bool BindTypeScriptClass(const CString& ClassName, const NScriptClassMeta& ClassMeta);
    CString GenerateClassBinding(const CString& ClassName, const NScriptClassMeta& Meta);
    
    // 热重载实现
    void HotReloadThreadFunction();
    void OnFileChanged(const CString& FilePath);
    bool RecompileAndReload(const CString& FilePath);
    
    mutable NMutex EngineMutex;
};

/**
 * @brief TypeScript模块系统
 */
class LIBNLIB_API NTypeScriptModuleSystem
{
public:
    // 模块解析
    static CString ResolveModule(const CString& ModuleName, const CString& CurrentPath);
    static bool IsBuiltinModule(const CString& ModuleName);
    
    // 模块缓存
    static void CacheModule(const CString& ModuleName, const CString& CompiledCode);
    static CString GetCachedModule(const CString& ModuleName);
    static void ClearCache();
    
    // 依赖分析
    static CArray<CString> ExtractDependencies(const CString& TSCode);
    static CArray<CString> ResolveDependencyTree(const CString& EntryModule);
    
private:
    static CHashMap<CString, CString> ModuleCache;
    static NMutex CacheMutex;
};

/**
 * @brief TypeScript示例代码
 */
namespace TypeScriptExamples
{
    // 基本类定义
    extern const char* BasicClassExample;
    
    // 接口定义
    extern const char* InterfaceExample;
    
    // 泛型类
    extern const char* GenericClassExample;
    
    // 模块定义
    extern const char* ModuleExample;
    
    // 异步/Promise
    extern const char* AsyncExample;
    
    // 装饰器
    extern const char* DecoratorExample;
}

/**
 * @brief 生成的TypeScript代码示例
 * 
 * 从C++类：
 * NCLASS(ScriptCreatable, Category="Gameplay")
 * class CPlayer : public NGameObject {
 * public:
 *     NPROPERTY(ScriptReadWrite) float Health = 100.0f;
 *     NFUNCTION(ScriptCallable) void TakeDamage(float Amount);
 * };
 * 
 * 生成TypeScript定义：
 * interface IGameObject {
 *     readonly position: Vector3;
 *     readonly name: string;
 *     move(offset: Vector3): void;
 * }
 * 
 * interface IPlayer extends IGameObject {
 *     health: number;
 *     takeDamage(amount: number): void;
 * }
 * 
 * declare class Player implements IPlayer {
 *     health: number;
 *     constructor();
 *     takeDamage(amount: number): void;
 *     
 *     // 继承自GameObject
 *     readonly position: Vector3;
 *     readonly name: string;
 *     move(offset: Vector3): void;
 * }
 * 
 * 以及实现代码：
 * class PlayerImpl extends GameObjectImpl implements IPlayer {
 *     private _health: number = 100.0;
 *     
 *     get health(): number { return this._health; }
 *     set health(value: number) { this._health = value; }
 *     
 *     takeDamage(amount: number): void {
 *         this._health -= amount;
 *         if (this._health <= 0) {
 *             this.onDeath();
 *         }
 *     }
 *     
 *     protected onDeath(): void {
 *         console.log(`Player ${this.name} has died`);
 *     }
 * }
 */

} // namespace NLib