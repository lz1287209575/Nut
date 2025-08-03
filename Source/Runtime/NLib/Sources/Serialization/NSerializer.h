#pragma once

#include "Serialization/CArchive.h"
#include "Serialization/NSerializable.h"
#include "Core/CObject.h"
#include "Containers/CString.h"
#include "FileSystem/NFileSystem.h"
#include "Async/NAsyncTask.h"

namespace NLib
{

/**
 * @brief 序列化器工厂 - 创建不同类型的序列化器
 */
class LIBNLIB_API NSerializerFactory
{
public:
    // 创建存档
    static TSharedPtr<CArchive> CreateBinaryArchive(ESerializationMode Mode, const CString& FilePath);
    static TSharedPtr<CArchive> CreateJsonArchive(ESerializationMode Mode, const CString& FilePath = "");
    static TSharedPtr<CArchive> CreateMemoryArchive(ESerializationMode Mode, const CArray<uint8_t>& Data = {});
    
    // 从流创建存档
    static TSharedPtr<CArchive> CreateBinaryArchive(ESerializationMode Mode, TSharedPtr<NFileStream> Stream);
    static TSharedPtr<CArchive> CreateJsonArchive(ESerializationMode Mode, const CString& JsonString);
    
    // 根据文件扩展名自动创建
    static TSharedPtr<CArchive> CreateArchiveFromFile(ESerializationMode Mode, const CString& FilePath);
    
    // 创建序列化上下文
    static TSharedPtr<CSerializationContext> CreateContext(ESerializationFormat Format, ESerializationMode Mode);

private:
    NSerializerFactory() = delete; // 静态工厂类
};

/**
 * @brief 高级序列化器 - 提供便捷的序列化接口
 */
NCLASS()
class LIBNLIB_API NSerializer : public CObject
{
    GENERATED_BODY()

public:
    NSerializer();
    virtual ~NSerializer();
    
    // 配置选项
    void SetDefaultFormat(ESerializationFormat Format) { DefaultFormat = Format; }
    ESerializationFormat GetDefaultFormat() const { return DefaultFormat; }
    
    void SetCompressionEnabled(bool bEnabled) { bCompressionEnabled = bEnabled; }
    bool IsCompressionEnabled() const { return bCompressionEnabled; }
    
    void SetPrettyPrintEnabled(bool bEnabled) { bPrettyPrintEnabled = bEnabled; }
    bool IsPrettyPrintEnabled() const { return bPrettyPrintEnabled; }
    
    void SetVersioning(bool bEnabled) { bVersioningEnabled = bEnabled; }
    bool IsVersioningEnabled() const { return bVersioningEnabled; }
    
    // 文件序列化
    template<typename TType>
    bool SerializeToFile(const TType& Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    
    template<typename TType>
    bool DeserializeFromFile(TType& Object, const CString& FilePath);
    
    bool SerializeObjectToFile(ISerializable* Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    bool DeserializeObjectFromFile(ISerializable* Object, const CString& FilePath);
    
    // NObject序列化
    bool SerializeNObjectToFile(CObject* Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    TSharedPtr<CObject> DeserializeNObjectFromFile(const CString& FilePath);
    
    // 字符串序列化（JSON/XML）
    template<typename TType>
    CString SerializeToString(const TType& Object, ESerializationFormat Format = ESerializationFormat::JSON);
    
    template<typename TType>
    bool DeserializeFromString(TType& Object, const CString& Data, ESerializationFormat Format = ESerializationFormat::JSON);
    
    CString SerializeObjectToString(ISerializable* Object, ESerializationFormat Format = ESerializationFormat::JSON);
    bool DeserializeObjectFromString(ISerializable* Object, const CString& Data, ESerializationFormat Format = ESerializationFormat::JSON);
    
    // 内存序列化
    template<typename TType>
    CArray<uint8_t> SerializeToMemory(const TType& Object);
    
    template<typename TType>
    bool DeserializeFromMemory(TType& Object, const CArray<uint8_t>& Data);
    
    CArray<uint8_t> SerializeObjectToMemory(ISerializable* Object);
    bool DeserializeObjectFromMemory(ISerializable* Object, const CArray<uint8_t>& Data);
    
    // 异步序列化
    template<typename TType>
    TSharedPtr<NAsyncTask<bool>> SerializeToFileAsync(const TType& Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    
    template<typename TType>
    TSharedPtr<NAsyncTask<bool>> DeserializeFromFileAsync(TType& Object, const CString& FilePath);
    
    TSharedPtr<NAsyncTask<bool>> SerializeObjectToFileAsync(ISerializable* Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    TSharedPtr<NAsyncTask<bool>> DeserializeObjectFromFileAsync(ISerializable* Object, const CString& FilePath);
    
    // 批量序列化
    template<typename TType>
    bool SerializeArray(const CArray<TType>& Objects, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    
    template<typename TType>
    bool DeserializeArray(CArray<TType>& Objects, const CString& FilePath);
    
    // 对象克隆（通过序列化）
    template<typename TType>
    TType CloneObject(const TType& Object);
    
    TSharedPtr<ISerializable> CloneSerializableObject(ISerializable* Object);
    TSharedPtr<CObject> CloneNObject(CObject* Object);
    
    // 序列化统计
    struct SerializationStats
    {
        size_t TotalBytesWritten;
        size_t TotalBytesRead;
        size_t ObjectsSerializeed;
        size_t ObjectsDeserialized;
        double TotalSerializationTime;
        double TotalDeserializationTime;
        
        SerializationStats()
            : TotalBytesWritten(0), TotalBytesRead(0)
            , ObjectsSerializeed(0), ObjectsDeserialized(0)
            , TotalSerializationTime(0.0), TotalDeserializationTime(0.0)
        {}
        
        void Reset()
        {
            TotalBytesWritten = 0;
            TotalBytesRead = 0;
            ObjectsSerializeed = 0;
            ObjectsDeserialized = 0;
            TotalSerializationTime = 0.0;
            TotalDeserializationTime = 0.0;
        }
        
        CString ToString() const;
    };
    
    const SerializationStats& GetStatistics() const { return Statistics; }
    void ResetStatistics() { Statistics.Reset(); }
    
    // 错误处理
    const CArray<CString>& GetLastErrors() const { return LastErrors; }
    const CArray<CString>& GetLastWarnings() const { return LastWarnings; }
    bool HasErrors() const { return !LastErrors.IsEmpty(); }
    bool HasWarnings() const { return !LastWarnings.IsEmpty(); }
    void ClearMessages();

private:
    ESerializationFormat DefaultFormat;
    bool bCompressionEnabled;
    bool bPrettyPrintEnabled;
    bool bVersioningEnabled;
    
    SerializationStats Statistics;
    CArray<CString> LastErrors;
    CArray<CString> LastWarnings;
    
    // 内部辅助方法
    TSharedPtr<CSerializationContext> CreateContext(ESerializationFormat Format, ESerializationMode Mode);
    TSharedPtr<CArchive> CreateArchive(ESerializationFormat Format, ESerializationMode Mode, const CString& FilePath = "");
    
    void UpdateStatistics(CArchive* Archive, bool bSerialization, double ElapsedTime);
    void CopyMessages(CSerializationContext* Context);
    
    ESerializationFormat DetectFormat(const CString& FilePath);
    
    template<typename TType>
    bool SerializeInternal(const TType& Object, CArchive& Archive);
    
    template<typename TType>
    bool DeserializeInternal(TType& Object, CArchive& Archive);
};

/**
 * @brief 序列化助手类 - 提供全局便捷函数
 */
class LIBNLIB_API NSerializationHelper
{
public:
    // 全局序列化器实例
    static NSerializer& GetGlobalSerializer();
    
    // 便捷函数
    template<typename TType>
    static bool SaveToFile(const TType& Object, const CString& FilePath, ESerializationFormat Format = ESerializationFormat::Binary);
    
    template<typename TType>
    static bool LoadFromFile(TType& Object, const CString& FilePath);
    
    template<typename TType>
    static CString SaveToString(const TType& Object, ESerializationFormat Format = ESerializationFormat::JSON);
    
    template<typename TType>
    static bool LoadFromString(TType& Object, const CString& Data, ESerializationFormat Format = ESerializationFormat::JSON);
    
    template<typename TType>
    static TType Clone(const TType& Object);
    
    // 类型注册
    template<typename TType>
    static void RegisterType();
    
    static void RegisterType(const CString& TypeName, const std::type_info& TypeInfo);

private:
    NSerializationHelper() = delete; // 静态助手类
};

/**
 * @brief 版本化序列化助手
 */
class LIBNLIB_API NVersionedSerializer : public CObject
{
    NCLASS()
class NVersionedSerializer : public NObject
{
    GENERATED_BODY()

public:
    NVersionedSerializer();
    virtual ~NVersionedSerializer();
    
    // 版本管理
    void SetCurrentVersion(uint32_t Version) { CurrentVersion = Version; }
    uint32_t GetCurrentVersion() const { return CurrentVersion; }
    
    void RegisterMigration(uint32_t FromVersion, uint32_t ToVersion, NFunction<bool(CArchive&)> MigrationFunc);
    bool RunMigrations(CArchive& Archive, uint32_t FromVersion, uint32_t ToVersion);
    
    // 版本化序列化
    template<typename TType>
    bool SerializeVersioned(const TType& Object, CArchive& Archive);
    
    template<typename TType>
    bool DeserializeVersioned(TType& Object, CArchive& Archive);

private:
    uint32_t CurrentVersion;
    CHashMap<uint64_t, NFunction<bool(CArchive&)>> Migrations; // Key = (FromVersion << 32) | ToVersion
    
    uint64_t MakeMigrationKey(uint32_t FromVersion, uint32_t ToVersion) const;
};

// === 模板实现 ===

template<typename TType>
bool NSerializer::SerializeToFile(const TType& Object, const CString& FilePath, ESerializationFormat Format)
{
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    auto Archive = CreateArchive(Format, ESerializationMode::Writing, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return false;
    }
    
    bool Success = SerializeInternal(Object, *Archive);
    
    UpdateStatistics(Archive.Get(), true, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Success;
}

template<typename TType>
bool NSerializer::DeserializeFromFile(TType& Object, const CString& FilePath)
{
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    ESerializationFormat Format = DetectFormat(FilePath);
    auto Archive = CreateArchive(Format, ESerializationMode::Reading, FilePath);
    if (!Archive)
    {
        AddError("Failed to create archive for file: " + FilePath);
        return false;
    }
    
    bool Success = DeserializeInternal(Object, *Archive);
    
    UpdateStatistics(Archive.Get(), false, Stopwatch.GetElapsed().GetTotalSeconds());
    CopyMessages(Archive->GetContext());
    
    return Success;
}

template<typename TType>
CString NSerializer::SerializeToString(const TType& Object, ESerializationFormat Format)
{
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
    
    if (SerializeInternal(Object, *Archive))
    {
        if (auto JsonArchive = CObject::Cast<NJsonArchive>(Archive))
        {
            return JsonArchive->ToJsonString(bPrettyPrintEnabled);
        }
    }
    
    CopyMessages(Archive->GetContext());
    return "";
}

template<typename TType>
bool NSerializer::DeserializeFromString(TType& Object, const CString& Data, ESerializationFormat Format)
{
    if (Format == ESerializationFormat::Binary)
    {
        AddError("Binary format not supported for string deserialization");
        return false;
    }
    
    auto Context = CreateContext(Format, ESerializationMode::Reading);
    auto Archive = NewNObject<NJsonArchive>(Context, Data);
    
    bool Success = DeserializeInternal(Object, *Archive);
    CopyMessages(Context.Get());
    
    return Success;
}

template<typename TType>
CArray<uint8_t> NSerializer::SerializeToMemory(const TType& Object)
{
    auto Archive = CreateArchive(ESerializationFormat::Binary, ESerializationMode::Writing);
    if (auto MemoryArchive = CObject::Cast<NMemoryArchive>(Archive))
    {
        if (SerializeInternal(Object, *Archive))
        {
            return MemoryArchive->GetData();
        }
    }
    
    return CArray<uint8_t>();
}

template<typename TType>
bool NSerializer::DeserializeFromMemory(TType& Object, const CArray<uint8_t>& Data)
{
    auto Context = CreateContext(ESerializationFormat::Binary, ESerializationMode::Reading);
    auto Archive = NewNObject<NMemoryArchive>(Context, Data);
    
    return DeserializeInternal(Object, *Archive);
}

template<typename TType>
TType NSerializer::CloneObject(const TType& Object)
{
    CArray<uint8_t> Data = SerializeToMemory(Object);
    TType ClonedObject;
    if (DeserializeFromMemory(ClonedObject, Data))
    {
        return ClonedObject;
    }
    return TType{};
}

template<typename TType>
bool NSerializer::SerializeInternal(const TType& Object, CArchive& Archive)
{
    if constexpr (std::is_base_of_v<ISerializable, TType>)
    {
        const_cast<TType&>(Object).Serialize(Archive);
        return !Archive.HasErrors();
    }
    else
    {
        // 对于基础类型，直接序列化
        TType& MutableObject = const_cast<TType&>(Object);
        return Archive.SerializeValue("Value", MutableObject);
    }
}

template<typename TType>
bool NSerializer::DeserializeInternal(TType& Object, CArchive& Archive)
{
    if constexpr (std::is_base_of_v<ISerializable, TType>)
    {
        Object.Deserialize(Archive);
        return !Archive.HasErrors();
    }
    else
    {
        // 对于基础类型，直接反序列化
        return Archive.SerializeValue("Value", Object);
    }
}

// NSerializationHelper 模板实现
template<typename TType>
bool NSerializationHelper::SaveToFile(const TType& Object, const CString& FilePath, ESerializationFormat Format)
{
    return GetGlobalSerializer().SerializeToFile(Object, FilePath, Format);
}

template<typename TType>
bool NSerializationHelper::LoadFromFile(TType& Object, const CString& FilePath)
{
    return GetGlobalSerializer().DeserializeFromFile(Object, FilePath);
}

template<typename TType>
CString NSerializationHelper::SaveToString(const TType& Object, ESerializationFormat Format)
{
    return GetGlobalSerializer().SerializeToString(Object, Format);
}

template<typename TType>
bool NSerializationHelper::LoadFromString(TType& Object, const CString& Data, ESerializationFormat Format)
{
    return GetGlobalSerializer().DeserializeFromString(Object, Data, Format);
}

template<typename TType>
TType NSerializationHelper::Clone(const TType& Object)
{
    return GetGlobalSerializer().CloneObject(Object);
}

template<typename TType>
void NSerializationHelper::RegisterType()
{
    RegisterType(typeid(TType).name(), typeid(TType));
}

/**
 * @brief 序列化便捷宏
 */
#define SERIALIZE_TO_FILE(Object, FilePath) \
    NLib::NSerializationHelper::SaveToFile(Object, FilePath)

#define DESERIALIZE_FROM_FILE(Object, FilePath) \
    NLib::NSerializationHelper::LoadFromFile(Object, FilePath)

#define SERIALIZE_TO_JSON(Object) \
    NLib::NSerializationHelper::SaveToString(Object, NLib::ESerializationFormat::JSON)

#define DESERIALIZE_FROM_JSON(Object, JsonString) \
    NLib::NSerializationHelper::LoadFromString(Object, JsonString, NLib::ESerializationFormat::JSON)

#define CLONE_OBJECT(Object) \
    NLib::NSerializationHelper::Clone(Object)

#define REGISTER_SERIALIZABLE_TYPE(Type) \
    NLib::NSerializationHelper::RegisterType<Type>()

} // namespace NLib
