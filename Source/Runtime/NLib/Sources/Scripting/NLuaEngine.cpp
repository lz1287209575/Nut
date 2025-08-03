#include "Scripting/NLuaEngine.h"
#include "Scripting/NScriptMeta.h"
#include "Memory/CAllocator.h"
#include "Core/CLogger.h"
#include "FileSystem/NFileSystem.h"
#include "Threading/CThread.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace NLib
{

// NLuaContext Implementation
NLuaContext::NLuaContext()
    : LuaState(nullptr)
    , bDebugMode(false)
{
    // Create new Lua state
    LuaState = luaL_newstate();
    if (LuaState)
    {
        // Open standard libraries
        luaL_openlibs(LuaState);
        
        // Set up error handling
        SetupErrorHandling();
        
        // Register NLib global table
        SetupNLibGlobals();
    }
}

NLuaContext::~NLuaContext()
{
    if (LuaState)
    {
        lua_close(LuaState);
        LuaState = nullptr;
    }
}

void NLuaContext::SetGlobal(const CString& Name, const CScriptValue& Value)
{
    if (!LuaState) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    PushScriptValue(Value);
    lua_setglobal(LuaState, *Name);
}

CScriptValue NLuaContext::GetGlobal(const CString& Name) const
{
    if (!LuaState) return CScriptValue();
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    lua_getglobal(LuaState, *Name);
    CScriptValue Result = PopScriptValue();
    return Result;
}

bool NLuaContext::HasGlobal(const CString& Name) const
{
    if (!LuaState) return false;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    lua_getglobal(LuaState, *Name);
    bool bExists = !lua_isnil(LuaState, -1);
    lua_pop(LuaState, 1);
    return bExists;
}

void NLuaContext::BindObject(const CString& Name, CObject* Object)
{
    if (!LuaState || !Object) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    BoundObjects.Add(Name, Object);
    
    // Create userdata for the object
    void** UserData = static_cast<void**>(lua_newuserdata(LuaState, sizeof(void*)));
    *UserData = Object;
    
    // Set metatable for the object
    CString ClassName = Object->GetClass()->GetName();
    if (luaL_newmetatable(LuaState, *ClassName))
    {
        SetupObjectMetatable(ClassName);
    }
    lua_setmetatable(LuaState, -2);
    
    // Set as global
    lua_setglobal(LuaState, *Name);
}

void NLuaContext::UnbindObject(const CString& Name)
{
    if (!LuaState) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    BoundObjects.Remove(Name);
    lua_pushnil(LuaState);
    lua_setglobal(LuaState, *Name);
}

void NLuaContext::BindFunction(const CString& Name, const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Function)
{
    if (!LuaState) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    BoundFunctions.Add(Name, Function);
    
    // Create C closure
    lua_pushlstring(LuaState, *Name, Name.Len());
    lua_pushcclosure(LuaState, &NLuaContext::LuaFunctionCallback, 1);
    lua_setglobal(LuaState, *Name);
}

void NLuaContext::UnbindFunction(const CString& Name)
{
    if (!LuaState) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    BoundFunctions.Remove(Name);
    lua_pushnil(LuaState);
    lua_setglobal(LuaState, *Name);
}

bool NLuaContext::LoadModule(const CString& ModuleName, const CString& ModulePath)
{
    if (!LuaState) return false;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    if (!NFileSystem::FileExists(ModulePath))
    {
        return false;
    }
    
    int Result = luaL_dofile(LuaState, *ModulePath);
    if (Result == LUA_OK)
    {
        LoadedModules.AddUnique(ModuleName);
        return true;
    }
    
    return false;
}

bool NLuaContext::UnloadModule(const CString& ModuleName)
{
    if (!LuaState) return false;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    LoadedModules.Remove(ModuleName);
    
    // Remove from package.loaded
    lua_getglobal(LuaState, "package");
    lua_getfield(LuaState, -1, "loaded");
    lua_pushnil(LuaState);
    lua_setfield(LuaState, -2, *ModuleName);
    lua_pop(LuaState, 2);
    
    return true;
}

CArray<CString> NLuaContext::GetLoadedModules() const
{
    CLockGuard<NMutex> Lock(ContextMutex);
    return LoadedModules;
}

NScriptResult NLuaContext::Execute(const CString& Code)
{
    if (!LuaState)
    {
        return NScriptResult::Error("Lua state not initialized");
    }
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    int LoadResult = luaL_loadstring(LuaState, *Code);
    if (LoadResult != LUA_OK)
    {
        CString ErrorMsg = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);
        return NScriptResult::Error(ErrorMsg);
    }
    
    int ExecResult = lua_pcall(LuaState, 0, LUA_MULTRET, 0);
    if (ExecResult != LUA_OK)
    {
        CString ErrorMsg = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);
        return NScriptResult::Error(ErrorMsg);
    }
    
    // Get return value
    CScriptValue ReturnValue;
    if (lua_gettop(LuaState) > 0)
    {
        ReturnValue = PopScriptValue();
    }
    
    return NScriptResult::Success(ReturnValue);
}

NScriptResult NLuaContext::ExecuteFile(const CString& FilePath)
{
    if (!LuaState)
    {
        return NScriptResult::Error("Lua state not initialized");
    }
    
    if (!NFileSystem::FileExists(FilePath))
    {
        return NScriptResult::Error("File does not exist: " + FilePath);
    }
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    int Result = luaL_dofile(LuaState, *FilePath);
    if (Result != LUA_OK)
    {
        CString ErrorMsg = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);
        return NScriptResult::Error(ErrorMsg);
    }
    
    return NScriptResult::Success();
}

NScriptResult NLuaContext::CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args)
{
    if (!LuaState)
    {
        return NScriptResult::Error("Lua state not initialized");
    }
    
    CLockGuard<NMutex> Lock(ContextMutex);
    
    lua_getglobal(LuaState, *FunctionName);
    if (!lua_isfunction(LuaState, -1))
    {
        lua_pop(LuaState, 1);
        return NScriptResult::Error("Function not found: " + FunctionName);
    }
    
    // Push arguments
    for (const auto& Arg : Args)
    {
        PushScriptValue(Arg);
    }
    
    int Result = lua_pcall(LuaState, Args.Num(), 1, 0);
    if (Result != LUA_OK)
    {
        CString ErrorMsg = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);
        return NScriptResult::Error(ErrorMsg);
    }
    
    CScriptValue ReturnValue = PopScriptValue();
    return NScriptResult::Success(ReturnValue);
}

void NLuaContext::SetBreakpoint(const CString& FilePath, int32_t Line)
{
    // Lua debugging support would be implemented here
}

void NLuaContext::RemoveBreakpoint(const CString& FilePath, int32_t Line)
{
    // Lua debugging support would be implemented here
}

void NLuaContext::SetDebugMode(bool bEnabled)
{
    bDebugMode = bEnabled;
}

void NLuaContext::CollectGarbage()
{
    if (!LuaState) return;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    lua_gc(LuaState, LUA_GCCOLLECT, 0);
}

size_t NLuaContext::GetMemoryUsage() const
{
    if (!LuaState) return 0;
    
    CLockGuard<NMutex> Lock(ContextMutex);
    return lua_gc(LuaState, LUA_GCCOUNT, 0) * 1024 + lua_gc(LuaState, LUA_GCCOUNTB, 0);
}

void NLuaContext::PushScriptValue(const CScriptValue& Value)
{
    switch (Value.GetType())
    {
        case EScriptValueType::Null:
            lua_pushnil(LuaState);
            break;
        case EScriptValueType::Boolean:
            lua_pushboolean(LuaState, Value.ToBool());
            break;
        case EScriptValueType::Integer:
            lua_pushinteger(LuaState, Value.ToInt());
            break;
        case EScriptValueType::Float:
            lua_pushnumber(LuaState, Value.ToFloat());
            break;
        case EScriptValueType::String:
            {
                CString Str = Value.ToString();
                lua_pushlstring(LuaState, *Str, Str.Len());
            }
            break;
        case EScriptValueType::Object:
            // Push object userdata
            if (Value.GetObjectValue())
            {
                void** UserData = static_cast<void**>(lua_newuserdata(LuaState, sizeof(void*)));
                *UserData = Value.GetObjectValue();
                
                CString ClassName = Value.GetObjectValue()->GetClass()->GetName();
                luaL_getmetatable(LuaState, *ClassName);
                lua_setmetatable(LuaState, -2);
            }
            else
            {
                lua_pushnil(LuaState);
            }
            break;
        default:
            lua_pushnil(LuaState);
            break;
    }
}

CScriptValue NLuaContext::PopScriptValue()
{
    if (lua_gettop(LuaState) == 0)
    {
        return CScriptValue();
    }
    
    int Type = lua_type(LuaState, -1);
    CScriptValue Result;
    
    switch (Type)
    {
        case LUA_TNIL:
            Result = CScriptValue();
            break;
        case LUA_TBOOLEAN:
            Result = CScriptValue(lua_toboolean(LuaState, -1) != 0);
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(LuaState, -1))
            {
                Result = CScriptValue(lua_tointeger(LuaState, -1));
            }
            else
            {
                Result = CScriptValue(lua_tonumber(LuaState, -1));
            }
            break;
        case LUA_TSTRING:
            {
                size_t Len;
                const char* Str = lua_tolstring(LuaState, -1, &Len);
                Result = CScriptValue(CString(Str, Len));
            }
            break;
        case LUA_TUSERDATA:
            {
                void** UserData = static_cast<void**>(lua_touserdata(LuaState, -1));
                if (UserData && *UserData)
                {
                    Result = CScriptValue(static_cast<CObject*>(*UserData));
                }
            }
            break;
        default:
            Result = CScriptValue();
            break;
    }
    
    lua_pop(LuaState, 1);
    return Result;
}

void NLuaContext::SetupErrorHandling()
{
    // Set up panic function
    lua_atpanic(LuaState, &NLuaContext::LuaPanicHandler);
}

void NLuaContext::SetupNLibGlobals()
{
    // Create NLib global table
    lua_newtable(LuaState);
    lua_setglobal(LuaState, "NLib");
}

void NLuaContext::SetupObjectMetatable(const CString& ClassName)
{
    // __index metamethod
    lua_pushstring(LuaState, "__index");
    lua_pushcclosure(LuaState, &NLuaContext::LuaObjectIndex, 0);
    lua_settable(LuaState, -3);
    
    // __newindex metamethod
    lua_pushstring(LuaState, "__newindex");
    lua_pushcclosure(LuaState, &NLuaContext::LuaObjectNewIndex, 0);
    lua_settable(LuaState, -3);
    
    // __gc metamethod
    lua_pushstring(LuaState, "__gc");
    lua_pushcclosure(LuaState, &NLuaContext::LuaObjectGC, 0);
    lua_settable(LuaState, -3);
}

int NLuaContext::LuaFunctionCallback(lua_State* L)
{
    // Get function name from upvalue
    const char* FunctionName = lua_tostring(L, lua_upvalueindex(1));
    
    // Find the context (simplified - would need proper context management)
    // For now, just return 0
    return 0;
}

int NLuaContext::LuaObjectIndex(lua_State* L)
{
    // Object property getter
    return 0;
}

int NLuaContext::LuaObjectNewIndex(lua_State* L)
{
    // Object property setter
    return 0;
}

int NLuaContext::LuaObjectGC(lua_State* L)
{
    // Object garbage collection
    return 0;
}

int NLuaContext::LuaPanicHandler(lua_State* L)
{
    const char* ErrorMsg = lua_tostring(L, -1);
    CLogger::Fatal("Lua panic: %s", ErrorMsg ? ErrorMsg : "Unknown error");
    return 0;
}

// NLuaEngine Implementation
NLuaEngine::NLuaEngine()
    : bInitialized(false)
    , bHotReloadEnabled(false)
{
}

NLuaEngine::~NLuaEngine()
{
    Shutdown();
}

CString NLuaEngine::GetVersion() const
{
    return CString::Printf("Lua %s", LUA_VERSION);
}

bool NLuaEngine::Initialize()
{
    if (bInitialized)
    {
        return true;
    }
    
    CLogger::Info("Initializing Lua engine...");
    
    // Create main context
    MainContext = MakeShared<NLuaContext>();
    if (!MainContext.IsValid() || !MainContext->GetLuaState())
    {
        CLogger::Error("Failed to create Lua main context");
        return false;
    }
    
    bInitialized = true;
    CLogger::Info("Lua engine initialized successfully");
    return true;
}

void NLuaEngine::Shutdown()
{
    if (!bInitialized)
    {
        return;
    }
    
    CLogger::Info("Shutting down Lua engine...");
    
    if (bHotReloadEnabled)
    {
        DisableHotReload();
    }
    
    // Clean up contexts
    CreatedContexts.Empty();
    MainContext.Reset();
    
    bInitialized = false;
    CLogger::Info("Lua engine shutdown complete");
}

TSharedPtr<IScriptContext> NLuaEngine::CreateContext()
{
    if (!bInitialized)
    {
        return nullptr;
    }
    
    auto Context = MakeShared<NLuaContext>();
    if (Context.IsValid())
    {
        CreatedContexts.Add(Context);
    }
    
    return Context;
}

void NLuaEngine::DestroyContext(TSharedPtr<IScriptContext> Context)
{
    for (int32_t i = CreatedContexts.Num() - 1; i >= 0; --i)
    {
        if (CreatedContexts[i].Pin() == Context)
        {
            CreatedContexts.RemoveAt(i);
            break;
        }
    }
}

TSharedPtr<IScriptContext> NLuaEngine::GetMainContext()
{
    return MainContext;
}

bool NLuaEngine::RegisterClass(const CString& ClassName)
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    if (RegisteredClasses.Contains(ClassName))
    {
        return false;
    }
    
    RegisteredClasses.Add(ClassName);
    return true;
}

bool NLuaEngine::UnregisterClass(const CString& ClassName)
{
    CLockGuard<NMutex> Lock(EngineMutex);
    return RegisteredClasses.Remove(ClassName) > 0;
}

bool NLuaEngine::IsClassRegistered(const CString& ClassName) const
{
    CLockGuard<NMutex> Lock(EngineMutex);
    return RegisteredClasses.Contains(ClassName);
}

bool NLuaEngine::AutoBindClasses()
{
    if (!bInitialized)
    {
        return false;
    }
    
    CLogger::Info("Auto-binding Lua classes...");
    
    // Get all script accessible classes from meta registry
    auto& MetaRegistry = NScriptMetaRegistry::Get();
    auto ClassNames = MetaRegistry.GetClassesForLanguage(EScriptLanguage::Lua);
    
    bool bSuccess = true;
    for (const CString& ClassName : ClassNames)
    {
        if (!AutoBindClass(ClassName))
        {
            CLogger::Warning("Failed to auto-bind class: %s", *ClassName);
            bSuccess = false;
        }
    }
    
    return bSuccess;
}

bool NLuaEngine::AutoBindClass(const CString& ClassName)
{
    if (!RegisterClass(ClassName))
    {
        return false;
    }
    
    // Bind the class to the main context
    if (MainContext.IsValid())
    {
        return BindLuaClass(ClassName);
    }
    
    return false;
}

bool NLuaEngine::EnableHotReload(const CString& WatchDirectory)
{
    // Hot reload implementation would go here
    bHotReloadEnabled = true;
    this->WatchDirectory = WatchDirectory;
    return true;
}

void NLuaEngine::DisableHotReload()
{
    bHotReloadEnabled = false;
    WatchDirectory.Empty();
}

void NLuaEngine::ResetStatistics()
{
    CLockGuard<NMutex> Lock(StatsMutex);
    Statistics.Empty();
}

CHashMap<CString, double> NLuaEngine::GetStatistics() const
{
    CLockGuard<NMutex> Lock(StatsMutex);
    
    CHashMap<CString, double> Stats = Statistics;
    
    // Add runtime statistics
    Stats.Add("Memory.Usage", MainContext.IsValid() ? MainContext->GetMemoryUsage() : 0.0);
    Stats.Add("Contexts.Created", static_cast<double>(CreatedContexts.Num()));
    Stats.Add("Classes.Registered", static_cast<double>(RegisteredClasses.Num()));
    
    return Stats;
}

bool NLuaEngine::BindLuaClass(const CString& ClassName)
{
    // Get class metadata
    auto& MetaRegistry = NScriptMetaRegistry::Get();
    const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
    
    if (!ClassMeta)
    {
        return false;
    }
    
    // Generate and execute Lua binding code
    CString BindingCode = GenerateClassBinding(ClassName, *ClassMeta);
    
    if (MainContext.IsValid())
    {
        auto Result = MainContext->Execute(BindingCode);
        return Result.IsSuccess();
    }
    
    return false;
}

CString NLuaEngine::GenerateClassBinding(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    // Create class table
    Code += CString::Printf("NLib.%s = {}\n", *ClassName);
    Code += CString::Printf("NLib.%s.__index = NLib.%s\n", *ClassName, *ClassName);
    
    // Constructor
    Code += CString::Printf("function NLib.%s.New(...)\n", *ClassName);
    Code += CString::Printf("    local obj = {}\n");
    Code += CString::Printf("    setmetatable(obj, NLib.%s)\n", *ClassName);
    Code += CString::Printf("    return obj\n");
    Code += CString::Printf("end\n\n");
    
    // Properties
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        if (PropMeta.IsReadable())
        {
            Code += CString::Printf("function NLib.%s:Get%s()\n", *ClassName, *PropName);
            Code += CString::Printf("    -- Property getter implementation\n");
            Code += CString::Printf("    return nil\n");
            Code += CString::Printf("end\n\n");
        }
        
        if (PropMeta.IsWritable())
        {
            Code += CString::Printf("function NLib.%s:Set%s(value)\n", *ClassName, *PropName);
            Code += CString::Printf("    -- Property setter implementation\n");
            Code += CString::Printf("end\n\n");
        }
    }
    
    // Functions
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += CString::Printf("function NLib.%s:%s(...)\n", *ClassName, *FuncName);
        Code += CString::Printf("    -- Function implementation\n");
        Code += CString::Printf("    return nil\n");
        Code += CString::Printf("end\n\n");
    }
    
    return Code;
}

} // namespace NLib