#include "Serialization/CArchive.h"
#include "Core/CLogger.h"
#include "Core/CAllocator.h"
#include "FileSystem/NFileSystem.h"
#include <cstring>

namespace NLib
{

// === CArchive 基类实现 ===

CArchive::CArchive(TSharedPtr<CSerializationContext> InContext)
    : Context(InContext)
    , BytesProcessed(0)
{
    if (!Context)
    {
        Context = NewNObject<CSerializationContext>();
    }
}

CArchive::~CArchive()
{
}

bool CArchive::IsReading() const
{
    return Context->IsReading();
}

bool CArchive::IsWriting() const
{
    return Context->IsWriting();
}

ESerializationMode CArchive::GetMode() const
{
    return Context->GetMode();
}

ESerializationFormat CArchive::GetFormat() const
{
    return Context->GetFormat();
}

bool CArchive::SerializeObject(const CString& Name, TSharedPtr<ISerializable>& Object)
{
    if (IsWriting())
    {
        if (!Object)
        {
            bool IsNull = true;
            return SerializeValue(Name + "_IsNull", IsNull);
        }
        
        bool IsNull = false;
        if (!SerializeValue(Name + "_IsNull", IsNull))
        {
            return false;
        }
        
        if (!BeginObject(Name))
        {
            return false;
        }
        
        Object->Serialize(*this);
        
        return EndObject();
    }
    else
    {
        bool IsNull = false;
        if (!SerializeValue(Name + "_IsNull", IsNull))
        {
            return false;
        }
        
        if (IsNull)
        {
            Object.Reset();
            return true;
        }
        
        if (!BeginObject(Name))
        {
            return false;
        }
        
        if (Object)
        {
            Object->Deserialize(*this);
        }
        else
        {
            AddError("Cannot deserialize null ISerializable object");
            return false;
        }
        
        return EndObject();
    }
}

bool CArchive::SerializeObject(const CString& Name, ISerializable* Object)
{
    if (IsWriting())
    {
        if (!Object)
        {
            bool IsNull = true;
            return SerializeValue(Name + "_IsNull", IsNull);
        }
        
        bool IsNull = false;
        if (!SerializeValue(Name + "_IsNull", IsNull))
        {
            return false;
        }
        
        if (!BeginObject(Name))
        {
            return false;
        }
        
        Object->Serialize(*this);
        
        return EndObject();
    }
    else
    {
        bool IsNull = false;
        if (!SerializeValue(Name + "_IsNull", IsNull))
        {
            return false;
        }
        
        if (IsNull)
        {
            return true;
        }
        
        if (!BeginObject(Name))
        {
            return false;
        }
        
        if (Object)
        {
            Object->Deserialize(*this);
        }
        else
        {
            AddError("Cannot deserialize to null ISerializable object");
            return false;
        }
        
        return EndObject();
    }
}

bool CArchive::SerializeNObject(const CString& Name, TSharedPtr<CObject>& Object)
{
    return NSerializationUtils::SerializeObject(*this, Name, Object);
}

bool CArchive::SerializeNObject(const CString& Name, CObject* Object)
{
    return NSerializationUtils::SerializeObject(*this, Name, Object);
}

bool CArchive::SerializeVersion(const CString& Name, uint32_t& Version)
{
    return SerializeValue(Name, Version);
}

bool CArchive::CheckVersion(uint32_t RequiredVersion, uint32_t MaxVersion)
{
    uint32_t CurrentVersion = Context->GetVersion();
    
    if (CurrentVersion < RequiredVersion || CurrentVersion > MaxVersion)
    {
        AddError(CString::Format(
            "Version check failed: Current={}, Required={}, Max={}",
            CurrentVersion, RequiredVersion, MaxVersion
        ));
        return false;
    }
    
    return true;
}

bool CArchive::SerializeConditional(const CString& Name, bool Condition, NFunction<bool()> SerializeFunc)
{
    if (IsWriting())
    {
        if (!SerializeValue(Name + "_HasData", Condition))
        {
            return false;
        }
        
        if (Condition)
        {
            return SerializeFunc();
        }
    }
    else
    {
        bool HasData = false;
        if (!SerializeValue(Name + "_HasData", HasData))
        {
            return false;
        }
        
        if (HasData)
        {
            return SerializeFunc();
        }
    }
    
    return true;
}

void CArchive::AddError(const CString& Error)
{
    Context->AddError(Error);
}

void CArchive::AddWarning(const CString& Warning)
{
    Context->AddWarning(Warning);
}

bool CArchive::HasErrors() const
{
    return Context->HasErrors();
}

bool CArchive::HasWarnings() const
{
    return Context->HasWarnings();
}

const CArray<CString>& CArchive::GetErrors() const
{
    return Context->GetErrors();
}

const CArray<CString>& CArchive::GetWarnings() const
{
    return Context->GetWarnings();
}

// === NBinaryArchive 实现 ===

NBinaryArchive::NBinaryArchive(TSharedPtr<CSerializationContext> Context, TSharedPtr<NFileStream> InStream)
    : CArchive(Context)
    , Stream(InStream)
    , bCompressionEnabled(false)
{
    if (Context)
    {
        Context->SetFormat(ESerializationFormat::Binary);
    }
}

NBinaryArchive::~NBinaryArchive()
{
}

bool NBinaryArchive::SerializeValue(const CString& Name, bool& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, int8_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, uint8_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, int16_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, uint16_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, int32_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, uint32_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, int64_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, uint64_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, float& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, double& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NBinaryArchive::SerializeValue(const CString& Name, CString& Value)
{
    return IsWriting() ? WriteString(Value) : ReadString(Value);
}

bool NBinaryArchive::BeginObject(const CString& Name)
{
    // 二进制格式不需要对象标记
    return true;
}

bool NBinaryArchive::EndObject()
{
    return true;
}

bool NBinaryArchive::BeginArray(const CString& Name, int32_t& ElementCount)
{
    return SerializeValue("ArraySize", ElementCount);
}

bool NBinaryArchive::EndArray()
{
    return true;
}

bool NBinaryArchive::BeginArrayElement(int32_t Index)
{
    return true;
}

bool NBinaryArchive::EndArrayElement()
{
    return true;
}

bool NBinaryArchive::SerializeRawData(const CString& Name, void* Data, size_t Size)
{
    if (IsWriting())
    {
        return WriteRawBytes(Data, Size);
    }
    else
    {
        return ReadRawBytes(Data, Size);
    }
}

bool NBinaryArchive::SerializeRawData(const CString& Name, CArray<uint8_t>& Data)
{
    if (IsWriting())
    {
        int32_t Size = Data.GetSize();
        if (!WriteValue(Size))
        {
            return false;
        }
        
        if (Size > 0)
        {
            return WriteRawBytes(Data.GetData(), Size);
        }
    }
    else
    {
        int32_t Size = 0;
        if (!ReadValue(Size))
        {
            return false;
        }
        
        if (Size < 0 || Size > 1024 * 1024 * 100) // 限制100MB
        {
            AddError("Invalid data size in binary stream");
            return false;
        }
        
        Data.Resize(Size);
        if (Size > 0)
        {
            return ReadRawBytes(Data.GetData(), Size);
        }
    }
    
    return true;
}

bool NBinaryArchive::IsStreamValid() const
{
    return Stream && Stream->IsOpen();
}

bool NBinaryArchive::SerializeArrayBegin(const CString& Name, int32_t ElementCount)
{
    return BeginArray(Name, ElementCount);
}

bool NBinaryArchive::SerializeArrayEnd()
{
    return EndArray();
}

bool NBinaryArchive::SerializeArrayElementBegin(int32_t Index)
{
    return BeginArrayElement(Index);
}

bool NBinaryArchive::SerializeArrayElementEnd()
{
    return EndArrayElement();
}

template<typename TType>
bool NBinaryArchive::WriteValue(const TType& Value)
{
    if (!IsStreamValid())
    {
        AddError("Invalid stream for writing");
        return false;
    }
    
    size_t BytesWritten = Stream->Write(&Value, sizeof(TType));
    UpdateBytesProcessed(BytesWritten);
    
    return BytesWritten == sizeof(TType);
}

template<typename TType>
bool NBinaryArchive::ReadValue(TType& Value)
{
    if (!IsStreamValid())
    {
        AddError("Invalid stream for reading");
        return false;
    }
    
    size_t BytesRead = Stream->Read(&Value, sizeof(TType));
    UpdateBytesProcessed(BytesRead);
    
    return BytesRead == sizeof(TType);
}

bool NBinaryArchive::WriteString(const CString& Value)
{
    int32_t Length = Value.GetLength();
    if (!WriteValue(Length))
    {
        return false;
    }
    
    if (Length > 0)
    {
        const char* Data = Value.GetCStr();
        return WriteRawBytes(Data, Length);
    }
    
    return true;
}

bool NBinaryArchive::ReadString(CString& Value)
{
    int32_t Length = 0;
    if (!ReadValue(Length))
    {
        return false;
    }
    
    if (Length < 0 || Length > 1024 * 1024) // 限制1MB字符串
    {
        AddError("Invalid string length in binary stream");
        return false;
    }
    
    if (Length == 0)
    {
        Value.Clear();
        return true;
    }
    
    CArray<char> Buffer(Length + 1);
    if (!ReadRawBytes(Buffer.GetData(), Length))
    {
        return false;
    }
    
    Buffer[Length] = '\0';
    Value = CString(Buffer.GetData());
    
    return true;
}

bool NBinaryArchive::WriteRawBytes(const void* Data, size_t Size)
{
    if (!IsStreamValid() || !Data || Size == 0)
    {
        return false;
    }
    
    size_t BytesWritten = Stream->Write(Data, Size);
    UpdateBytesProcessed(BytesWritten);
    
    return BytesWritten == Size;
}

bool NBinaryArchive::ReadRawBytes(void* Data, size_t Size)
{
    if (!IsStreamValid() || !Data || Size == 0)
    {
        return false;
    }
    
    size_t BytesRead = Stream->Read(Data, Size);
    UpdateBytesProcessed(BytesRead);
    
    return BytesRead == Size;
}

// === NJsonArchive 实现 ===

NJsonArchive::NJsonArchive(TSharedPtr<CSerializationContext> Context)
    : CArchive(Context)
    , RootValue(nullptr)
    , CurrentValue(nullptr)
    , bPrettyPrint(true)
    , IndentSize(2)
    , CurrentArrayIndex(-1)
{
    if (Context)
    {
        Context->SetFormat(ESerializationFormat::JSON);
    }
    
    // 简化实现：这里应该创建实际的JSON值对象
    // 当前只是占位符实现
}

NJsonArchive::NJsonArchive(TSharedPtr<CSerializationContext> Context, const CString& JsonString)
    : NJsonArchive(Context)
{
    FromJsonString(JsonString);
}

NJsonArchive::~NJsonArchive()
{
    // 清理JSON值对象
}

bool NJsonArchive::SerializeValue(const CString& Name, bool& Value)
{
    // 简化实现：实际需要JSON库支持
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, int8_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, uint8_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, int16_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, uint16_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, int32_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, uint32_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, int64_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, uint64_t& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, float& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, double& Value)
{
    return true;
}

bool NJsonArchive::SerializeValue(const CString& Name, CString& Value)
{
    return true;
}

bool NJsonArchive::BeginObject(const CString& Name)
{
    return true;
}

bool NJsonArchive::EndObject()
{
    return true;
}

bool NJsonArchive::BeginArray(const CString& Name, int32_t& ElementCount)
{
    return true;
}

bool NJsonArchive::EndArray()
{
    return true;
}

bool NJsonArchive::BeginArrayElement(int32_t Index)
{
    CurrentArrayIndex = Index;
    return true;
}

bool NJsonArchive::EndArrayElement()
{
    return true;
}

bool NJsonArchive::SerializeRawData(const CString& Name, void* Data, size_t Size)
{
    if (IsWriting())
    {
        CString Encoded = EncodeBase64(Data, Size);
        return SerializeValue(Name, Encoded);
    }
    else
    {
        CString Encoded;
        if (SerializeValue(Name, Encoded))
        {
            CArray<uint8_t> Decoded = DecodeBase64(Encoded);
            if (Decoded.GetSize() == Size)
            {
                memcpy(Data, Decoded.GetData(), Size);
                return true;
            }
        }
        return false;
    }
}

bool NJsonArchive::SerializeRawData(const CString& Name, CArray<uint8_t>& Data)
{
    if (IsWriting())
    {
        CString Encoded = EncodeBase64(Data.GetData(), Data.GetSize());
        return SerializeValue(Name, Encoded);
    }
    else
    {
        CString Encoded;
        if (SerializeValue(Name, Encoded))
        {
            Data = DecodeBase64(Encoded);
            return true;
        }
        return false;
    }
}

CString NJsonArchive::ToJsonString(bool Pretty) const
{
    // 简化实现：应该序列化JSON对象为字符串
    return "{}";
}

bool NJsonArchive::FromJsonString(const CString& JsonString)
{
    // 简化实现：应该解析JSON字符串
    return !JsonString.IsEmpty();
}

bool NJsonArchive::SerializeArrayBegin(const CString& Name, int32_t ElementCount)
{
    return BeginArray(Name, ElementCount);
}

bool NJsonArchive::SerializeArrayEnd()
{
    return EndArray();
}

bool NJsonArchive::SerializeArrayElementBegin(int32_t Index)
{
    return BeginArrayElement(Index);
}

bool NJsonArchive::SerializeArrayElementEnd()
{
    return EndArrayElement();
}

CString NJsonArchive::EncodeBase64(const void* Data, size_t Size) const
{
    // 简化的Base64编码实现
    static const char* Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    CString Result;
    const uint8_t* Bytes = static_cast<const uint8_t*>(Data);
    
    for (size_t i = 0; i < Size; i += 3)
    {
        uint32_t Triple = 0;
        int32_t PadCount = 0;
        
        for (int32_t j = 0; j < 3; ++j)
        {
            Triple <<= 8;
            if (i + j < Size)
            {
                Triple |= Bytes[i + j];
            }
            else
            {
                PadCount++;
            }
        }
        
        for (int32_t j = 3; j >= 0; --j)
        {
            if (j <= PadCount)
            {
                Result += '=';
            }
            else
            {
                Result += Base64Chars[(Triple >> (6 * j)) & 0x3F];
            }
        }
    }
    
    return Result;
}

CArray<uint8_t> NJsonArchive::DecodeBase64(const CString& EncodedData) const
{
    // 简化的Base64解码实现
    CArray<uint8_t> Result;
    
    // 实际实现需要完整的Base64解码逻辑
    // 这里只是占位符
    
    return Result;
}

// === NMemoryArchive 实现 ===

NMemoryArchive::NMemoryArchive(TSharedPtr<CSerializationContext> Context)
    : CArchive(Context)
    , Position(0)
{
    if (Context)
    {
        Context->SetFormat(ESerializationFormat::Binary);
    }
}

NMemoryArchive::NMemoryArchive(TSharedPtr<CSerializationContext> Context, const CArray<uint8_t>& InData)
    : CArchive(Context)
    , Data(InData)
    , Position(0)
{
    if (Context)
    {
        Context->SetFormat(ESerializationFormat::Binary);
    }
}

NMemoryArchive::~NMemoryArchive()
{
}

// NMemoryArchive的具体实现委托给内存操作
bool NMemoryArchive::SerializeValue(const CString& Name, bool& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, int8_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, uint8_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, int16_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, uint16_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, int32_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, uint32_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, int64_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, uint64_t& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, float& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, double& Value)
{
    return IsWriting() ? WriteValue(Value) : ReadValue(Value);
}

bool NMemoryArchive::SerializeValue(const CString& Name, CString& Value)
{
    if (IsWriting())
    {
        int32_t Length = Value.GetLength();
        if (!WriteValue(Length))
        {
            return false;
        }
        
        if (Length > 0)
        {
            return WriteBytes(Value.GetCStr(), Length);
        }
    }
    else
    {
        int32_t Length = 0;
        if (!ReadValue(Length))
        {
            return false;
        }
        
        if (Length < 0 || Length > 1024 * 1024)
        {
            AddError("Invalid string length in memory archive");
            return false;
        }
        
        if (Length == 0)
        {
            Value.Clear();
            return true;
        }
        
        CArray<char> Buffer(Length + 1);
        if (!ReadBytes(Buffer.GetData(), Length))
        {
            return false;
        }
        
        Buffer[Length] = '\0';
        Value = CString(Buffer.GetData());
    }
    
    return true;
}

bool NMemoryArchive::BeginObject(const CString& Name)
{
    return true;
}

bool NMemoryArchive::EndObject()
{
    return true;
}

bool NMemoryArchive::BeginArray(const CString& Name, int32_t& ElementCount)
{
    return SerializeValue("ArraySize", ElementCount);
}

bool NMemoryArchive::EndArray()
{
    return true;
}

bool NMemoryArchive::BeginArrayElement(int32_t Index)
{
    return true;
}

bool NMemoryArchive::EndArrayElement()
{
    return true;
}

bool NMemoryArchive::SerializeRawData(const CString& Name, void* DataPtr, size_t Size)
{
    return IsWriting() ? WriteBytes(DataPtr, Size) : ReadBytes(DataPtr, Size);
}

bool NMemoryArchive::SerializeRawData(const CString& Name, CArray<uint8_t>& DataArray)
{
    if (IsWriting())
    {
        int32_t Size = DataArray.GetSize();
        if (!WriteValue(Size))
        {
            return false;
        }
        
        if (Size > 0)
        {
            return WriteBytes(DataArray.GetData(), Size);
        }
    }
    else
    {
        int32_t Size = 0;
        if (!ReadValue(Size))
        {
            return false;
        }
        
        if (Size < 0 || Size > 1024 * 1024 * 100)
        {
            AddError("Invalid data size in memory archive");
            return false;
        }
        
        DataArray.Resize(Size);
        if (Size > 0)
        {
            return ReadBytes(DataArray.GetData(), Size);
        }
    }
    
    return true;
}

void NMemoryArchive::SetData(const CArray<uint8_t>& InData)
{
    Data = InData;
    Position = 0;
}

void NMemoryArchive::ClearData()
{
    Data.Clear();
    Position = 0;
}

void NMemoryArchive::SetPosition(size_t InPosition)
{
    Position = NMath::Min(InPosition, Data.GetSize());
}

bool NMemoryArchive::SerializeArrayBegin(const CString& Name, int32_t ElementCount)
{
    return BeginArray(Name, ElementCount);
}

bool NMemoryArchive::SerializeArrayEnd()
{
    return EndArray();
}

bool NMemoryArchive::SerializeArrayElementBegin(int32_t Index)
{
    return BeginArrayElement(Index);
}

bool NMemoryArchive::SerializeArrayElementEnd()
{
    return EndArrayElement();
}

template<typename TType>
bool NMemoryArchive::WriteValue(const TType& Value)
{
    return WriteBytes(&Value, sizeof(TType));
}

template<typename TType>
bool NMemoryArchive::ReadValue(TType& Value)
{
    return ReadBytes(&Value, sizeof(TType));
}

bool NMemoryArchive::WriteBytes(const void* Bytes, size_t Size)
{
    if (!Bytes || Size == 0)
    {
        return true;
    }
    
    EnsureCapacity(Position + Size);
    
    memcpy(Data.GetData() + Position, Bytes, Size);
    Position += Size;
    UpdateBytesProcessed(Size);
    
    return true;
}

bool NMemoryArchive::ReadBytes(void* Bytes, size_t Size)
{
    if (!Bytes || Size == 0)
    {
        return true;
    }
    
    if (Position + Size > Data.GetSize())
    {
        AddError("Not enough data in memory archive");
        return false;
    }
    
    memcpy(Bytes, Data.GetData() + Position, Size);
    Position += Size;
    UpdateBytesProcessed(Size);
    
    return true;
}

void NMemoryArchive::EnsureCapacity(size_t RequiredSize)
{
    if (Data.GetSize() < RequiredSize)
    {
        size_t NewSize = NMath::Max(Data.GetSize() * 2, RequiredSize);
        Data.Resize(NewSize);
    }
}

} // namespace NLib