#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Reflection/CObjectReflection.h"

namespace NLib
{

class CArchive;
class CSerializationContext;

/**
 * @brief 序列化接口
 */
class LIBNLIB_API ISerializable
{
public:
    virtual ~ISerializable() = default;
    
    /**
     * @brief 序列化对象到存档
     * @param Archive 序列化存档
     */
    virtual void Serialize(CArchive& Archive) = 0;
    
    /**
     * @brief 反序列化对象从存档
     * @param Archive 序列化存档
     */
    virtual void Deserialize(CArchive& Archive) = 0;
    
    /**
     * @brief 获取序列化版本号
     */
    virtual uint32_t GetSerializationVersion() const { return 1; }
    
    /**
     * @brief 检查是否支持指定版本的反序列化
     */
    virtual bool CanDeserializeVersion(uint32_t Version) const { return Version <= GetSerializationVersion(); }
};

/**
 * @brief 序列化模式
 */
enum class ESerializationMode : uint32_t
{
    Reading,    // 读取模式（反序列化）
    Writing     // 写入模式（序列化）
};

/**
 * @brief 序列化格式
 */
enum class ESerializationFormat : uint32_t
{
    Binary,     // 二进制格式
    JSON,       // JSON格式
    XML,        // XML格式
    Custom      // 自定义格式
};

/**
 * @brief 序列化上下文 - 提供序列化过程中的上下文信息
 */
NCLASS()
class LIBNLIB_API CSerializationContext : public CObject
{
    GENERATED_BODY()

public:
    CSerializationContext();
    virtual ~CSerializationContext();
    
    // 序列化选项
    void SetFormat(ESerializationFormat InFormat) { Format = InFormat; }
    ESerializationFormat GetFormat() const { return Format; }
    
    void SetMode(ESerializationMode InMode) { Mode = InMode; }
    ESerializationMode GetMode() const { return Mode; }
    
    bool IsReading() const { return Mode == ESerializationMode::Reading; }
    bool IsWriting() const { return Mode == ESerializationMode::Writing; }
    
    // 版本管理
    void SetVersion(uint32_t InVersion) { Version = InVersion; }
    uint32_t GetVersion() const { return Version; }
    
    // 对象引用管理
    void RegisterObject(CObject* Object, uint64_t ObjectId);
    CObject* FindObject(uint64_t ObjectId) const;
    uint64_t GetObjectId(CObject* Object) const;
    bool HasObject(CObject* Object) const;
    
    // 类型注册
    void RegisterType(const CString& TypeName, const std::type_info& TypeInfo);
    const std::type_info* FindType(const CString& TypeName) const;
    CString GetTypeName(const std::type_info& TypeInfo) const;
    
    // 自定义数据
    template<typename TType>
    void SetCustomData(const CString& Key, const T& Value);
    
    template<typename TType>
    T GetCustomData(const CString& Key, const T& DefaultValue = T{}) const;
    
    bool HasCustomData(const CString& Key) const;
    void RemoveCustomData(const CString& Key);
    
    // 错误处理
    void AddError(const CString& Error);
    void AddWarning(const CString& Warning);
    const CArray<CString>& GetErrors() const { return Errors; }
    const CArray<CString>& GetWarnings() const { return Warnings; }
    bool HasErrors() const { return !Errors.IsEmpty(); }
    bool HasWarnings() const { return !Warnings.IsEmpty(); }
    void ClearMessages();

private:
    ESerializationFormat Format;
    ESerializationMode Mode;
    uint32_t Version;
    
    // 对象引用映射
    CHashMap<uint64_t, TWeakPtr<CObject>> ObjectIdMap;
    CHashMap<CObject*, uint64_t> ObjectToIdMap;
    uint64_t NextObjectId;
    
    // 类型注册
    CHashMap<CString, const std::type_info*> TypeRegistry;
    CHashMap<const std::type_info*, CString> TypeNameRegistry;
    
    // 自定义数据
    CHashMap<CString, CString> CustomData;
    
    // 错误和警告
    CArray<CString> Errors;
    CArray<CString> Warnings;
};

/**
 * @brief 序列化特性 - 控制字段的序列化行为
 */
struct LIBNLIB_API NSerializationAttribute
{
    enum EFlags : uint32_t
    {
        None = 0,
        SkipSerialization = 1 << 0,    // 跳过序列化
        SkipDeserialization = 1 << 1,  // 跳过反序列化
        Required = 1 << 2,             // 必需字段
        Optional = 1 << 3,             // 可选字段
        Deprecated = 1 << 4,           // 已弃用字段
        Transient = 1 << 5,            // 临时字段（不序列化）
    };
    
    uint32_t Flags;
    CString Name;           // 序列化名称（可与字段名不同）
    uint32_t Version;       // 引入版本
    uint32_t MaxVersion;    // 最大支持版本
    CString DefaultValue;   // 默认值
    
    NSerializationAttribute()
        : Flags(None), Version(1), MaxVersion(UINT32_MAX)
    {}
    
    bool HasFlag(EFlags Flag) const { return (Flags & Flag) != 0; }
    void SetFlag(EFlags Flag) { Flags |= Flag; }
    void ClearFlag(EFlags Flag) { Flags &= ~Flag; }
};

/**
 * @brief 序列化宏定义
 */
#define DECLARE_SERIALIZABLE() \
    virtual void Serialize(NLib::CArchive& Archive) override; \
    virtual void Deserialize(NLib::CArchive& Archive) override;

#define DECLARE_SERIALIZABLE_VERSION(Version) \
    DECLARE_SERIALIZABLE() \
    virtual uint32_t GetSerializationVersion() const override { return Version; }

#define SERIALIZE_FIELD(Archive, Field) \
    Archive.SerializeValue(#Field, Field)

#define SERIALIZE_FIELD_NAMED(Archive, Field, Name) \
    Archive.SerializeValue(Name, Field)

#define SERIALIZE_FIELD_OPTIONAL(Archive, Field, DefaultValue) \
    Archive.SerializeOptionalValue(#Field, Field, DefaultValue)

#define SERIALIZE_OBJECT(Archive, Object) \
    Archive.SerializeObject(#Object, Object)

#define SERIALIZE_ARRAY(Archive, Array) \
    Archive.SerializeArray(#Array, Array)

/**
 * @brief 自动序列化宏 - 使用反射系统
 */
#define AUTO_SERIALIZE_BEGIN(ClassName) \
    void ClassName::Serialize(NLib::CArchive& Archive) { \
        Super::Serialize(Archive); \
        Archive.BeginObject(#ClassName);

#define AUTO_SERIALIZE_FIELD(Field) \
        SERIALIZE_FIELD(Archive, Field);

#define AUTO_SERIALIZE_FIELD_OPTIONAL(Field, DefaultValue) \
        SERIALIZE_FIELD_OPTIONAL(Archive, Field, DefaultValue);

#define AUTO_SERIALIZE_END() \
        Archive.EndObject(); \
    }

#define AUTO_DESERIALIZE_BEGIN(ClassName) \
    void ClassName::Deserialize(NLib::CArchive& Archive) { \
        Super::Deserialize(Archive); \
        Archive.BeginObject(#ClassName);

#define AUTO_DESERIALIZE_FIELD(Field) \
        SERIALIZE_FIELD(Archive, Field);

#define AUTO_DESERIALIZE_FIELD_OPTIONAL(Field, DefaultValue) \
        SERIALIZE_FIELD_OPTIONAL(Archive, Field, DefaultValue);

#define AUTO_DESERIALIZE_END() \
        Archive.EndObject(); \
    }

/**
 * @brief 序列化工具类
 */
class LIBNLIB_API NSerializationUtils
{
public:
    // 基础类型序列化辅助
    template<typename TType>
    static bool SerializePrimitiveType(CArchive& Archive, const CString& Name, T& Value);
    
    // 容器序列化辅助
    template<typename TType>
    static bool SerializeArray(CArchive& Archive, const CString& Name, CArray<T>& Array);
    
    template<typename TKey, typename TValue>
    static bool SerializeHashMap(CArchive& Archive, const CString& Name, CHashMap<K, V>& Map);
    
    // 对象序列化辅助
    static bool SerializeObject(CArchive& Archive, const CString& Name, TSharedPtr<CObject>& Object);
    static bool SerializeObject(CArchive& Archive, const CString& Name, CObject* Object);
    
    // 多态对象序列化
    template<typename TBase>
    static bool SerializePolymorphicObject(CArchive& Archive, const CString& Name, TSharedPtr<TBase>& Object);
    
    // 枚举序列化
    template<typename TEnum>
    static bool SerializeEnum(CArchive& Archive, const CString& Name, TEnum& EnumValue);
    
    // 版本兼容性检查
    static bool IsVersionCompatible(uint32_t CurrentVersion, uint32_t RequiredVersion);
    static void HandleVersionMismatch(CSerializationContext& Context, uint32_t ExpectedVersion, uint32_t ActualVersion);
    
    // 类型信息
    template<typename TType>
    static CString GetTypeName();
    
    template<typename TType>
    static void RegisterType(CSerializationContext& Context);

private:
    NSerializationUtils() = delete; // 静态工具类
};

// === 模板实现 ===

template<typename TType>
void CSerializationContext::SetCustomData(const CString& Key, const T& Value)
{
    if constexpr (std::is_same_v<T, CString>)
    {
        CustomData[Key] = Value;
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        CustomData[Key] = CString::Format("{}", Value);
    }
    else
    {
        CustomData[Key] = Value.ToString();
    }
}

template<typename TType>
T CSerializationContext::GetCustomData(const CString& Key, const T& DefaultValue) const
{
    if (!CustomData.Contains(Key))
    {
        return DefaultValue;
    }
    
    const CString& ValueStr = CustomData[Key];
    
    if constexpr (std::is_same_v<T, CString>)
    {
        return ValueStr;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return ValueStr.ToLower() == "true" || ValueStr == "1";
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        if constexpr (std::is_integral_v<T>)
        {
            return static_cast<T>(strtoll(ValueStr.GetCStr(), nullptr, 10));
        }
        else
        {
            return static_cast<T>(strtod(ValueStr.GetCStr(), nullptr));
        }
    }
    else
    {
        return DefaultValue;
    }
}

template<typename TType>
CString NSerializationUtils::GetTypeName()
{
    return typeid(T).name();
}

template<typename TType>
void NSerializationUtils::RegisterType(CSerializationContext& Context)
{
    Context.RegisterType(GetTypeName<T>(), typeid(T));
}

} // namespace NLib
