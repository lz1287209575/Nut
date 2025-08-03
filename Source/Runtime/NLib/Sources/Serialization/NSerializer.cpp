#include "Serialization/NSerializer.h"
#include "Serialization/CArchive.h"
#include "Core/CLogger.h"
#include "Core/CAllocator.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NSerializerFactory 实现 ===

TSharedPtr<CArchive> NSerializerFactory::CreateBinaryArchive(ESerializationMode Mode, const CString& FilePath)
{
    auto Context = CreateContext(ESerializationFormat::Binary, Mode);
    auto Stream = NFile::OpenFile(FilePath, Mode == ESerializationMode::Writing ? EFileMode::Write : EFileMode::Read);
    
    if (!Stream)
    {
        return nullptr;
    }
    
    return NewNObject<NBinaryArchive>(Context, Stream);
}

TSharedPtr<CArchive> NSerializerFactory::CreateJsonArchive(ESerializationMode Mode, const CString& FilePath)
{
    auto Context = CreateContext(ESerializationFormat::JSON, Mode);
    
    if (FilePath.IsEmpty())
    {
        return NewNObject<NJsonArchive>(Context);
    }
    
    if (Mode == ESerializationMode::Reading)
    {
        CString JsonContent = NFile::ReadAllText(FilePath);
        return NewNObject<NJsonArchive>(Context, JsonContent);
    }
    else
    {
        return NewNObject<NJsonArchive>(Context);
    }
}

TSharedPtr<CArchive> NSerializerFactory::CreateMemoryArchive(ESerializationMode Mode, const CArray<uint8_t>& Data)
{
    auto Context = CreateContext(ESerializationFormat::Binary, Mode);
    return NewNObject<NMemoryArchive>(Context, Data);
}

TSharedPtr<CArchive> NSerializerFactory::CreateBinaryArchive(ESerializationMode Mode, TSharedPtr<NFileStream> Stream)
{
    auto Context = CreateContext(ESerializationFormat::Binary, Mode);
    return NewNObject<NBinaryArchive>(Context, Stream);
}

TSharedPtr<CArchive> NSerializerFactory::CreateJsonArchive(ESerializationMode Mode, const CString& JsonString)
{
    auto Context = CreateContext(ESerializationFormat::JSON, Mode);
    return NewNObject<NJsonArchive>(Context, JsonString);
}

TSharedPtr<CArchive> NSerializerFactory::CreateArchiveFromFile(ESerializationMode Mode, const CString& FilePath)
{
    CString Extension = NPath::GetExtension(FilePath).ToLower();
    
    if (Extension == ".json")
    {
        return CreateJsonArchive(Mode, FilePath);
    }
    else if (Extension == ".bin" || Extension == ".dat")
    {
        return CreateBinaryArchive(Mode, FilePath);
    }
    else
    {
        // 默认使用二进制格式
        return CreateBinaryArchive(Mode, FilePath);
    }
}

TSharedPtr<CSerializationContext> NSerializerFactory::CreateContext(ESerializationFormat Format, ESerializationMode Mode)
{
    auto Context = NewNObject<CSerializationContext>();
    Context->SetFormat(Format);
    Context->SetMode(Mode);
    return Context;
}

// === NSerializer 实现 ===

NSerializer::NSerializer()
    : DefaultFormat(ESerializationFormat::Binary)
    , bCompressionEnabled(false)
    , bPrettyPrintEnabled(true)
    , bVersioningEnabled(true)
{
}

NSerializer::~NSerializer()
{
}

bool NSerializer::SerializeObjectToFile(ISerializable* Object, const CString& FilePath, ESerializationFormat Format)
{
    if (!Object)
    {
        AddError("Cannot serialize null object");
        return false;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    auto Archive = CreateArchive(Format, ESerializationMode::Writing, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return false;
    }
    
    bool Success = false;
    try
    {
        if (bVersioningEnabled)
        {
            uint32_t Version = Object->GetSerializationVersion();
            Archive->SerializeVersion("Version", Version);
        }
        
        Object->Serialize(*Archive);
        Success = !Archive->HasErrors();
    }
    catch (...)
    {
        AddError("Exception during serialization");
        Success = false;
    }
    
    UpdateStatistics(Archive.Get(), true, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Success;
}

bool NSerializer::DeserializeObjectFromFile(ISerializable* Object, const CString& FilePath)
{
    if (!Object)
    {
        AddError("Cannot deserialize to null object");
        return false;
    }
    
    if (!NFile::Exists(FilePath))
    {
        AddError("File does not exist: " + FilePath);
        return false;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    ESerializationFormat Format = DetectFormat(FilePath);
    auto Archive = CreateArchive(Format, ESerializationMode::Reading, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return false;
    }
    
    bool Success = false;
    try
    {
        if (bVersioningEnabled)
        {
            uint32_t Version = 1;
            Archive->SerializeVersion("Version", Version);
            
            if (!Object->CanDeserializeVersion(Version))
            {
                AddError(CString::Format("Version {} is not supported by object", Version));
                return false;
            }
        }
        
        Object->Deserialize(*Archive);
        Success = !Archive->HasErrors();
    }
    catch (...)
    {
        AddError("Exception during deserialization");
        Success = false;
    }
    
    UpdateStatistics(Archive.Get(), false, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Success;
}

bool NSerializer::SerializeNObjectToFile(CObject* Object, const CString& FilePath, ESerializationFormat Format)
{
    if (!Object)
    {
        AddError("Cannot serialize null CObject");
        return false;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    auto Archive = CreateArchive(Format, ESerializationMode::Writing, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return false;
    }
    
    bool Success = false;
    try
    {
        TSharedPtr<CObject> SharedObject = Object->AsShared();
        Success = Archive->SerializeNObject("RootObject", SharedObject);
    }
    catch (...)
    {
        AddError("Exception during CObject serialization");
        Success = false;
    }
    
    UpdateStatistics(Archive.Get(), true, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Success;
}

TSharedPtr<CObject> NSerializer::DeserializeNObjectFromFile(const CString& FilePath)
{
    if (!NFile::Exists(FilePath))
    {
        AddError("File does not exist: " + FilePath);
        return nullptr;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    ESerializationFormat Format = DetectFormat(FilePath);
    auto Archive = CreateArchive(Format, ESerializationMode::Reading, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return nullptr;
    }
    
    TSharedPtr<CObject> Object;
    try
    {
        Archive->SerializeNObject("RootObject", Object);
    }
    catch (...)
    {
        AddError("Exception during CObject deserialization");
        Object.Reset();
    }
    
    UpdateStatistics(Archive.Get(), false, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Object;
}

CString NSerializer::SerializeObjectToString(ISerializable* Object, ESerializationFormat Format)
{
    if (!Object)
    {
        AddError("Cannot serialize null object");
        return "";
    }
    
    if (Format == ESerializationFormat::Binary)
    {
        AddError("Binary format not supported for string serialization");
        return "";
    }
    
    auto Archive = CreateArchive(Format, ESerializationMode::Writing);
    if (!Archive)
    {
        AddError("Failed to create archive for string serialization");
        return "";
    }
    
    try
    {
        if (bVersioningEnabled)
        {
            uint32_t Version = Object->GetSerializationVersion();
            Archive->SerializeVersion("Version", Version);
        }
        
        Object->Serialize(*Archive);
        
        if (auto JsonArchive = CObject::Cast<NJsonArchive>(Archive))
        {
            return JsonArchive->ToJsonString(bPrettyPrintEnabled);
        }
    }
    catch (...)
    {
        AddError("Exception during string serialization");
    }
    
    CopyMessages(Archive->GetContext());
    return "";
}

bool NSerializer::DeserializeObjectFromString(ISerializable* Object, const CString& Data, ESerializationFormat Format)
{
    if (!Object)
    {
        AddError("Cannot deserialize to null object");
        return false;
    }
    
    if (Data.IsEmpty())
    {
        AddError("Cannot deserialize from empty string");
        return false;
    }
    
    if (Format == ESerializationFormat::Binary)
    {
        AddError("Binary format not supported for string deserialization");
        return false;
    }
    
    auto Context = CreateContext(Format, ESerializationMode::Reading);
    auto Archive = NewNObject<NJsonArchive>(Context, Data);
    
    bool Success = false;
    try
    {
        if (bVersioningEnabled)
        {
            uint32_t Version = 1;
            Archive->SerializeVersion("Version", Version);
            
            if (!Object->CanDeserializeVersion(Version))
            {
                AddError(CString::Format("Version {} is not supported by object", Version));
                return false;
            }
        }
        
        Object->Deserialize(*Archive);
        Success = !Archive->HasErrors();
    }
    catch (...)
    {
        AddError("Exception during string deserialization");
        Success = false;
    }
    
    CopyMessages(Context.Get());
    return Success;
}

CArray<uint8_t> NSerializer::SerializeObjectToMemory(ISerializable* Object)
{
    if (!Object)
    {
        AddError("Cannot serialize null object");
        return CArray<uint8_t>();
    }
    
    auto Archive = CreateArchive(ESerializationFormat::Binary, ESerializationMode::Writing);
    if (auto MemoryArchive = CObject::Cast<NMemoryArchive>(Archive))
    {
        try
        {
            if (bVersioningEnabled)
            {
                uint32_t Version = Object->GetSerializationVersion();
                Archive->SerializeVersion("Version", Version);
            }
            
            Object->Serialize(*Archive);
            
            if (!Archive->HasErrors())
            {
                return MemoryArchive->GetData();
            }
        }
        catch (...)
        {
            AddError("Exception during memory serialization");
        }
    }
    
    return CArray<uint8_t>();
}

bool NSerializer::DeserializeObjectFromMemory(ISerializable* Object, const CArray<uint8_t>& Data)
{
    if (!Object)
    {
        AddError("Cannot deserialize to null object");
        return false;
    }
    
    if (Data.IsEmpty())
    {
        AddError("Cannot deserialize from empty data");
        return false;
    }
    
    auto Context = CreateContext(ESerializationFormat::Binary, ESerializationMode::Reading);
    auto Archive = NewNObject<NMemoryArchive>(Context, Data);
    
    bool Success = false;
    try
    {
        if (bVersioningEnabled)
        {
            uint32_t Version = 1;
            Archive->SerializeVersion("Version", Version);
            
            if (!Object->CanDeserializeVersion(Version))
            {
                AddError(CString::Format("Version {} is not supported by object", Version));
                return false;
            }
        }
        
        Object->Deserialize(*Archive);
        Success = !Archive->HasErrors();
    }
    catch (...)
    {
        AddError("Exception during memory deserialization");
        Success = false;
    }
    
    CopyMessages(Context.Get());
    return Success;
}

TSharedPtr<ISerializable> NSerializer::CloneSerializableObject(ISerializable* Object)
{
    if (!Object)
    {
        return nullptr;
    }
    
    CArray<uint8_t> Data = SerializeObjectToMemory(Object);
    if (Data.IsEmpty())
    {
        return nullptr;
    }
    
    // 这里需要类型工厂来创建相同类型的对象
    // 简化实现：暂时返回nullptr
    AddWarning("Object cloning requires type factory implementation");
    return nullptr;
}

TSharedPtr<CObject> NSerializer::CloneNObject(CObject* Object)
{
    if (!Object)
    {
        return nullptr;
    }
    
    // 使用反射系统克隆对象
    // 简化实现：暂时返回nullptr
    AddWarning("CObject cloning requires reflection system implementation");
    return nullptr;
}

void NSerializer::ClearMessages()
{
    LastErrors.Clear();
    LastWarnings.Clear();
}

TSharedPtr<CSerializationContext> NSerializer::CreateContext(ESerializationFormat Format, ESerializationMode Mode)
{
    auto Context = NewNObject<CSerializationContext>();
    Context->SetFormat(Format);
    Context->SetMode(Mode);
    return Context;
}

TSharedPtr<CArchive> NSerializer::CreateArchive(ESerializationFormat Format, ESerializationMode Mode, const CString& FilePath)
{
    auto Context = CreateContext(Format, Mode);
    
    switch (Format)
    {
    case ESerializationFormat::Binary:
        if (FilePath.IsEmpty())
        {
            return NewNObject<NMemoryArchive>(Context);
        }
        else
        {
            auto Stream = NFile::OpenFile(FilePath, Mode == ESerializationMode::Writing ? EFileMode::Write : EFileMode::Read);
            if (Stream)
            {
                return NewNObject<NBinaryArchive>(Context, Stream);
            }
        }
        break;
        
    case ESerializationFormat::JSON:
        if (FilePath.IsEmpty())
        {
            return NewNObject<NJsonArchive>(Context);
        }
        else if (Mode == ESerializationMode::Reading)
        {
            CString JsonContent = NFile::ReadAllText(FilePath);
            return NewNObject<NJsonArchive>(Context, JsonContent);
        }
        else
        {
            return NewNObject<NJsonArchive>(Context);
        }
        break;
        
    default:
        AddError("Unsupported serialization format");
        break;
    }
    
    return nullptr;
}

void NSerializer::UpdateStatistics(CArchive* Archive, bool bSerialization, double ElapsedTime)
{
    if (!Archive)
    {
        return;
    }
    
    if (bSerialization)
    {
        Statistics.TotalBytesWritten += Archive->GetBytesProcessed();
        Statistics.ObjectsSerializeed++;
        Statistics.TotalSerializationTime += ElapsedTime;
    }
    else
    {
        Statistics.TotalBytesRead += Archive->GetBytesProcessed();
        Statistics.ObjectsDeserialized++;
        Statistics.TotalDeserializationTime += ElapsedTime;
    }
}

void NSerializer::CopyMessages(CSerializationContext* Context)
{
    if (!Context)
    {
        return;
    }
    
    for (const auto& Error : Context->GetErrors())
    {
        LastErrors.PushBack(Error);
    }
    
    for (const auto& Warning : Context->GetWarnings())
    {
        LastWarnings.PushBack(Warning);
    }
}

ESerializationFormat NSerializer::DetectFormat(const CString& FilePath)
{
    CString Extension = NPath::GetExtension(FilePath).ToLower();
    
    if (Extension == ".json")
    {
        return ESerializationFormat::JSON;
    }
    else if (Extension == ".xml")
    {
        return ESerializationFormat::XML;
    }
    else
    {
        return ESerializationFormat::Binary;
    }
}

void NSerializer::AddError(const CString& Error)
{
    LastErrors.PushBack(Error);
    CLogger::Get().LogError("NSerializer: {}", Error);
}

void NSerializer::AddWarning(const CString& Warning)
{
    LastWarnings.PushBack(Warning);
    CLogger::Get().LogWarning("NSerializer: {}", Warning);
}

// === NSerializer::SerializationStats 实现 ===

CString NSerializer::SerializationStats::ToString() const
{
    return CString::Format(
        "Serialization Statistics:\n"
        "  Bytes Written: {}\n"
        "  Bytes Read: {}\n"
        "  Objects Serialized: {}\n"
        "  Objects Deserialized: {}\n"
        "  Serialization Time: {:.3f}s\n"
        "  Deserialization Time: {:.3f}s",
        TotalBytesWritten,
        TotalBytesRead,
        ObjectsSerializeed,
        ObjectsDeserialized,
        TotalSerializationTime,
        TotalDeserializationTime
    );
}

// === NSerializationHelper 实现 ===

static NSerializer GlobalSerializer;

NSerializer& NSerializationHelper::GetGlobalSerializer()
{
    return GlobalSerializer;
}

void NSerializationHelper::RegisterType(const CString& TypeName, const std::type_info& TypeInfo)
{
    // 实际实现需要全局类型注册表
    CLogger::Get().LogInfo("Registered serialization type: {}", TypeName);
}

// === NVersionedSerializer 实现 ===

NVersionedSerializer::NVersionedSerializer()
    : CurrentVersion(1)
{
}

NVersionedSerializer::~NVersionedSerializer()
{
}

void NVersionedSerializer::RegisterMigration(uint32_t FromVersion, uint32_t ToVersion, NFunction<bool(CArchive&)> MigrationFunc)
{
    uint64_t Key = MakeMigrationKey(FromVersion, ToVersion);
    Migrations[Key] = MigrationFunc;
}

bool NVersionedSerializer::RunMigrations(CArchive& Archive, uint32_t FromVersion, uint32_t ToVersion)
{
    if (FromVersion == ToVersion)
    {
        return true;
    }
    
    // 简化实现：直接查找迁移函数
    uint64_t Key = MakeMigrationKey(FromVersion, ToVersion);
    auto It = Migrations.Find(Key);
    
    if (It != Migrations.End())
    {
        try
        {
            return It->Value(Archive);
        }
        catch (...)
        {
            Archive.AddError(CString::Format("Exception during migration from {} to {}", FromVersion, ToVersion));
            return false;
        }
    }
    
    Archive.AddWarning(CString::Format("No migration found from version {} to {}", FromVersion, ToVersion));
    return false;
}

uint64_t NVersionedSerializer::MakeMigrationKey(uint32_t FromVersion, uint32_t ToVersion) const
{
    return (static_cast<uint64_t>(FromVersion) << 32) | ToVersion;
}

} // namespace NLib