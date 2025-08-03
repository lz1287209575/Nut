#include "Scripting/NScriptMeta.h"
#include "Core/NClass.h"
#include "Memory/CAllocator.h"
#include "Threading/NMutex.h"
#include "Core/CLogger.h"

namespace NLib
{

// NScriptMetaInfo Implementation
NScriptMetaInfo::NScriptMetaInfo()
    : Languages(EScriptLanguage::None)
    , bSingleton(false)
    , bAbstract(false)
{
}

bool NScriptMetaInfo::HasLanguage(EScriptLanguage Language) const
{
    return EnumHasAnyFlags(Languages, Language);
}

void NScriptMetaInfo::AddLanguage(EScriptLanguage Language)
{
    Languages |= Language;
}

void NScriptMetaInfo::RemoveLanguage(EScriptLanguage Language)
{
    Languages &= ~Language;
}

CArray<EScriptLanguage> NScriptMetaInfo::GetSupportedLanguages() const
{
    CArray<EScriptLanguage> Result;
    
    if (EnumHasAnyFlags(Languages, EScriptLanguage::Lua))        Result.Add(EScriptLanguage::Lua);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::LuaClass))   Result.Add(EScriptLanguage::LuaClass);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::Python))     Result.Add(EScriptLanguage::Python);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::JavaScript)) Result.Add(EScriptLanguage::JavaScript);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::TypeScript)) Result.Add(EScriptLanguage::TypeScript);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::CSharp))     Result.Add(EScriptLanguage::CSharp);
    if (EnumHasAnyFlags(Languages, EScriptLanguage::NBP))        Result.Add(EScriptLanguage::NBP);
    
    return Result;
}

// NScriptPropertyMeta Implementation
NScriptPropertyMeta::NScriptPropertyMeta()
    : Access(EScriptPropertyAccess::None)
    , bReadOnly(false)
    , bTransient(false)
    , MinValue(0.0)
    , MaxValue(0.0)
    , bHasMinMax(false)
{
}

bool NScriptPropertyMeta::IsReadable() const
{
    return EnumHasAnyFlags(Access, EScriptPropertyAccess::Read);
}

bool NScriptPropertyMeta::IsWritable() const
{
    return EnumHasAnyFlags(Access, EScriptPropertyAccess::Write) && !bReadOnly;
}

bool NScriptPropertyMeta::HasValidator() const
{
    return !ValidatorFunction.IsEmpty();
}

// NScriptFunctionMeta Implementation
NScriptFunctionMeta::NScriptFunctionMeta()
    : bPure(false)
    , bAsync(false)
    , bStatic(false)
{
}

int32_t NScriptFunctionMeta::GetParameterCount() const
{
    return ParamNames.Num();
}

CString NScriptFunctionMeta::GetParameterName(int32_t Index) const
{
    if (Index >= 0 && Index < ParamNames.Num())
    {
        return ParamNames[Index];
    }
    return CString();
}

CString NScriptFunctionMeta::GetParameterDescription(int32_t Index) const
{
    if (Index >= 0 && Index < ParamDescriptions.Num())
    {
        return ParamDescriptions[Index];
    }
    return CString();
}

CString NScriptFunctionMeta::GetParameterDefault(int32_t Index) const
{
    if (Index >= 0 && Index < ParamDefaults.Num())
    {
        return ParamDefaults[Index];
    }
    return CString();
}

bool NScriptFunctionMeta::HasParameterDefault(int32_t Index) const
{
    return Index >= 0 && Index < ParamDefaults.Num() && !ParamDefaults[Index].IsEmpty();
}

// NScriptClassMeta Implementation
NScriptClassMeta::NScriptClassMeta()
{
}

void NScriptClassMeta::AddProperty(const CString& Name, const NScriptPropertyMeta& Meta)
{
    Properties.Add(Name, Meta);
}

void NScriptClassMeta::AddFunction(const CString& Name, const NScriptFunctionMeta& Meta)
{
    Functions.Add(Name, Meta);
}

const NScriptPropertyMeta* NScriptClassMeta::GetProperty(const CString& Name) const
{
    return Properties.Find(Name);
}

const NScriptFunctionMeta* NScriptClassMeta::GetFunction(const CString& Name) const
{
    return Functions.Find(Name);
}

CArray<CString> NScriptClassMeta::GetPropertyNames() const
{
    CArray<CString> Names;
    for (const auto& Pair : Properties)
    {
        Names.Add(Pair.Key);
    }
    return Names;
}

CArray<CString> NScriptClassMeta::GetFunctionNames() const
{
    CArray<CString> Names;
    for (const auto& Pair : Functions)
    {
        Names.Add(Pair.Key);
    }
    return Names;
}

CArray<CString> NScriptClassMeta::GetReadableProperties() const
{
    CArray<CString> Names;
    for (const auto& Pair : Properties)
    {
        if (Pair.Value.IsReadable())
        {
            Names.Add(Pair.Key);
        }
    }
    return Names;
}

CArray<CString> NScriptClassMeta::GetWritableProperties() const
{
    CArray<CString> Names;
    for (const auto& Pair : Properties)
    {
        if (Pair.Value.IsWritable())
        {
            Names.Add(Pair.Key);
        }
    }
    return Names;
}

CArray<CString> NScriptClassMeta::GetCallableFunctions() const
{
    CArray<CString> Names;
    for (const auto& Pair : Functions)
    {
        Names.Add(Pair.Key);
    }
    return Names;
}

// NScriptMetaRegistry Implementation
NScriptMetaRegistry* NScriptMetaRegistry::Instance = nullptr;
NMutex NScriptMetaRegistry::InstanceMutex;

NScriptMetaRegistry& NScriptMetaRegistry::Get()
{
    if (!Instance)
    {
        CLockGuard<NMutex> Lock(InstanceMutex);
        if (!Instance)
        {
            Instance = CAllocator::New<NScriptMetaRegistry>();
        }
    }
    return *Instance;
}

void NScriptMetaRegistry::Destroy()
{
    CLockGuard<NMutex> Lock(InstanceMutex);
    if (Instance)
    {
        CAllocator::Delete(Instance);
        Instance = nullptr;
    }
}

NScriptMetaRegistry::NScriptMetaRegistry()
{
}

NScriptMetaRegistry::~NScriptMetaRegistry()
{
}

bool NScriptMetaRegistry::RegisterClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    if (Classes.Contains(ClassName))
    {
        CLogger::Warning("Script class already registered: %s", *ClassName);
        return false;
    }
    
    Classes.Add(ClassName, Meta);
    CLogger::Verbose("Registered script class: %s", *ClassName);
    return true;
}

bool NScriptMetaRegistry::UnregisterClass(const CString& ClassName)
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    if (!Classes.Contains(ClassName))
    {
        return false;
    }
    
    Classes.Remove(ClassName);
    CLogger::Verbose("Unregistered script class: %s", *ClassName);
    return true;
}

const NScriptClassMeta* NScriptMetaRegistry::GetClassMeta(const CString& ClassName) const
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    return Classes.Find(ClassName);
}

bool NScriptMetaRegistry::IsClassRegistered(const CString& ClassName) const
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    return Classes.Contains(ClassName);
}

CArray<CString> NScriptMetaRegistry::GetRegisteredClasses() const
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    CArray<CString> Names;
    for (const auto& Pair : Classes)
    {
        Names.Add(Pair.Key);
    }
    return Names;
}

CArray<CString> NScriptMetaRegistry::GetClassesForLanguage(EScriptLanguage Language) const
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    CArray<CString> Names;
    for (const auto& Pair : Classes)
    {
        if (Pair.Value.MetaInfo.HasLanguage(Language))
        {
            Names.Add(Pair.Key);
        }
    }
    return Names;
}

CArray<CString> NScriptMetaRegistry::GetCreatableClasses() const
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    CArray<CString> Names;
    for (const auto& Pair : Classes)
    {
        if (!Pair.Value.MetaInfo.bAbstract)
        {
            Names.Add(Pair.Key);
        }
    }
    return Names;
}

void NScriptMetaRegistry::Clear()
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    Classes.Empty();
}

bool NScriptMetaRegistry::AutoRegisterClasses()
{
    CLockGuard<NMutex> Lock(RegistryMutex);
    
    bool bSuccess = true;
    
    // This would normally iterate through the NCLASS system
    // For now, we'll just log that auto-registration is available
    CLogger::Info("Auto-registering script accessible classes...");
    
    // TODO: Integrate with NCLASS reflection system to automatically
    // discover classes with script metadata and register them
    
    CLogger::Info("Auto-registration complete. %d classes registered.", Classes.Num());
    return bSuccess;
}

// Utility functions for metadata parsing
NScriptMetaInfo ParseScriptMetaInfo(const CString& MetaString)
{
    NScriptMetaInfo Info;
    
    // Simple parser for metadata strings like "ScriptCreatable,Languages=Lua|Python,Category=Gameplay"
    CArray<CString> Parts;
    MetaString.Split(",", Parts);
    
    for (const CString& Part : Parts)
    {
        CString TrimmedPart = Part.Trim();
        
        if (TrimmedPart == "ScriptCreatable")
        {
            Info.AddLanguage(EScriptLanguage::All);
        }
        else if (TrimmedPart == "Singleton")
        {
            Info.bSingleton = true;
        }
        else if (TrimmedPart == "Abstract")
        {
            Info.bAbstract = true;
        }
        else if (TrimmedPart.StartsWith("ScriptName="))
        {
            Info.ScriptName = TrimmedPart.Mid(11).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("Category="))
        {
            Info.Category = TrimmedPart.Mid(9).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("Description="))
        {
            Info.Description = TrimmedPart.Mid(12).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("Languages="))
        {
            CString LanguageStr = TrimmedPart.Mid(10);
            CArray<CString> Languages;
            LanguageStr.Split("|", Languages);
            
            Info.Languages = EScriptLanguage::None;
            for (const CString& Lang : Languages)
            {
                EScriptLanguage Language = StringToEnum(Lang.Trim());
                if (Language != EScriptLanguage::None)
                {
                    Info.AddLanguage(Language);
                }
            }
        }
    }
    
    return Info;
}

NScriptPropertyMeta ParsePropertyMeta(const CString& MetaString)
{
    NScriptPropertyMeta Meta;
    
    CArray<CString> Parts;
    MetaString.Split(",", Parts);
    
    for (const CString& Part : Parts)
    {
        CString TrimmedPart = Part.Trim();
        
        if (TrimmedPart == "ScriptReadable")
        {
            Meta.Access |= EScriptPropertyAccess::Read;
        }
        else if (TrimmedPart == "ScriptWritable")
        {
            Meta.Access |= EScriptPropertyAccess::Write;
        }
        else if (TrimmedPart == "ScriptReadWrite")
        {
            Meta.Access = EScriptPropertyAccess::ReadWrite;
        }
        else if (TrimmedPart == "ReadOnly")
        {
            Meta.bReadOnly = true;
        }
        else if (TrimmedPart == "Transient")
        {
            Meta.bTransient = true;
        }
        else if (TrimmedPart.StartsWith("Description="))
        {
            Meta.Description = TrimmedPart.Mid(12).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("DefaultValue="))
        {
            Meta.DefaultValue = TrimmedPart.Mid(13).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("Validator="))
        {
            Meta.ValidatorFunction = TrimmedPart.Mid(10).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("Min="))
        {
            Meta.MinValue = TrimmedPart.Mid(4).ToDouble();
            Meta.bHasMinMax = true;
        }
        else if (TrimmedPart.StartsWith("Max="))
        {
            Meta.MaxValue = TrimmedPart.Mid(4).ToDouble();
            Meta.bHasMinMax = true;
        }
    }
    
    return Meta;
}

NScriptFunctionMeta ParseFunctionMeta(const CString& MetaString)
{
    NScriptFunctionMeta Meta;
    
    CArray<CString> Parts;
    MetaString.Split(",", Parts);
    
    for (const CString& Part : Parts)
    {
        CString TrimmedPart = Part.Trim();
        
        if (TrimmedPart == "ScriptCallable")
        {
            // Function is callable by default when this meta is present
        }
        else if (TrimmedPart == "Pure")
        {
            Meta.bPure = true;
        }
        else if (TrimmedPart == "Async")
        {
            Meta.bAsync = true;
        }
        else if (TrimmedPart == "Static")
        {
            Meta.bStatic = true;
        }
        else if (TrimmedPart.StartsWith("Description="))
        {
            Meta.Description = TrimmedPart.Mid(12).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("ReturnDescription="))
        {
            Meta.ReturnDescription = TrimmedPart.Mid(18).Trim("\"");
        }
        else if (TrimmedPart.StartsWith("ParamNames="))
        {
            CString ParamStr = TrimmedPart.Mid(11).Trim("\"");
            ParamStr.Split(",", Meta.ParamNames);
            for (CString& Name : Meta.ParamNames)
            {
                Name = Name.Trim();
            }
        }
        else if (TrimmedPart.StartsWith("ParamDescriptions="))
        {
            CString ParamStr = TrimmedPart.Mid(18).Trim("\"");
            ParamStr.Split(",", Meta.ParamDescriptions);
            for (CString& Desc : Meta.ParamDescriptions)
            {
                Desc = Desc.Trim();
            }
        }
        else if (TrimmedPart.StartsWith("ParamDefaults="))
        {
            CString ParamStr = TrimmedPart.Mid(14).Trim("\"");
            ParamStr.Split(",", Meta.ParamDefaults);
            for (CString& Default : Meta.ParamDefaults)
            {
                Default = Default.Trim();
            }
        }
    }
    
    return Meta;
}

} // namespace NLib