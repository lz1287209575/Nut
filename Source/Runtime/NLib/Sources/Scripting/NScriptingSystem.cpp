#include "Scripting/NScripting.h"
#include "Scripting/NLuaEngine.h"
#include "Scripting/NLuaClassEngine.h"
#include "Scripting/NPythonEngine.h"
#include "Scripting/NTypeScriptEngine.h"
#include "Scripting/NCSharpEngine.h"
#include "Scripting/NBPEngine.h"
#include "Memory/CAllocator.h"
#include "Core/CLogger.h"
#include "FileSystem/NFileSystem.h"
#include "Threading/CThread.h"

namespace NLib
{

// Static member definitions
bool NScriptingSystem::bInitialized = false;
EScriptLanguage NScriptingSystem::InitializedLanguages = EScriptLanguage::None;

bool NScriptingSystem::Initialize(EScriptLanguage Languages)
{
    if (bInitialized)
    {
        CLogger::Warning("Scripting system is already initialized");
        return true;
    }

    CLogger::Info("Initializing NLib Scripting System...");

    auto& Manager = NScriptEngineManager::Get();

    // Initialize Lua engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::Lua))
    {
        auto LuaEngine = MakeShared<NLuaEngine>();
        if (LuaEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::Lua, LuaEngine);
            InitializedLanguages |= EScriptLanguage::Lua;
            CLogger::Info("Lua engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize Lua engine");
        }
    }

    // Initialize LuaClass engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::LuaClass))
    {
        auto LuaClassEngine = MakeShared<NLuaClassEngine>();
        if (LuaClassEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::LuaClass, LuaClassEngine);
            InitializedLanguages |= EScriptLanguage::LuaClass;
            CLogger::Info("LuaClass engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize LuaClass engine");
        }
    }

    // Initialize Python engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::Python))
    {
        auto PythonEngine = MakeShared<NPythonEngine>();
        if (PythonEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::Python, PythonEngine);
            InitializedLanguages |= EScriptLanguage::Python;
            CLogger::Info("Python engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize Python engine");
        }
    }

    // Initialize TypeScript engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::TypeScript))
    {
        auto TypeScriptEngine = MakeShared<NTypeScriptEngine>();
        if (TypeScriptEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::TypeScript, TypeScriptEngine);
            InitializedLanguages |= EScriptLanguage::TypeScript;
            CLogger::Info("TypeScript engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize TypeScript engine");
        }
    }

    // Initialize C# engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::CSharp))
    {
        auto CSharpEngine = MakeShared<NCSharpEngine>();
        if (CSharpEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::CSharp, CSharpEngine);
            InitializedLanguages |= EScriptLanguage::CSharp;
            CLogger::Info("C# engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize C# engine");
        }
    }

    // Initialize NBP engine
    if (EnumHasAnyFlags(Languages, EScriptLanguage::NBP))
    {
        auto NBPEngine = MakeShared<NBP::NBPEngine>();
        if (NBPEngine->Initialize())
        {
            Manager.RegisterEngine(EScriptLanguage::NBP, NBPEngine);
            InitializedLanguages |= EScriptLanguage::NBP;
            CLogger::Info("NBP engine initialized successfully");
        }
        else
        {
            CLogger::Error("Failed to initialize NBP engine");
        }
    }

    bInitialized = InitializedLanguages != EScriptLanguage::None;
    
    if (bInitialized)
    {
        CLogger::Info("Scripting system initialized successfully");
    }
    else
    {
        CLogger::Error("Failed to initialize any script engines");
    }

    return bInitialized;
}

void NScriptingSystem::Shutdown()
{
    if (!bInitialized)
    {
        return;
    }

    CLogger::Info("Shutting down NLib Scripting System...");

    NScriptEngineManager::Get().Shutdown();
    NScriptEngineManager::Destroy();

    bInitialized = false;
    InitializedLanguages = EScriptLanguage::None;

    CLogger::Info("Scripting system shutdown complete");
}

bool NScriptingSystem::IsInitialized()
{
    return bInitialized;
}

bool NScriptingSystem::AutoBindAllClasses()
{
    if (!bInitialized)
    {
        CLogger::Error("Scripting system not initialized");
        return false;
    }

    CLogger::Info("Auto-binding all script accessible classes...");

    bool bSuccess = true;
    auto& Manager = NScriptEngineManager::Get();
    auto Languages = Manager.GetRegisteredLanguages();

    for (EScriptLanguage Language : Languages)
    {
        auto Engine = Manager.GetEngine(Language);
        if (Engine.IsValid())
        {
            if (!Engine->AutoBindClasses())
            {
                CLogger::Warning("Failed to auto-bind classes for %s", *EnumToString(Language));
                bSuccess = false;
            }
        }
    }

    if (bSuccess)
    {
        CLogger::Info("All classes auto-bound successfully");
    }
    else
    {
        CLogger::Warning("Some classes failed to auto-bind");
    }

    return bSuccess;
}

bool NScriptingSystem::ExecuteInitScript(const CString& ScriptPath)
{
    if (!bInitialized)
    {
        CLogger::Error("Scripting system not initialized");
        return false;
    }

    if (!NFileSystem::FileExists(ScriptPath))
    {
        CLogger::Error("Init script file does not exist: %s", *ScriptPath);
        return false;
    }

    // Determine script language from file extension
    CString Extension = NFileSystem::GetFileExtension(ScriptPath).ToLower();
    EScriptLanguage Language = EScriptLanguage::None;

    if (Extension == "lua")
    {
        Language = EScriptLanguage::Lua;
    }
    else if (Extension == "luac")
    {
        Language = EScriptLanguage::LuaClass;
    }
    else if (Extension == "py")
    {
        Language = EScriptLanguage::Python;
    }
    else if (Extension == "ts")
    {
        Language = EScriptLanguage::TypeScript;
    }
    else if (Extension == "js")
    {
        Language = EScriptLanguage::JavaScript;
    }
    else if (Extension == "cs")
    {
        Language = EScriptLanguage::CSharp;
    }
    else if (Extension == "nbp")
    {
        Language = EScriptLanguage::NBP;
    }
    else
    {
        CLogger::Error("Unsupported script file extension: %s", *Extension);
        return false;
    }

    auto& Manager = NScriptEngineManager::Get();
    auto Engine = Manager.GetEngine(Language);
    
    if (!Engine.IsValid())
    {
        CLogger::Error("Script engine not available for language: %s", *EnumToString(Language));
        return false;
    }

    auto Context = Engine->GetMainContext();
    if (!Context.IsValid())
    {
        CLogger::Error("Failed to get main context for %s engine", *EnumToString(Language));
        return false;
    }

    CLogger::Info("Executing init script: %s", *ScriptPath);

    auto Result = Context->ExecuteFile(ScriptPath);
    if (Result.IsSuccess())
    {
        CLogger::Info("Init script executed successfully");
        return true;
    }
    else
    {
        CLogger::Error("Failed to execute init script: %s", *Result.GetErrorMessage());
        return false;
    }
}

bool NScriptingSystem::EnableHotReload(const CString& WatchDirectory)
{
    if (!bInitialized)
    {
        CLogger::Error("Scripting system not initialized");
        return false;
    }

    if (!NFileSystem::DirectoryExists(WatchDirectory))
    {
        CLogger::Error("Watch directory does not exist: %s", *WatchDirectory);
        return false;
    }

    CLogger::Info("Enabling hot reload for directory: %s", *WatchDirectory);

    bool bSuccess = true;
    auto& Manager = NScriptEngineManager::Get();
    auto Languages = Manager.GetRegisteredLanguages();

    for (EScriptLanguage Language : Languages)
    {
        auto Engine = Manager.GetEngine(Language);
        if (Engine.IsValid())
        {
            if (!Engine->EnableHotReload(WatchDirectory))
            {
                CLogger::Warning("Failed to enable hot reload for %s", *EnumToString(Language));
                bSuccess = false;
            }
        }
    }

    if (bSuccess)
    {
        CLogger::Info("Hot reload enabled successfully");
    }
    else
    {
        CLogger::Warning("Hot reload partially enabled");
    }

    return bSuccess;
}

CHashMap<CString, double> NScriptingSystem::GetSystemStatistics()
{
    if (!bInitialized)
    {
        return CHashMap<CString, double>();
    }

    auto& Manager = NScriptEngineManager::Get();
    auto Stats = Manager.GetAllStatistics();

    // Add system-wide statistics
    Stats.Add("System.InitializedEngines", static_cast<double>(Manager.GetRegisteredLanguages().Num()));
    Stats.Add("System.IsInitialized", bInitialized ? 1.0 : 0.0);
    Stats.Add("System.InitializedLanguageFlags", static_cast<double>(static_cast<uint32_t>(InitializedLanguages)));

    return Stats;
}

} // namespace NLib