#include "Config/NConfig.h"

#include "Logging/CLogger.h"
#include "FileSystem/NFileSystem.h"

#include <cstdlib>
#include <cstring>

namespace NLib
{

// === CConfigValue 实现 ===

CConfigValue::CConfigValue()
    : Type(EConfigValueType::Null)
{
    memset(&BoolValue, 0, sizeof(BoolValue));
}

CConfigValue::CConfigValue(bool Value)
    : Type(EConfigValueType::Bool), BoolValue(Value)
{}

CConfigValue::CConfigValue(int32_t Value)
    : Type(EConfigValueType::Int), IntValue(static_cast<int64_t>(Value))
{}

CConfigValue::CConfigValue(int64_t Value)
    : Type(EConfigValueType::Int), IntValue(Value)
{}

CConfigValue::CConfigValue(float Value)
    : Type(EConfigValueType::Float), FloatValue(static_cast<double>(Value))
{}

CConfigValue::CConfigValue(double Value)
    : Type(EConfigValueType::Float), FloatValue(Value)
{}

CConfigValue::CConfigValue(const char* Value)
    : Type(EConfigValueType::String)
{
    StringValue = new CString(Value ? Value : "");
}

CConfigValue::CConfigValue(const CString& Value)
    : Type(EConfigValueType::String)
{
    StringValue = new CString(Value);
}

CConfigValue::CConfigValue(const CArray<CConfigValue>& Array)
    : Type(EConfigValueType::Array)
{
    ArrayValue = new CArray<CConfigValue>(Array);
}

CConfigValue::CConfigValue(const CHashMap<CString, CConfigValue>& Object)
    : Type(EConfigValueType::Object)
{
    ObjectValue = new CHashMap<CString, CConfigValue>(Object);
}

CConfigValue::CConfigValue(const CConfigValue& Other)
    : Type(EConfigValueType::Null)
{
    CopyFrom(Other);
}

CConfigValue::CConfigValue(CConfigValue&& Other) noexcept
    : Type(EConfigValueType::Null)
{
    MoveFrom(NLib::Move(Other));
}

CConfigValue& CConfigValue::operator=(const CConfigValue& Other)
{
    if (this != &Other)
    {
        DestroyValue();
        CopyFrom(Other);
    }
    return *this;
}

CConfigValue& CConfigValue::operator=(CConfigValue&& Other) noexcept
{
    if (this != &Other)
    {
        DestroyValue();
        MoveFrom(NLib::Move(Other));
    }
    return *this;
}

CConfigValue::~CConfigValue()
{
    DestroyValue();
}

EConfigValueType CConfigValue::GetType() const
{
    return Type;
}

bool CConfigValue::IsNull() const
{
    return Type == EConfigValueType::Null;
}

bool CConfigValue::IsBool() const
{
    return Type == EConfigValueType::Bool;
}

bool CConfigValue::IsNumber() const
{
    return Type == EConfigValueType::Int || Type == EConfigValueType::Float;
}

bool CConfigValue::IsString() const
{
    return Type == EConfigValueType::String;
}

bool CConfigValue::IsArray() const
{
    return Type == EConfigValueType::Array;
}

bool CConfigValue::IsObject() const
{
    return Type == EConfigValueType::Object;
}

bool CConfigValue::AsBool(bool DefaultValue) const
{
    switch (Type)
    {
    case EConfigValueType::Bool:
        return BoolValue;
    case EConfigValueType::Int:
        return IntValue != 0;
    case EConfigValueType::Float:
        return FloatValue != 0.0;
    case EConfigValueType::String:
        return StringValue && (StringValue->ToLower() == "true" || StringValue->ToLower() == "1");
    default:
        return DefaultValue;
    }
}

int32_t CConfigValue::AsInt(int32_t DefaultValue) const
{
    return static_cast<int32_t>(AsInt64(static_cast<int64_t>(DefaultValue)));
}

int64_t CConfigValue::AsInt64(int64_t DefaultValue) const
{
    switch (Type)
    {
    case EConfigValueType::Bool:
        return BoolValue ? 1 : 0;
    case EConfigValueType::Int:
        return IntValue;
    case EConfigValueType::Float:
        return static_cast<int64_t>(FloatValue);
    case EConfigValueType::String:
        if (StringValue)
        {
            // 简化的字符串转数字实现
            const char* Str = StringValue->GetCStr();
            char* EndPtr = nullptr;
            int64_t Result = strtoll(Str, &EndPtr, 10);
            return (EndPtr != Str) ? Result : DefaultValue;
        }
        return DefaultValue;
    default:
        return DefaultValue;
    }
}

float CConfigValue::AsFloat(float DefaultValue) const
{
    return static_cast<float>(AsDouble(static_cast<double>(DefaultValue)));
}

double CConfigValue::AsDouble(double DefaultValue) const
{
    switch (Type)
    {
    case EConfigValueType::Bool:
        return BoolValue ? 1.0 : 0.0;
    case EConfigValueType::Int:
        return static_cast<double>(IntValue);
    case EConfigValueType::Float:
        return FloatValue;
    case EConfigValueType::String:
        if (StringValue)
        {
            // 简化的字符串转浮点数实现
            const char* Str = StringValue->GetCStr();
            char* EndPtr = nullptr;
            double Result = strtod(Str, &EndPtr);
            return (EndPtr != Str) ? Result : DefaultValue;
        }
        return DefaultValue;
    default:
        return DefaultValue;
    }
}

CString CConfigValue::AsString(const CString& DefaultValue) const
{
    switch (Type)
    {
    case EConfigValueType::Bool:
        return BoolValue ? "true" : "false";
    case EConfigValueType::Int:
        return CString::Format("{}", IntValue);
    case EConfigValueType::Float:
        return CString::Format("{}", FloatValue);
    case EConfigValueType::String:
        return StringValue ? *StringValue : DefaultValue;
    default:
        return DefaultValue;
    }
}

const CConfigValue& CConfigValue::operator[](int32_t Index) const
{
    if (Type == EConfigValueType::Array && ArrayValue && Index >= 0 && Index < ArrayValue->GetSize())
    {
        return (*ArrayValue)[Index];
    }
    return GetNullValue();
}

CConfigValue& CConfigValue::operator[](int32_t Index)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Array;
        ArrayValue = new CArray<CConfigValue>();
    }
    
    if (Type == EConfigValueType::Array && ArrayValue)
    {
        // 自动扩展数组
        if (Index >= ArrayValue->GetSize())
        {
            ArrayValue->Resize(Index + 1);
        }
        return (*ArrayValue)[Index];
    }
    
    return const_cast<CConfigValue&>(GetNullValue());
}

const CConfigValue& CConfigValue::operator[](const CString& Key) const
{
    if (Type == EConfigValueType::Object && ObjectValue && ObjectValue->Contains(Key))
    {
        return (*ObjectValue)[Key];
    }
    return GetNullValue();
}

CConfigValue& CConfigValue::operator[](const CString& Key)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Object;
        ObjectValue = new CHashMap<CString, CConfigValue>();
    }
    
    if (Type == EConfigValueType::Object && ObjectValue)
    {
        return (*ObjectValue)[Key];
    }
    
    return const_cast<CConfigValue&>(GetNullValue());
}

void CConfigValue::PushBack(const CConfigValue& Value)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Array;
        ArrayValue = new CArray<CConfigValue>();
    }
    
    if (Type == EConfigValueType::Array && ArrayValue)
    {
        ArrayValue->PushBack(Value);
    }
}

void CConfigValue::PushBack(CConfigValue&& Value)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Array;
        ArrayValue = new CArray<CConfigValue>();
    }
    
    if (Type == EConfigValueType::Array && ArrayValue)
    {
        ArrayValue->PushBack(NLib::Move(Value));
    }
}

void CConfigValue::PopBack()
{
    if (Type == EConfigValueType::Array && ArrayValue && !ArrayValue->IsEmpty())
    {
        ArrayValue->PopBack();
    }
}

int32_t CConfigValue::GetArraySize() const
{
    return (Type == EConfigValueType::Array && ArrayValue) ? ArrayValue->GetSize() : 0;
}

void CConfigValue::ResizeArray(int32_t NewSize)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Array;
        ArrayValue = new CArray<CConfigValue>();
    }
    
    if (Type == EConfigValueType::Array && ArrayValue)
    {
        ArrayValue->Resize(NewSize);
    }
}

void CConfigValue::ClearArray()
{
    if (Type == EConfigValueType::Array && ArrayValue)
    {
        ArrayValue->Clear();
    }
}

bool CConfigValue::HasKey(const CString& Key) const
{
    return (Type == EConfigValueType::Object && ObjectValue) ? ObjectValue->Contains(Key) : false;
}

void CConfigValue::SetValue(const CString& Key, const CConfigValue& Value)
{
    if (Type == EConfigValueType::Null)
    {
        Type = EConfigValueType::Object;
        ObjectValue = new CHashMap<CString, CConfigValue>();
    }
    
    if (Type == EConfigValueType::Object && ObjectValue)
    {
        (*ObjectValue)[Key] = Value;
    }
}

void CConfigValue::RemoveKey(const CString& Key)
{
    if (Type == EConfigValueType::Object && ObjectValue)
    {
        ObjectValue->Remove(Key);
    }
}

CArray<CString> CConfigValue::GetKeys() const
{
    if (Type == EConfigValueType::Object && ObjectValue)
    {
        CArray<CString> Keys;
        for (const auto& Pair : *ObjectValue)
        {
            Keys.PushBack(Pair.Key);
        }
        return Keys;
    }
    return CArray<CString>();
}

int32_t CConfigValue::GetObjectSize() const
{
    return (Type == EConfigValueType::Object && ObjectValue) ? ObjectValue->GetSize() : 0;
}

void CConfigValue::ClearObject()
{
    if (Type == EConfigValueType::Object && ObjectValue)
    {
        ObjectValue->Clear();
    }
}

CConfigValue& CConfigValue::GetPath(const CString& Path)
{
    CArray<CString> PathParts = SplitPath(Path);
    CConfigValue* Current = this;
    
    for (const CString& Part : PathParts)
    {
        Current = &((*Current)[Part]);
    }
    
    return *Current;
}

const CConfigValue& CConfigValue::GetPath(const CString& Path) const
{
    CArray<CString> PathParts = SplitPath(Path);
    const CConfigValue* Current = this;
    
    for (const CString& Part : PathParts)
    {
        Current = &((*Current)[Part]);
        if (Current->IsNull())
        {
            break;
        }
    }
    
    return *Current;
}

void CConfigValue::SetPath(const CString& Path, const CConfigValue& Value)
{
    CArray<CString> PathParts = SplitPath(Path);
    if (PathParts.IsEmpty())
    {
        return;
    }
    
    CConfigValue* Current = this;
    for (int32_t i = 0; i < PathParts.GetSize() - 1; ++i)
    {
        Current = &((*Current)[PathParts[i]]);
    }
    
    (*Current)[PathParts.GetLast()] = Value;
}

bool CConfigValue::HasPath(const CString& Path) const
{
    return !GetPath(Path).IsNull();
}

void CConfigValue::Clear()
{
    DestroyValue();
    Type = EConfigValueType::Null;
}

bool CConfigValue::IsEmpty() const
{
    switch (Type)
    {
    case EConfigValueType::Null:
        return true;
    case EConfigValueType::String:
        return !StringValue || StringValue->IsEmpty();
    case EConfigValueType::Array:
        return !ArrayValue || ArrayValue->IsEmpty();
    case EConfigValueType::Object:
        return !ObjectValue || ObjectValue->IsEmpty();
    default:
        return false;
    }
}

CString CConfigValue::ToString() const
{
    return AsString();
}

CString CConfigValue::ToJsonString(bool Pretty) const
{
    // 这里简化实现，实际应该使用专门的JSON序列化器
    switch (Type)
    {
    case EConfigValueType::Null:
        return "null";
    case EConfigValueType::Bool:
        return BoolValue ? "true" : "false";
    case EConfigValueType::Int:
        return CString::Format("{}", IntValue);
    case EConfigValueType::Float:
        return CString::Format("{}", FloatValue);
    case EConfigValueType::String:
        return CString::Format("\"{}\"", StringValue ? *StringValue : "");
    case EConfigValueType::Array:
        {
            CString Result = "[";
            if (ArrayValue)
            {
                for (int32_t i = 0; i < ArrayValue->GetSize(); ++i)
                {
                    if (i > 0) Result += ",";
                    if (Pretty) Result += " ";
                    Result += (*ArrayValue)[i].ToJsonString(Pretty);
                }
            }
            if (Pretty && ArrayValue && !ArrayValue->IsEmpty()) Result += " ";
            Result += "]";
            return Result;
        }
    case EConfigValueType::Object:
        {
            CString Result = "{";
            if (ObjectValue)
            {
                bool First = true;
                for (const auto& Pair : *ObjectValue)
                {
                    if (!First) Result += ",";
                    if (Pretty) Result += " ";
                    Result += CString::Format("\"{}\":", Pair.Key);
                    if (Pretty) Result += " ";
                    Result += Pair.Value.ToJsonString(Pretty);
                    First = false;
                }
            }
            if (Pretty && ObjectValue && !ObjectValue->IsEmpty()) Result += " ";
            Result += "}";
            return Result;
        }
    default:
        return "null";
    }
}

bool CConfigValue::operator==(const CConfigValue& Other) const
{
    if (Type != Other.Type)
    {
        return false;
    }
    
    switch (Type)
    {
    case EConfigValueType::Null:
        return true;
    case EConfigValueType::Bool:
        return BoolValue == Other.BoolValue;
    case EConfigValueType::Int:
        return IntValue == Other.IntValue;
    case EConfigValueType::Float:
        return FloatValue == Other.FloatValue;
    case EConfigValueType::String:
        return StringValue && Other.StringValue && (*StringValue == *Other.StringValue);
    case EConfigValueType::Array:
        return ArrayValue && Other.ArrayValue && (*ArrayValue == *Other.ArrayValue);
    case EConfigValueType::Object:
        return ObjectValue && Other.ObjectValue && (*ObjectValue == *Other.ObjectValue);
    default:
        return false;
    }
}

bool CConfigValue::operator!=(const CConfigValue& Other) const
{
    return !(*this == Other);
}

void CConfigValue::Merge(const CConfigValue& Other, bool Overwrite)
{
    if (Other.Type == EConfigValueType::Object && Type == EConfigValueType::Object && ObjectValue && Other.ObjectValue)
    {
        // 递归合并对象
        for (const auto& Pair : *Other.ObjectValue)
        {
            if (ObjectValue->Contains(Pair.Key))
            {
                if (Overwrite)
                {
                    (*ObjectValue)[Pair.Key].Merge(Pair.Value, Overwrite);
                }
            }
            else
            {
                (*ObjectValue)[Pair.Key] = Pair.Value;
            }
        }
    }
    else if (Overwrite || Type == EConfigValueType::Null)
    {
        *this = Other;
    }
}

CConfigValue CConfigValue::CreateNull()
{
    return CConfigValue();
}

CConfigValue CConfigValue::CreateArray()
{
    CConfigValue Value;
    Value.Type = EConfigValueType::Array;
    Value.ArrayValue = new CArray<CConfigValue>();
    return Value;
}

CConfigValue CConfigValue::CreateObject()
{
    CConfigValue Value;
    Value.Type = EConfigValueType::Object;
    Value.ObjectValue = new CHashMap<CString, CConfigValue>();
    return Value;
}

CConfigValue CConfigValue::FromJsonString(const CString& JsonString)
{
    // 这里应该使用专门的JSON解析器，简化实现
    CConfigValue Result;
    // TODO: 实现JSON解析
    return Result;
}

void CConfigValue::DestroyValue()
{
    switch (Type)
    {
    case EConfigValueType::String:
        delete StringValue;
        StringValue = nullptr;
        break;
    case EConfigValueType::Array:
        delete ArrayValue;
        ArrayValue = nullptr;
        break;
    case EConfigValueType::Object:
        delete ObjectValue;
        ObjectValue = nullptr;
        break;
    default:
        break;
    }
}

void CConfigValue::CopyFrom(const CConfigValue& Other)
{
    Type = Other.Type;
    
    switch (Type)
    {
    case EConfigValueType::Bool:
        BoolValue = Other.BoolValue;
        break;
    case EConfigValueType::Int:
        IntValue = Other.IntValue;
        break;
    case EConfigValueType::Float:
        FloatValue = Other.FloatValue;
        break;
    case EConfigValueType::String:
        StringValue = Other.StringValue ? new CString(*Other.StringValue) : new CString();
        break;
    case EConfigValueType::Array:
        ArrayValue = Other.ArrayValue ? new CArray<CConfigValue>(*Other.ArrayValue) : new CArray<CConfigValue>();
        break;
    case EConfigValueType::Object:
        ObjectValue = Other.ObjectValue ? new CHashMap<CString, CConfigValue>(*Other.ObjectValue) : new CHashMap<CString, CConfigValue>();
        break;
    default:
        break;
    }
}

void CConfigValue::MoveFrom(CConfigValue&& Other)
{
    Type = Other.Type;
    
    switch (Type)
    {
    case EConfigValueType::Bool:
        BoolValue = Other.BoolValue;
        break;
    case EConfigValueType::Int:
        IntValue = Other.IntValue;
        break;
    case EConfigValueType::Float:
        FloatValue = Other.FloatValue;
        break;
    case EConfigValueType::String:
        StringValue = Other.StringValue;
        Other.StringValue = nullptr;
        break;
    case EConfigValueType::Array:
        ArrayValue = Other.ArrayValue;
        Other.ArrayValue = nullptr;
        break;
    case EConfigValueType::Object:
        ObjectValue = Other.ObjectValue;
        Other.ObjectValue = nullptr;
        break;
    default:
        break;
    }
    
    Other.Type = EConfigValueType::Null;
}

CArray<CString> CConfigValue::SplitPath(const CString& Path) const
{
    CArray<CString> Parts;
    CString Current;
    
    for (int32_t i = 0; i < Path.GetLength(); ++i)
    {
        char Ch = Path[i];
        if (Ch == '.')
        {
            if (!Current.IsEmpty())
            {
                Parts.PushBack(Current);
                Current.Clear();
            }
        }
        else
        {
            Current += Ch;
        }
    }
    
    if (!Current.IsEmpty())
    {
        Parts.PushBack(Current);
    }
    
    return Parts;
}

const CConfigValue& CConfigValue::GetNullValue()
{
    static CConfigValue NullValue;
    return NullValue;
}

// === NJsonConfigLoader 实现 ===

NJsonConfigLoader::NJsonConfigLoader()
    : bPrettyPrint(true)
{}

NJsonConfigLoader::~NJsonConfigLoader()
{}

bool NJsonConfigLoader::CanLoad(const CString& FilePath) const
{
    CString Extension = NPath::GetExtension(FilePath).ToLower();
    return Extension == ".json";
}

bool NJsonConfigLoader::Load(const CString& FilePath, CConfigValue& OutConfig)
{
    if (!NFile::Exists(FilePath))
    {
        CLogger::Error("NJsonConfigLoader: File does not exist: {}", FilePath);
        return false;
    }
    
    CString JsonContent = NFile::ReadAllText(FilePath);
    if (JsonContent.IsEmpty())
    {
        CLogger::Warning("NJsonConfigLoader: Empty file: {}", FilePath);
        OutConfig = CConfigValue::CreateObject();
        return true;
    }
    
    int32_t Index = 0;
    bool Success = ParseJsonValue(JsonContent, Index, OutConfig);
    
    if (!Success)
    {
        CLogger::Error("NJsonConfigLoader: Failed to parse JSON file: {}", FilePath);
        return false;
    }
    
    return true;
}

bool NJsonConfigLoader::Save(const CString& FilePath, const CConfigValue& Config)
{
    CString JsonContent = Config.ToJsonString(bPrettyPrint);
    
    try
    {
        NFile::WriteAllText(FilePath, JsonContent);
        return true;
    }
    catch (...)
    {
        CLogger::Error("NJsonConfigLoader: Failed to save JSON file: {}", FilePath);
        return false;
    }
}

CString NJsonConfigLoader::GetSupportedExtensions() const
{
    return ".json";
}

void NJsonConfigLoader::SetPrettyPrint(bool bPretty)
{
    bPrettyPrint = bPretty;
}

bool NJsonConfigLoader::GetPrettyPrint() const
{
    return bPrettyPrint;
}

bool NJsonConfigLoader::ParseJsonValue(const CString& JsonText, int32_t& Index, CConfigValue& OutValue)
{
    SkipWhitespace(JsonText, Index);
    
    if (Index >= JsonText.GetLength())
    {
        return false;
    }
    
    char Ch = JsonText[Index];
    
    if (Ch == '{')
    {
        return ParseJsonObject(JsonText, Index, OutValue);
    }
    else if (Ch == '[')
    {
        return ParseJsonArray(JsonText, Index, OutValue);
    }
    else if (Ch == '"')
    {
        CString StringValue;
        if (ParseJsonString(JsonText, Index, StringValue))
        {
            OutValue = CConfigValue(StringValue);
            return true;
        }
        return false;
    }
    else if (Ch == 't' || Ch == 'f' || Ch == 'n')
    {
        return ParseJsonKeyword(JsonText, Index, OutValue);
    }
    else if (Ch == '-' || (Ch >= '0' && Ch <= '9'))
    {
        return ParseJsonNumber(JsonText, Index, OutValue);
    }
    
    return false;
}

bool NJsonConfigLoader::ParseJsonObject(const CString& JsonText, int32_t& Index, CConfigValue& OutValue)
{
    if (Index >= JsonText.GetLength() || JsonText[Index] != '{')
    {
        return false;
    }
    
    Index++; // Skip '{'
    OutValue = CConfigValue::CreateObject();
    
    SkipWhitespace(JsonText, Index);
    
    // Handle empty object
    if (Index < JsonText.GetLength() && JsonText[Index] == '}')
    {
        Index++;
        return true;
    }
    
    while (Index < JsonText.GetLength())
    {
        SkipWhitespace(JsonText, Index);
        
        // Parse key
        if (Index >= JsonText.GetLength() || JsonText[Index] != '"')
        {
            return false;
        }
        
        CString Key;
        if (!ParseJsonString(JsonText, Index, Key))
        {
            return false;
        }
        
        SkipWhitespace(JsonText, Index);
        
        // Expect ':'
        if (Index >= JsonText.GetLength() || JsonText[Index] != ':')
        {
            return false;
        }
        
        Index++; // Skip ':'
        
        // Parse value
        CConfigValue Value;
        if (!ParseJsonValue(JsonText, Index, Value))
        {
            return false;
        }
        
        OutValue.SetValue(Key, Value);
        
        SkipWhitespace(JsonText, Index);
        
        if (Index >= JsonText.GetLength())
        {
            return false;
        }
        
        if (JsonText[Index] == '}')
        {
            Index++;
            return true;
        }
        else if (JsonText[Index] == ',')
        {
            Index++;
        }
        else
        {
            return false;
        }
    }
    
    return false;
}

bool NJsonConfigLoader::ParseJsonArray(const CString& JsonText, int32_t& Index, CConfigValue& OutValue)
{
    if (Index >= JsonText.GetLength() || JsonText[Index] != '[')
    {
        return false;
    }
    
    Index++; // Skip '['
    OutValue = CConfigValue::CreateArray();
    
    SkipWhitespace(JsonText, Index);
    
    // Handle empty array
    if (Index < JsonText.GetLength() && JsonText[Index] == ']')
    {
        Index++;
        return true;
    }
    
    while (Index < JsonText.GetLength())
    {
        CConfigValue Value;
        if (!ParseJsonValue(JsonText, Index, Value))
        {
            return false;
        }
        
        OutValue.PushBack(Value);
        
        SkipWhitespace(JsonText, Index);
        
        if (Index >= JsonText.GetLength())
        {
            return false;
        }
        
        if (JsonText[Index] == ']')
        {
            Index++;
            return true;
        }
        else if (JsonText[Index] == ',')
        {
            Index++;
            SkipWhitespace(JsonText, Index);
        }
        else
        {
            return false;
        }
    }
    
    return false;
}

bool NJsonConfigLoader::ParseJsonString(const CString& JsonText, int32_t& Index, CString& OutString)
{
    if (Index >= JsonText.GetLength() || JsonText[Index] != '"')
    {
        return false;
    }
    
    Index++; // Skip opening '"'
    OutString.Clear();
    
    while (Index < JsonText.GetLength())
    {
        char Ch = JsonText[Index];
        
        if (Ch == '"')
        {
            Index++; // Skip closing '"'
            return true;
        }
        else if (Ch == '\\')
        {
            Index++;
            if (Index >= JsonText.GetLength())
            {
                return false;
            }
            
            char EscapeChar = JsonText[Index];
            switch (EscapeChar)
            {
            case '"':
                OutString += '"';
                break;
            case '\\':
                OutString += '\\';
                break;
            case '/':
                OutString += '/';
                break;
            case 'b':
                OutString += '\b';
                break;
            case 'f':
                OutString += '\f';
                break;
            case 'n':
                OutString += '\n';
                break;
            case 'r':
                OutString += '\r';
                break;
            case 't':
                OutString += '\t';
                break;
            default:
                OutString += EscapeChar;
                break;
            }
        }
        else
        {
            OutString += Ch;
        }
        
        Index++;
    }
    
    return false; // Unclosed string
}

bool NJsonConfigLoader::ParseJsonNumber(const CString& JsonText, int32_t& Index, CConfigValue& OutValue)
{
    int32_t StartIndex = Index;
    bool HasDecimalPoint = false;
    
    // Handle negative sign
    if (Index < JsonText.GetLength() && JsonText[Index] == '-')
    {
        Index++;
    }
    
    // Parse digits
    while (Index < JsonText.GetLength())
    {
        char Ch = JsonText[Index];
        
        if (Ch >= '0' && Ch <= '9')
        {
            Index++;
        }
        else if (Ch == '.' && !HasDecimalPoint)
        {
            HasDecimalPoint = true;
            Index++;
        }
        else
        {
            break;
        }
    }
    
    if (Index == StartIndex || (JsonText[StartIndex] == '-' && Index == StartIndex + 1))
    {
        return false;
    }
    
    CString NumberStr = JsonText.Substring(StartIndex, Index - StartIndex);
    
    if (HasDecimalPoint)
    {
        double Value = strtod(NumberStr.GetCStr(), nullptr);
        OutValue = CConfigValue(Value);
    }
    else
    {
        int64_t Value = strtoll(NumberStr.GetCStr(), nullptr, 10);
        OutValue = CConfigValue(Value);
    }
    
    return true;
}

bool NJsonConfigLoader::ParseJsonKeyword(const CString& JsonText, int32_t& Index, CConfigValue& OutValue)
{
    if (Index + 4 <= JsonText.GetLength() && JsonText.Substring(Index, 4) == "true")
    {
        Index += 4;
        OutValue = CConfigValue(true);
        return true;
    }
    else if (Index + 5 <= JsonText.GetLength() && JsonText.Substring(Index, 5) == "false")
    {
        Index += 5;
        OutValue = CConfigValue(false);
        return true;
    }
    else if (Index + 4 <= JsonText.GetLength() && JsonText.Substring(Index, 4) == "null")
    {
        Index += 4;
        OutValue = CConfigValue::CreateNull();
        return true;
    }
    
    return false;
}

void NJsonConfigLoader::SkipWhitespace(const CString& JsonText, int32_t& Index)
{
    while (Index < JsonText.GetLength())
    {
        char Ch = JsonText[Index];
        if (Ch == ' ' || Ch == '\t' || Ch == '\n' || Ch == '\r')
        {
            Index++;
        }
        else
        {
            break;
        }
    }
}

CString NJsonConfigLoader::EscapeJsonString(const CString& Input) const
{
    CString Result;
    for (int32_t i = 0; i < Input.GetLength(); ++i)
    {
        char Ch = Input[i];
        switch (Ch)
        {
        case '"':
            Result += "\\\"";
            break;
        case '\\':
            Result += "\\\\";
            break;
        case '\b':
            Result += "\\b";
            break;
        case '\f':
            Result += "\\f";
            break;
        case '\n':
            Result += "\\n";
            break;
        case '\r':
            Result += "\\r";
            break;
        case '\t':
            Result += "\\t";
            break;
        default:
            Result += Ch;
            break;
        }
    }
    return Result;
}

CString NJsonConfigLoader::UnescapeJsonString(const CString& Input) const
{
    CString Result;
    for (int32_t i = 0; i < Input.GetLength(); ++i)
    {
        char Ch = Input[i];
        if (Ch == '\\' && i + 1 < Input.GetLength())
        {
            i++;
            char NextCh = Input[i];
            switch (NextCh)
            {
            case '"':
                Result += '"';
                break;
            case '\\':
                Result += '\\';
                break;
            case 'b':
                Result += '\b';
                break;
            case 'f':
                Result += '\f';
                break;
            case 'n':
                Result += '\n';
                break;
            case 'r':
                Result += '\r';
                break;
            case 't':
                Result += '\t';
                break;
            default:
                Result += NextCh;
                break;
            }
        }
        else
        {
            Result += Ch;
        }
    }
    return Result;
}

CString NJsonConfigLoader::SerializeJsonValue(const CConfigValue& Value, int32_t IndentLevel) const
{
    // 使用NConfigValue的ToJsonString方法
    return Value.ToJsonString(bPrettyPrint);
}

// === NConfigWatcher 实现 ===

NConfigWatcher::NConfigWatcher()
{}

NConfigWatcher::~NConfigWatcher()
{}

void NConfigWatcher::Watch(const CString& KeyPath, ConfigChangeCallback Callback)
{
    if (!Watchers.Contains(KeyPath))
    {
        Watchers[KeyPath] = CArray<ConfigChangeCallback>();
    }
    Watchers[KeyPath].PushBack(Callback);
}

void NConfigWatcher::Unwatch(const CString& KeyPath)
{
    Watchers.Remove(KeyPath);
}

void NConfigWatcher::UnwatchAll()
{
    Watchers.Clear();
}

void NConfigWatcher::NotifyChange(const CString& KeyPath, const CConfigValue& OldValue, const CConfigValue& NewValue)
{
    if (Watchers.Contains(KeyPath))
    {
        for (const auto& Callback : Watchers[KeyPath])
        {
            try
            {
                Callback.ExecuteIfBound(KeyPath, OldValue, NewValue);
            }
            catch (...)
            {
                CLogger::Error("NConfigWatcher: Exception in config change callback for key: {}", KeyPath);
            }
        }
    }
}

bool NConfigWatcher::IsWatching(const CString& KeyPath) const
{
    return Watchers.Contains(KeyPath);
}

int32_t NConfigWatcher::GetWatchCount() const
{
    return Watchers.GetSize();
}

} // namespace NLib