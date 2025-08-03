#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Delegates/CDelegate.h"

namespace NLib
{

/**
 * @brief 配置值类型枚举
 */
enum class EConfigValueType : uint32_t
{
    Null,
    Bool,
    Int,
    Float,
    String,
    Array,
    Object
};

/**
 * @brief 配置值包装器 - 支持多种数据类型的统一访问
 */
class LIBNLIB_API ICConfigValue : public CObject
{
    NCLASS()
class ICConfigValue : public CObject
{
    GENERATED_BODY()

public:
    // 构造函数
    ICConfigValue();
    ICConfigValue(bool Value);
    ICConfigValue(int32_t Value);
    ICConfigValue(int64_t Value);
    ICConfigValue(float Value);
    ICConfigValue(double Value);
    ICConfigValue(const char* Value);
    ICConfigValue(const CString& Value);
    ICConfigValue(const CArray<ICConfigValue>& Array);
    ICConfigValue(const CHashMap<CString, ICConfigValue>& Object);
    
    // 拷贝和移动构造
    ICConfigValue(const ICConfigValue& Other);
    ICConfigValue(ICConfigValue&& Other) noexcept;
    
    // 赋值操作符
    ICConfigValue& operator=(const ICConfigValue& Other);
    ICConfigValue& operator=(ICConfigValue&& Other) noexcept;
    
    virtual ~ICConfigValue();
    
    // 类型查询
    EConfigValueType GetType() const;
    bool IsNull() const;
    bool IsBool() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsObject() const;
    
    // 类型转换 - 带默认值
    bool AsBool(bool DefaultValue = false) const;
    int32_t AsInt(int32_t DefaultValue = 0) const;
    int64_t AsInt64(int64_t DefaultValue = 0) const;
    float AsFloat(float DefaultValue = 0.0f) const;
    double AsDouble(double DefaultValue = 0.0) const;
    CString AsString(const CString& DefaultValue = "") const;
    
    // 数组访问
    const ICConfigValue& operator[](int32_t Index) const;
    ICConfigValue& operator[](int32_t Index);
    
    // 对象访问
    const ICConfigValue& operator[](const CString& Key) const;
    ICConfigValue& operator[](const CString& Key);
    
    // 数组操作
    void PushBack(const ICConfigValue& Value);
    void PushBack(ICConfigValue&& Value);
    void PopBack();
    int32_t GetArraySize() const;
    void ResizeArray(int32_t NewSize);
    void ClearArray();
    
    // 对象操作
    bool HasKey(const CString& Key) const;
    void SetValue(const CString& Key, const ICConfigValue& Value);
    void RemoveKey(const CString& Key);
    CArray<CString> GetKeys() const;
    int32_t GetObjectSize() const;
    void ClearObject();
    
    // 路径访问 - 支持 "section.subsection.key" 格式
    ICConfigValue& GetPath(const CString& Path);
    const ICConfigValue& GetPath(const CString& Path) const;
    void SetPath(const CString& Path, const ICConfigValue& Value);
    bool HasPath(const CString& Path) const;
    
    // 工具方法
    void Clear();
    bool IsEmpty() const;
    virtual CString ToString() const override;
    CString ToJsonString(bool Pretty = false) const;
    
    // 比较操作符
    bool operator==(const ICConfigValue& Other) const;
    bool operator!=(const ICConfigValue& Other) const;
    
    // 合并操作 - 递归合并对象
    void Merge(const ICConfigValue& Other, bool Overwrite = true);
    
    // 静态工厂方法
    static ICConfigValue CreateNull();
    static ICConfigValue CreateArray();
    static ICConfigValue CreateObject();
    static ICConfigValue FromJsonString(const CString& JsonString);

private:
    EConfigValueType Type;
    
    union
    {
        bool BoolValue;
        int64_t IntValue;
        double FloatValue;
        CString* StringValue;
        CArray<ICConfigValue>* ArrayValue;
        CHashMap<CString, ICConfigValue>* ObjectValue;
    };
    
    void DestroyValue();
    void CopyFrom(const ICConfigValue& Other);
    void MoveFrom(ICConfigValue&& Other);
    
    CArray<CString> SplitPath(const CString& Path) const;
    
    // 静态的空值引用
    static const ICConfigValue& GetNullValue();
};

/**
 * @brief 配置加载器接口
 */
class LIBNLIB_API IConfigLoader
{
public:
    virtual ~IConfigLoader() = default;
    
    virtual bool CanLoad(const CString& FilePath) const = 0;
    virtual bool Load(const CString& FilePath, ICConfigValue& OutConfig) = 0;
    virtual bool Save(const CString& FilePath, const ICConfigValue& Config) = 0;
    virtual CString GetSupportedExtensions() const = 0;
};

/**
 * @brief JSON配置加载器
 */
class LIBNLIB_API NJsonConfigLoader : public CObject, public IConfigLoader
{
    NCLASS()
class NJsonConfigLoader : public NObject
{
    GENERATED_BODY()

public:
    NJsonConfigLoader();
    virtual ~NJsonConfigLoader();
    
    virtual bool CanLoad(const CString& FilePath) const override;
    virtual bool Load(const CString& FilePath, ICConfigValue& OutConfig) override;
    virtual bool Save(const CString& FilePath, const ICConfigValue& Config) override;
    virtual CString GetSupportedExtensions() const override;
    
    // JSON特定选项
    void SetPrettyPrint(bool bPretty);
    bool GetPrettyPrint() const;

private:
    bool bPrettyPrint;
    
    bool ParseJsonValue(const CString& JsonText, int32_t& Index, ICConfigValue& OutValue);
    bool ParseJsonObject(const CString& JsonText, int32_t& Index, ICConfigValue& OutValue);
    bool ParseJsonArray(const CString& JsonText, int32_t& Index, ICConfigValue& OutValue);
    bool ParseJsonString(const CString& JsonText, int32_t& Index, CString& OutString);
    bool ParseJsonNumber(const CString& JsonText, int32_t& Index, ICConfigValue& OutValue);
    bool ParseJsonKeyword(const CString& JsonText, int32_t& Index, ICConfigValue& OutValue);
    
    void SkipWhitespace(const CString& JsonText, int32_t& Index);
    CString EscapeJsonString(const CString& Input) const;
    CString UnescapeJsonString(const CString& Input) const;
    
    CString SerializeJsonValue(const ICConfigValue& Value, int32_t IndentLevel = 0) const;
};

/**
 * @brief INI配置加载器
 */
class LIBNLIB_API NIniConfigLoader : public CObject, public IConfigLoader
{
    NCLASS()
class NIniConfigLoader : public NObject
{
    GENERATED_BODY()

public:
    NIniConfigLoader();
    virtual ~NIniConfigLoader();
    
    virtual bool CanLoad(const CString& FilePath) const override;
    virtual bool Load(const CString& FilePath, ICConfigValue& OutConfig) override;
    virtual bool Save(const CString& FilePath, const ICConfigValue& Config) override;
    virtual CString GetSupportedExtensions() const override;

private:
    CString TrimString(const CString& Input) const;
    bool IsComment(const CString& Line) const;
    bool IsSection(const CString& Line, CString& OutSectionName) const;
    bool IsKeyValue(const CString& Line, CString& OutKey, CString& OutValue) const;
    
    ICConfigValue ParseValue(const CString& ValueString) const;
    CString SerializeValue(const ICConfigValue& Value) const;
};

/**
 * @brief 配置观察器 - 监听配置变化
 */
class LIBNLIB_API NConfigWatcher : public CObject
{
    NCLASS()
class NConfigWatcher : public NObject
{
    GENERATED_BODY()

public:
    using ConfigChangeCallback = CDelegate<void(const CString&, const ICConfigValue&, const ICConfigValue&)>;
    
    NConfigWatcher();
    virtual ~NConfigWatcher();
    
    void Watch(const CString& KeyPath, ConfigChangeCallback Callback);
    void Unwatch(const CString& KeyPath);
    void UnwatchAll();
    
    void NotifyChange(const CString& KeyPath, const ICConfigValue& OldValue, const ICConfigValue& NewValue);
    
    bool IsWatching(const CString& KeyPath) const;
    int32_t GetWatchCount() const;

private:
    CHashMap<CString, CArray<ConfigChangeCallback>> Watchers;
};

} // namespace NLib