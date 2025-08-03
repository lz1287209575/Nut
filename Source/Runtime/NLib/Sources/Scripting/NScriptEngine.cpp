#include "Scripting/NScriptEngine.h"
#include "Scripting/NScriptMeta.h"
#include "Memory/CAllocator.h"
#include "Containers/CString.h"
#include "Threading/NMutex.h"

namespace NLib
{

// CScriptValue Implementation
CScriptValue::CScriptValue()
    : Type(EScriptValueType::Null)
{
}

CScriptValue::CScriptValue(bool Value)
    : Type(EScriptValueType::Boolean)
    , BoolValue(Value)
{
}

CScriptValue::CScriptValue(int32_t Value)
    : Type(EScriptValueType::Integer)
    , IntValue(Value)
{
}

CScriptValue::CScriptValue(int64_t Value)
    : Type(EScriptValueType::Integer)
    , IntValue(Value)
{
}

CScriptValue::CScriptValue(float Value)
    : Type(EScriptValueType::Float)
    , FloatValue(Value)
{
}

CScriptValue::CScriptValue(double Value)
    : Type(EScriptValueType::Float)
    , FloatValue(Value)
{
}

CScriptValue::CScriptValue(const char* Value)
    : Type(EScriptValueType::String)
{
    new(&StringValue) CString(Value);
}

CScriptValue::CScriptValue(const CString& Value)
    : Type(EScriptValueType::String)
{
    new(&StringValue) CString(Value);
}

CScriptValue::CScriptValue(CObject* Value)
    : Type(EScriptValueType::Object)
    , ObjectValue(Value)
{
}

CScriptValue::CScriptValue(const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Value)
    : Type(EScriptValueType::Function)
{
    new(&FunctionValue) NFunction<CScriptValue(const CArray<CScriptValue>&)>(Value);
}

CScriptValue::CScriptValue(const CArray<CScriptValue>& Value)
    : Type(EScriptValueType::Array)
{
    new(&ArrayValue) CArray<CScriptValue>(Value);
}

CScriptValue::CScriptValue(const CHashMap<CString, CScriptValue>& Value)
    : Type(EScriptValueType::Map)
{
    new(&MapValue) CHashMap<CString, CScriptValue>(Value);
}

CScriptValue::CScriptValue(const CScriptValue& Other)
{
    CopyFrom(Other);
}

CScriptValue::CScriptValue(CScriptValue&& Other) noexcept
{
    MoveFrom(MoveTemp(Other));
}

CScriptValue::~CScriptValue()
{
    Cleanup();
}

CScriptValue& CScriptValue::operator=(const CScriptValue& Other)
{
    if (this != &Other)
    {
        Cleanup();
        CopyFrom(Other);
    }
    return *this;
}

CScriptValue& CScriptValue::operator=(CScriptValue&& Other) noexcept
{
    if (this != &Other)
    {
        Cleanup();
        MoveFrom(MoveTemp(Other));
    }
    return *this;
}

void CScriptValue::CopyFrom(const CScriptValue& Other)
{
    Type = Other.Type;
    
    switch (Type)
    {
        case EScriptValueType::Boolean:
            BoolValue = Other.BoolValue;
            break;
        case EScriptValueType::Integer:
            IntValue = Other.IntValue;
            break;
        case EScriptValueType::Float:
            FloatValue = Other.FloatValue;
            break;
        case EScriptValueType::String:
            new(&StringValue) CString(Other.StringValue);
            break;
        case EScriptValueType::Object:
            ObjectValue = Other.ObjectValue;
            break;
        case EScriptValueType::Function:
            new(&FunctionValue) NFunction<CScriptValue(const CArray<CScriptValue>&)>(Other.FunctionValue);
            break;
        case EScriptValueType::Array:
            new(&ArrayValue) CArray<CScriptValue>(Other.ArrayValue);
            break;
        case EScriptValueType::Map:
            new(&MapValue) CHashMap<CString, CScriptValue>(Other.MapValue);
            break;
        default:
            break;
    }
}

void CScriptValue::MoveFrom(CScriptValue&& Other)
{
    Type = Other.Type;
    
    switch (Type)
    {
        case EScriptValueType::Boolean:
            BoolValue = Other.BoolValue;
            break;
        case EScriptValueType::Integer:
            IntValue = Other.IntValue;
            break;
        case EScriptValueType::Float:
            FloatValue = Other.FloatValue;
            break;
        case EScriptValueType::String:
            new(&StringValue) CString(MoveTemp(Other.StringValue));
            break;
        case EScriptValueType::Object:
            ObjectValue = Other.ObjectValue;
            Other.ObjectValue = nullptr;
            break;
        case EScriptValueType::Function:
            new(&FunctionValue) NFunction<CScriptValue(const CArray<CScriptValue>&)>(MoveTemp(Other.FunctionValue));
            break;
        case EScriptValueType::Array:
            new(&ArrayValue) CArray<CScriptValue>(MoveTemp(Other.ArrayValue));
            break;
        case EScriptValueType::Map:
            new(&MapValue) CHashMap<CString, CScriptValue>(MoveTemp(Other.MapValue));
            break;
        default:
            break;
    }
    
    Other.Type = EScriptValueType::Null;
}

void CScriptValue::Cleanup()
{
    switch (Type)
    {
        case EScriptValueType::String:
            StringValue.~CString();
            break;
        case EScriptValueType::Function:
            FunctionValue.~NFunction();
            break;
        case EScriptValueType::Array:
            ArrayValue.~CArray();
            break;
        case EScriptValueType::Map:
            MapValue.~CHashMap();
            break;
        default:
            break;
    }
    Type = EScriptValueType::Null;
}

bool CScriptValue::ToBool() const
{
    switch (Type)
    {
        case EScriptValueType::Boolean:
            return BoolValue;
        case EScriptValueType::Integer:
            return IntValue != 0;
        case EScriptValueType::Float:
            return FloatValue != 0.0;
        case EScriptValueType::String:
            return !StringValue.IsEmpty();
        case EScriptValueType::Object:
            return ObjectValue != nullptr;
        case EScriptValueType::Array:
            return !ArrayValue.IsEmpty();
        case EScriptValueType::Map:
            return !MapValue.IsEmpty();
        default:
            return false;
    }
}

int64_t CScriptValue::ToInt() const
{
    switch (Type)
    {
        case EScriptValueType::Boolean:
            return BoolValue ? 1 : 0;
        case EScriptValueType::Integer:
            return IntValue;
        case EScriptValueType::Float:
            return static_cast<int64_t>(FloatValue);
        case EScriptValueType::String:
            return StringValue.ToInt64();
        default:
            return 0;
    }
}

double CScriptValue::ToFloat() const
{
    switch (Type)
    {
        case EScriptValueType::Boolean:
            return BoolValue ? 1.0 : 0.0;
        case EScriptValueType::Integer:
            return static_cast<double>(IntValue);
        case EScriptValueType::Float:
            return FloatValue;
        case EScriptValueType::String:
            return StringValue.ToDouble();
        default:
            return 0.0;
    }
}

CString CScriptValue::ToString() const
{
    switch (Type)
    {
        case EScriptValueType::Null:
            return "null";
        case EScriptValueType::Boolean:
            return BoolValue ? "true" : "false";
        case EScriptValueType::Integer:
            return CString::FromInt64(IntValue);
        case EScriptValueType::Float:
            return CString::FromDouble(FloatValue);
        case EScriptValueType::String:
            return StringValue;
        case EScriptValueType::Object:
            return ObjectValue ? ObjectValue->GetClass()->GetName() : "null";
        case EScriptValueType::Function:
            return "[Function]";
        case EScriptValueType::Array:
            return CString::Printf("[Array:%d]", ArrayValue.Num());
        case EScriptValueType::Map:
            return CString::Printf("[Map:%d]", MapValue.Num());
        default:
            return "undefined";
    }
}

// NScriptResult Implementation
NScriptResult::NScriptResult()
    : bSuccess(true)
{
}

NScriptResult::NScriptResult(const CScriptValue& InValue)
    : bSuccess(true)
    , Value(InValue)
{
}

NScriptResult::NScriptResult(const CString& InError)
    : bSuccess(false)
    , ErrorMessage(InError)
{
}

NScriptResult NScriptResult::Success(const CScriptValue& Value)
{
    return NScriptResult(Value);
}

NScriptResult NScriptResult::Error(const CString& ErrorMessage)
{
    return NScriptResult(ErrorMessage);
}

// NScriptEngineManager Implementation
NScriptEngineManager* NScriptEngineManager::Instance = nullptr;
NMutex NScriptEngineManager::InstanceMutex;

NScriptEngineManager& NScriptEngineManager::Get()
{
    if (!Instance)
    {
        CLockGuard<NMutex> Lock(InstanceMutex);
        if (!Instance)
        {
            Instance = CAllocator::New<NScriptEngineManager>();
        }
    }
    return *Instance;
}

void NScriptEngineManager::Destroy()
{
    CLockGuard<NMutex> Lock(InstanceMutex);
    if (Instance)
    {
        CAllocator::Delete(Instance);
        Instance = nullptr;
    }
}

NScriptEngineManager::NScriptEngineManager()
{
}

NScriptEngineManager::~NScriptEngineManager()
{
    Shutdown();
}

bool NScriptEngineManager::RegisterEngine(EScriptLanguage Language, TSharedPtr<IScriptEngine> Engine)
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    if (Engines.Contains(Language))
    {
        return false;
    }
    
    Engines.Add(Language, Engine);
    return true;
}

void NScriptEngineManager::UnregisterEngine(EScriptLanguage Language)
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    if (auto EnginePtr = Engines.Find(Language))
    {
        if (EnginePtr->IsValid() && EnginePtr->Get()->IsInitialized())
        {
            EnginePtr->Get()->Shutdown();
        }
        Engines.Remove(Language);
    }
}

TSharedPtr<IScriptEngine> NScriptEngineManager::GetEngine(EScriptLanguage Language) const
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    if (auto EnginePtr = Engines.Find(Language))
    {
        return *EnginePtr;
    }
    
    return nullptr;
}

CArray<EScriptLanguage> NScriptEngineManager::GetRegisteredLanguages() const
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    CArray<EScriptLanguage> Languages;
    for (const auto& Pair : Engines)
    {
        Languages.Add(Pair.Key);
    }
    
    return Languages;
}

bool NScriptEngineManager::IsLanguageSupported(EScriptLanguage Language) const
{
    CLockGuard<NMutex> Lock(EngineMutex);
    return Engines.Contains(Language);
}

void NScriptEngineManager::Shutdown()
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    for (auto& Pair : Engines)
    {
        if (Pair.Value.IsValid() && Pair.Value->IsInitialized())
        {
            Pair.Value->Shutdown();
        }
    }
    
    Engines.Empty();
}

CHashMap<CString, double> NScriptEngineManager::GetAllStatistics() const
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    CHashMap<CString, double> AllStats;
    
    for (const auto& Pair : Engines)
    {
        if (Pair.Value.IsValid())
        {
            auto EngineStats = Pair.Value->GetStatistics();
            CString LanguageName = EnumToString(Pair.Key);
            
            for (const auto& StatPair : EngineStats)
            {
                CString StatName = CString::Printf("%s.%s", *LanguageName, *StatPair.Key);
                AllStats.Add(StatName, StatPair.Value);
            }
        }
    }
    
    return AllStats;
}

void NScriptEngineManager::ResetAllStatistics()
{
    CLockGuard<NMutex> Lock(EngineMutex);
    
    for (auto& Pair : Engines)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->ResetStatistics();
        }
    }
}

// Utility functions
CString EnumToString(EScriptLanguage Language)
{
    switch (Language)
    {
        case EScriptLanguage::Lua:        return "Lua";
        case EScriptLanguage::LuaClass:   return "LuaClass";
        case EScriptLanguage::Python:     return "Python";
        case EScriptLanguage::JavaScript: return "JavaScript";
        case EScriptLanguage::TypeScript: return "TypeScript";
        case EScriptLanguage::CSharp:     return "CSharp";
        case EScriptLanguage::NBP:        return "NBP";
        default:                          return "Unknown";
    }
}

EScriptLanguage StringToEnum(const CString& LanguageString)
{
    if (LanguageString == "Lua")        return EScriptLanguage::Lua;
    if (LanguageString == "LuaClass")   return EScriptLanguage::LuaClass;
    if (LanguageString == "Python")     return EScriptLanguage::Python;
    if (LanguageString == "JavaScript") return EScriptLanguage::JavaScript;
    if (LanguageString == "TypeScript") return EScriptLanguage::TypeScript;
    if (LanguageString == "CSharp")     return EScriptLanguage::CSharp;
    if (LanguageString == "NBP")        return EScriptLanguage::NBP;
    return EScriptLanguage::None;
}

} // namespace NLib