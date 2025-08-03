#pragma once

#include "Serialization/NSerializable.h"
#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "FileSystem/NFileSystem.h"

namespace NLib
{

/**
 * @brief 序列化存档基类 - 提供统一的序列化接口
 */
NCLASS()
class LIBNLIB_API CArchive : public CObject
{
    GENERATED_BODY()

public:
    CArchive(TSharedPtr<CSerializationContext> InContext);
    virtual ~CArchive();
    
    // 存档状态
    bool IsReading() const;
    bool IsWriting() const;
    ESerializationMode GetMode() const;
    ESerializationFormat GetFormat() const;
    
    CSerializationContext* GetContext() const { return Context.Get(); }
    void SetContext(TSharedPtr<CSerializationContext> InContext) { Context = InContext; }
    
    // 基础类型序列化
    virtual bool SerializeValue(const CString& Name, bool& Value) = 0;
    virtual bool SerializeValue(const CString& Name, int8_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, uint8_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, int16_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, uint16_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, int32_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, uint32_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, int64_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, uint64_t& Value) = 0;
    virtual bool SerializeValue(const CString& Name, float& Value) = 0;
    virtual bool SerializeValue(const CString& Name, double& Value) = 0;
    virtual bool SerializeValue(const CString& Name, CString& Value) = 0;
    
    // 可选值序列化（带默认值）
    template<typename TType>
    bool SerializeOptionalValue(const CString& Name, TType& Value, const TType& DefaultValue);
    
    // 数组序列化
    template<typename TType>
    bool SerializeArray(const CString& Name, CArray<TType>& Array);
    
    // 映射序列化
    template<typename TKey, typename TValue>
    bool SerializeHashMap(const CString& Name, CHashMap<K, V>& Map);
    
    // 对象序列化
    bool SerializeObject(const CString& Name, TSharedPtr<ISerializable>& Object);
    bool SerializeObject(const CString& Name, ISerializable* Object);
    
    // NObject序列化（支持多态）
    bool SerializeNObject(const CString& Name, TSharedPtr<CObject>& Object);
    bool SerializeNObject(const CString& Name, CObject* Object);
    
    // 结构化数据
    virtual bool BeginObject(const CString& Name) = 0;
    virtual bool EndObject() = 0;
    virtual bool BeginArray(const CString& Name, int32_t& ElementCount) = 0;
    virtual bool EndArray() = 0;
    virtual bool BeginArrayElement(int32_t Index) = 0;
    virtual bool EndArrayElement() = 0;
    
    // 数据块序列化（原始二进制数据）
    virtual bool SerializeRawData(const CString& Name, void* Data, size_t Size) = 0;
    virtual bool SerializeRawData(const CString& Name, CArray<uint8_t>& Data) = 0;
    
    // 版本控制
    virtual bool SerializeVersion(const CString& Name, uint32_t& Version);
    bool CheckVersion(uint32_t RequiredVersion, uint32_t MaxVersion = UINT32_MAX);
    
    // 条件序列化
    bool SerializeConditional(const CString& Name, bool Condition, NFunction<bool()> SerializeFunc);
    
    // 错误处理
    void AddError(const CString& Error);
    void AddWarning(const CString& Warning);
    bool HasErrors() const;
    bool HasWarnings() const;
    const CArray<CString>& GetErrors() const;
    const CArray<CString>& GetWarnings() const;
    
    // 统计信息
    virtual size_t GetBytesProcessed() const { return BytesProcessed; }
    virtual void ResetStatistics() { BytesProcessed = 0; }

protected:
    TSharedPtr<CSerializationContext> Context;
    size_t BytesProcessed;
    
    // 子类实现的内部方法
    virtual bool SerializeArrayBegin(const CString& Name, int32_t ElementCount) = 0;
    virtual bool SerializeArrayEnd() = 0;
    virtual bool SerializeArrayElementBegin(int32_t Index) = 0;
    virtual bool SerializeArrayElementEnd() = 0;
    
    // 辅助方法
    void UpdateBytesProcessed(size_t Bytes) { BytesProcessed += Bytes; }
};

/**
 * @brief 二进制存档 - 高效的二进制序列化
 */
class LIBNLIB_API NBinaryArchive : public CArchive
{
    NCLASS()
class NBinaryArchive : public CArchive
{
    GENERATED_BODY()

public:
    NBinaryArchive(TSharedPtr<CSerializationContext> Context, TSharedPtr<NFileStream> InStream);
    virtual ~NBinaryArchive();
    
    // 基础类型序列化实现
    virtual bool SerializeValue(const CString& Name, bool& Value) override;
    virtual bool SerializeValue(const CString& Name, int8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, float& Value) override;
    virtual bool SerializeValue(const CString& Name, double& Value) override;
    virtual bool SerializeValue(const CString& Name, CString& Value) override;
    
    // 结构化数据实现
    virtual bool BeginObject(const CString& Name) override;
    virtual bool EndObject() override;
    virtual bool BeginArray(const CString& Name, int32_t& ElementCount) override;
    virtual bool EndArray() override;
    virtual bool BeginArrayElement(int32_t Index) override;
    virtual bool EndArrayElement() override;
    
    // 原始数据序列化
    virtual bool SerializeRawData(const CString& Name, void* Data, size_t Size) override;
    virtual bool SerializeRawData(const CString& Name, CArray<uint8_t>& Data) override;
    
    // 流操作
    NFileStream* GetStream() const { return Stream.Get(); }
    bool IsStreamValid() const;
    
    // 压缩选项
    void SetCompressionEnabled(bool bEnabled) { bCompressionEnabled = bEnabled; }
    bool IsCompressionEnabled() const { return bCompressionEnabled; }

protected:
    virtual bool SerializeArrayBegin(const CString& Name, int32_t ElementCount) override;
    virtual bool SerializeArrayEnd() override;
    virtual bool SerializeArrayElementBegin(int32_t Index) override;
    virtual bool SerializeArrayElementEnd() override;

private:
    TSharedPtr<NFileStream> Stream;
    bool bCompressionEnabled;
    
    // 内部读写方法
    template<typename TType>
    bool WriteValue(const TType& Value);
    
    template<typename TType>
    bool ReadValue(TType& Value);
    
    bool WriteString(const CString& Value);
    bool ReadString(CString& Value);
    
    bool WriteRawBytes(const void* Data, size_t Size);
    bool ReadRawBytes(void* Data, size_t Size);
};

/**
 * @brief JSON存档 - 人类可读的JSON序列化
 */
class LIBNLIB_API NJsonArchive : public CArchive
{
    NCLASS()
class NJsonArchive : public CArchive
{
    GENERATED_BODY()

public:
    NJsonArchive(TSharedPtr<CSerializationContext> Context);
    NJsonArchive(TSharedPtr<CSerializationContext> Context, const CString& JsonString);
    virtual ~NJsonArchive();
    
    // 基础类型序列化实现
    virtual bool SerializeValue(const CString& Name, bool& Value) override;
    virtual bool SerializeValue(const CString& Name, int8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, float& Value) override;
    virtual bool SerializeValue(const CString& Name, double& Value) override;
    virtual bool SerializeValue(const CString& Name, CString& Value) override;
    
    // 结构化数据实现
    virtual bool BeginObject(const CString& Name) override;
    virtual bool EndObject() override;
    virtual bool BeginArray(const CString& Name, int32_t& ElementCount) override;
    virtual bool EndArray() override;
    virtual bool BeginArrayElement(int32_t Index) override;
    virtual bool EndArrayElement() override;
    
    // 原始数据序列化（Base64编码）
    virtual bool SerializeRawData(const CString& Name, void* Data, size_t Size) override;
    virtual bool SerializeRawData(const CString& Name, CArray<uint8_t>& Data) override;
    
    // JSON特定操作
    CString ToJsonString(bool Pretty = true) const;
    bool FromJsonString(const CString& JsonString);
    
    // 格式化选项
    void SetPrettyPrint(bool bPretty) { bPrettyPrint = bPretty; }
    bool IsPrettyPrint() const { return bPrettyPrint; }
    
    void SetIndentSize(int32_t Size) { IndentSize = Size; }
    int32_t GetIndentSize() const { return IndentSize; }

protected:
    virtual bool SerializeArrayBegin(const CString& Name, int32_t ElementCount) override;
    virtual bool SerializeArrayEnd() override;
    virtual bool SerializeArrayElementBegin(int32_t Index) override;
    virtual bool SerializeArrayElementEnd() override;

private:
    class CJsonValue* RootValue;
    CArray<class CJsonValue*> ValueStack;
    class CJsonValue* CurrentValue;
    bool bPrettyPrint;
    int32_t IndentSize;
    int32_t CurrentArrayIndex;
    
    // JSON操作辅助
    class CJsonValue* GetCurrentContainer();
    class CJsonValue* CreateValue(const CString& Name);
    class CJsonValue* FindValue(const CString& Name);
    void PushValue(class CJsonValue* Value);
    void PopValue();
    
    // Base64编码/解码
    CString EncodeBase64(const void* Data, size_t Size) const;
    CArray<uint8_t> DecodeBase64(const CString& EncodedData) const;
};

/**
 * @brief 内存存档 - 在内存中进行序列化
 */
class LIBNLIB_API NMemoryArchive : public CArchive
{
    NCLASS()
class NMemoryArchive : public CArchive
{
    GENERATED_BODY()

public:
    NMemoryArchive(TSharedPtr<CSerializationContext> Context);
    NMemoryArchive(TSharedPtr<CSerializationContext> Context, const CArray<uint8_t>& Data);
    virtual ~NMemoryArchive();
    
    // 基础类型序列化实现（委托给二进制实现）
    virtual bool SerializeValue(const CString& Name, bool& Value) override;
    virtual bool SerializeValue(const CString& Name, int8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint8_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint16_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint32_t& Value) override;
    virtual bool SerializeValue(const CString& Name, int64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, uint64_t& Value) override;
    virtual bool SerializeValue(const CString& Name, float& Value) override;
    virtual bool SerializeValue(const CString& Name, double& Value) override;
    virtual bool SerializeValue(const CString& Name, CString& Value) override;
    
    // 结构化数据实现
    virtual bool BeginObject(const CString& Name) override;
    virtual bool EndObject() override;
    virtual bool BeginArray(const CString& Name, int32_t& ElementCount) override;
    virtual bool EndArray() override;
    virtual bool BeginArrayElement(int32_t Index) override;
    virtual bool EndArrayElement() override;
    
    // 原始数据序列化
    virtual bool SerializeRawData(const CString& Name, void* Data, size_t Size) override;
    virtual bool SerializeRawData(const CString& Name, CArray<uint8_t>& Data) override;
    
    // 内存操作
    const CArray<uint8_t>& GetData() const { return Data; }
    void SetData(const CArray<uint8_t>& InData);
    void ClearData();
    
    size_t GetPosition() const { return Position; }
    void SetPosition(size_t InPosition);
    void Reset() { SetPosition(0); }

protected:
    virtual bool SerializeArrayBegin(const CString& Name, int32_t ElementCount) override;
    virtual bool SerializeArrayEnd() override;
    virtual bool SerializeArrayElementBegin(int32_t Index) override;
    virtual bool SerializeArrayElementEnd() override;

private:
    CArray<uint8_t> Data;
    size_t Position;
    
    template<typename TType>
    bool WriteValue(const TType& Value);
    
    template<typename TType>
    bool ReadValue(TType& Value);
    
    bool WriteBytes(const void* Bytes, size_t Size);
    bool ReadBytes(void* Bytes, size_t Size);
    
    void EnsureCapacity(size_t RequiredSize);
};

// === 模板实现 ===

template<typename TType>
bool CArchive::SerializeOptionalValue(const CString& Name, TType& Value, const TType& DefaultValue)
{
    if (IsWriting())
    {
        // 写入时，如果值等于默认值可以选择跳过
        return SerializeValue(Name, Value);
    }
    else
    {
        // 读取时，如果不存在则使用默认值
        if (!SerializeValue(Name, Value))
        {
            Value = DefaultValue;
            return true;
        }
        return true;
    }
}

template<typename TType>
bool CArchive::SerializeArray(const CString& Name, CArray<TType>& Array)
{
    int32_t ElementCount = Array.GetSize();
    
    if (!BeginArray(Name, ElementCount))
    {
        return false;
    }
    
    if (IsReading())
    {
        Array.Resize(ElementCount);
    }
    
    for (int32_t i = 0; i < ElementCount; ++i)
    {
        if (!BeginArrayElement(i))
        {
            return false;
        }
        
        CString ElementName = CString::Format("Element_{}", i);
        if (!SerializeValue(ElementName, Array[i]))
        {
            return false;
        }
        
        if (!EndArrayElement())
        {
            return false;
        }
    }
    
    return EndArray();
}

template<typename TKey, typename TValue>
bool CArchive::SerializeHashMap(const CString& Name, CHashMap<K, V>& Map)
{
    if (IsWriting())
    {
        int32_t ElementCount = Map.GetSize();
        if (!BeginArray(Name, ElementCount))
        {
            return false;
        }
        
        int32_t Index = 0;
        for (auto& Pair : Map)
        {
            if (!BeginArrayElement(Index))
            {
                return false;
            }
            
            if (!BeginObject("Pair"))
            {
                return false;
            }
            
            K Key = Pair.Key;
            V Value = Pair.Value;
            
            if (!SerializeValue("Key", Key) || !SerializeValue("Value", Value))
            {
                return false;
            }
            
            if (!EndObject())
            {
                return false;
            }
            
            if (!EndArrayElement())
            {
                return false;
            }
            
            ++Index;
        }
    }
    else
    {
        int32_t ElementCount = 0;
        if (!BeginArray(Name, ElementCount))
        {
            return false;
        }
        
        Map.Clear();
        
        for (int32_t i = 0; i < ElementCount; ++i)
        {
            if (!BeginArrayElement(i))
            {
                return false;
            }
            
            if (!BeginObject("Pair"))
            {
                return false;
            }
            
            K Key;
            V Value;
            
            if (!SerializeValue("Key", Key) || !SerializeValue("Value", Value))
            {
                return false;
            }
            
            Map[Key] = Value;
            
            if (!EndObject())
            {
                return false;
            }
            
            if (!EndArrayElement())
            {
                return false;
            }
        }
    }
    
    return EndArray();
}

} // namespace NLib
