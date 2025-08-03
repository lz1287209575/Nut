#pragma once

/**
 * @file NLuaEngine.h
 * @brief Lua脚本引擎实现
 * 
 * 基于Lua 5.4的脚本引擎实现，支持自动绑定和热重载
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"

// Lua headers
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace NLib
{

/**
 * @brief Lua上下文实现
 */
class LIBNLIB_API NLuaContext : public IScriptContext
{
public:
    explicit NLuaContext(lua_State* InLuaState);
    virtual ~NLuaContext();

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

    // Lua特定接口
    lua_State* GetLuaState() const { return L; }
    
    // 值转换
    void PushScriptValue(const CScriptValue& Value);
    CScriptValue PopScriptValue();
    CScriptValue GetScriptValueFromIndex(int Index);

private:
    lua_State* L;
    CHashMap<CString, NFunction<CScriptValue(const CArray<CScriptValue>&)>> BoundFunctions;
    CHashMap<CString, CObject*> BoundObjects;
    CArray<CString> LoadedModules;
    bool bDebugMode;
    
    // Lua回调函数
    static int LuaFunctionCallback(lua_State* L);
    static int LuaObjectIndexCallback(lua_State* L);
    static int LuaObjectNewIndexCallback(lua_State* L);
    static int LuaObjectGCCallback(lua_State* L);
    
    // 错误处理
    NScriptResult HandleLuaError(int Result, const CString& Context = "");
    static int LuaErrorHandler(lua_State* L);
    static void LuaDebugHook(lua_State* L, lua_Debug* ar);
    
    // 对象绑定辅助
    void CreateObjectMetatable(const CString& ClassName);
    void RegisterObjectInstance(CObject* Object, const CString& Name);
    
    mutable NMutex ContextMutex;
};

/**
 * @brief Lua引擎实现
 */
class LIBNLIB_API NLuaEngine : public IScriptEngine
{
public:
    NLuaEngine();
    virtual ~NLuaEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::Lua; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib Lua Engine"; }

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

    // Lua特定功能
    void SetMemoryLimit(size_t LimitBytes);
    size_t GetMemoryLimit() const { return MemoryLimit; }
    
    void SetExecutionTimeout(double TimeoutSeconds);
    double GetExecutionTimeout() const { return ExecutionTimeout; }

    bool LoadStandardLibraries();
    bool LoadLibrary(const CString& LibraryName);

    // 预编译脚本支持
    bool CompileScript(const CString& Code, const CString& OutputPath);
    bool LoadCompiledScript(const CString& FilePath);

private:
    bool bInitialized;
    TSharedPtr<NLuaContext> MainContext;
    CArray<TWeakPtr<NLuaContext>> CreatedContexts;
    
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
    
    // 内存管理
    size_t MemoryLimit;
    double ExecutionTimeout;
    
    // 初始化辅助
    bool InitializeLuaState(lua_State* L);
    void SetupErrorHandling(lua_State* L);
    void SetupDebugHooks(lua_State* L);
    
    // 类绑定辅助
    bool BindLuaClass(const CString& ClassName, const NScriptClassMeta& ClassMeta);
    bool BindLuaProperty(lua_State* L, const CString& ClassName, const CString& PropertyName, const NScriptPropertyMeta& PropertyMeta);
    bool BindLuaFunction(lua_State* L, const CString& ClassName, const CString& FunctionName, const NScriptFunctionMeta& FunctionMeta);
    
    // 热重载实现
    void HotReloadThreadFunction();
    void OnFileChanged(const CString& FilePath);
    bool ReloadScript(const CString& FilePath);
    
    // 内存分配器
    static void* LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize);
    
    // 执行超时检查
    static void LuaTimeoutHook(lua_State* L, lua_Debug* ar);
    
    mutable NMutex EngineMutex;
};

/**
 * @brief Lua脚本值转换工具
 */
class LIBNLIB_API NLuaValueConverter
{
public:
    // C++到Lua
    static void PushValue(lua_State* L, const CScriptValue& Value);
    static void PushBool(lua_State* L, bool Value);
    static void PushInt(lua_State* L, int32_t Value);
    static void PushFloat(lua_State* L, float Value);
    static void PushDouble(lua_State* L, double Value);
    static void PushString(lua_State* L, const CString& Value);
    static void PushObject(lua_State* L, CObject* Object);
    static void PushArray(lua_State* L, const CArray<CScriptValue>& Array);
    
    // Lua到C++
    static CScriptValue GetValue(lua_State* L, int Index);
    static bool GetBool(lua_State* L, int Index, bool DefaultValue = false);
    static int32_t GetInt(lua_State* L, int Index, int32_t DefaultValue = 0);
    static float GetFloat(lua_State* L, int Index, float DefaultValue = 0.0f);
    static double GetDouble(lua_State* L, int Index, double DefaultValue = 0.0);
    static CString GetString(lua_State* L, int Index, const CString& DefaultValue = "");
    static CObject* GetObject(lua_State* L, int Index);
    static CArray<CScriptValue> GetArray(lua_State* L, int Index);
    
    // 类型检查
    static bool IsBool(lua_State* L, int Index);
    static bool IsInt(lua_State* L, int Index);
    static bool IsNumber(lua_State* L, int Index);
    static bool IsString(lua_State* L, int Index);
    static bool IsObject(lua_State* L, int Index);
    static bool IsArray(lua_State* L, int Index);
};

/**
 * @brief Lua绑定辅助宏
 */
#define LUA_BIND_CLASS_BEGIN(ClassName) \
    bool Bind##ClassName##ToLua(lua_State* L) { \
        const char* className = #ClassName; \
        luaL_newmetatable(L, className);

#define LUA_BIND_PROPERTY(PropertyName, Getter, Setter) \
        lua_pushstring(L, #PropertyName); \
        lua_pushcfunction(L, Getter); \
        lua_settable(L, -3); \
        if (Setter) { \
            lua_pushstring(L, "set_" #PropertyName); \
            lua_pushcfunction(L, Setter); \
            lua_settable(L, -3); \
        }

#define LUA_BIND_FUNCTION(FunctionName, Function) \
        lua_pushstring(L, #FunctionName); \
        lua_pushcfunction(L, Function); \
        lua_settable(L, -3);

#define LUA_BIND_CLASS_END() \
        lua_pop(L, 1); \
        return true; \
    }

/**
 * @brief Lua脚本示例和测试
 */
namespace LuaExamples
{
    // 基本使用示例
    void BasicUsageExample();
    
    // 对象绑定示例
    void ObjectBindingExample();
    
    // 热重载示例
    void HotReloadExample();
    
    // 性能测试
    void PerformanceTest();
}

} // namespace NLib