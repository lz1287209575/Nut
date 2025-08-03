#include "Serialization/NSerializable.h"
#include "Core/CLogger.h"
#include "Core/CAllocator.h"

namespace NLib
{

// === CSerializationContext 实现 ===

CSerializationContext::CSerializationContext()
    : Format(ESerializationFormat::Binary)
    , Mode(ESerializationMode::Writing)
    , Version(1)
    , NextObjectId(1)
{
}

CSerializationContext::~CSerializationContext()
{
}

void CSerializationContext::RegisterObject(CObject* Object, uint64_t ObjectId)
{
    if (!Object)
    {
        return;
    }
    
    ObjectIdMap[ObjectId] = Object->AsWeak();
    ObjectToIdMap[Object] = ObjectId;
}

CObject* CSerializationContext::FindObject(uint64_t ObjectId) const
{
    auto It = ObjectIdMap.Find(ObjectId);
    if (It != ObjectIdMap.End())
    {
        return It->Value.Lock().Get();
    }
    return nullptr;
}

uint64_t CSerializationContext::GetObjectId(CObject* Object) const
{
    if (!Object)
    {
        return 0;
    }
    
    auto It = ObjectToIdMap.Find(Object);
    if (It != ObjectToIdMap.End())
    {
        return It->Value;
    }
    
    // 如果是写入模式，自动分配新ID
    if (Mode == ESerializationMode::Writing)
    {
        uint64_t NewId = NextObjectId++;
        const_cast<CSerializationContext*>(this)->RegisterObject(Object, NewId);
        return NewId;
    }
    
    return 0;
}

bool CSerializationContext::HasObject(CObject* Object) const
{
    return ObjectToIdMap.Contains(Object);
}

void CSerializationContext::RegisterType(const CString& TypeName, const std::type_info& TypeInfo)
{
    TypeRegistry[TypeName] = &TypeInfo;
    TypeNameRegistry[&TypeInfo] = TypeName;
}

const std::type_info* CSerializationContext::FindType(const CString& TypeName) const
{
    auto It = TypeRegistry.Find(TypeName);
    if (It != TypeRegistry.End())
    {
        return It->Value;
    }
    return nullptr;
}

CString CSerializationContext::GetTypeName(const std::type_info& TypeInfo) const
{
    auto It = TypeNameRegistry.Find(&TypeInfo);
    if (It != TypeNameRegistry.End())
    {
        return It->Value;
    }
    return TypeInfo.name(); // 回退到默认名称
}

bool CSerializationContext::HasCustomData(const CString& Key) const
{
    return CustomData.Contains(Key);
}

void CSerializationContext::RemoveCustomData(const CString& Key)
{
    CustomData.Remove(Key);
}

void CSerializationContext::AddError(const CString& Error)
{
    Errors.PushBack(Error);
    CLogger::Get().LogError("Serialization Error: {}", Error);
}

void CSerializationContext::AddWarning(const CString& Warning)
{
    Warnings.PushBack(Warning);
    CLogger::Get().LogWarning("Serialization Warning: {}", Warning);
}

void CSerializationContext::ClearMessages()
{
    Errors.Clear();
    Warnings.Clear();
}

// === NSerializationUtils 实现 ===

bool NSerializationUtils::IsVersionCompatible(uint32_t CurrentVersion, uint32_t RequiredVersion)
{
    return CurrentVersion >= RequiredVersion;
}

void NSerializationUtils::HandleVersionMismatch(CSerializationContext& Context, uint32_t ExpectedVersion, uint32_t ActualVersion)
{
    if (ActualVersion < ExpectedVersion)
    {
        Context.AddWarning(CString::Format(
            "Version mismatch: Expected {}, got {}. Data may be incomplete.",
            ExpectedVersion, ActualVersion
        ));
    }
    else if (ActualVersion > ExpectedVersion)
    {
        Context.AddWarning(CString::Format(
            "Version mismatch: Expected {}, got {}. Forward compatibility may be limited.",
            ExpectedVersion, ActualVersion
        ));
    }
}

bool NSerializationUtils::SerializeObject(CArchive& Archive, const CString& Name, TSharedPtr<CObject>& Object)
{
    if (Archive.IsWriting())
    {
        if (!Object)
        {
            // 序列化空对象
            uint64_t ObjectId = 0;
            return Archive.SerializeValue(Name + "_Id", ObjectId);
        }
        
        uint64_t ObjectId = Archive.GetContext()->GetObjectId(Object.Get());
        if (!Archive.SerializeValue(Name + "_Id", ObjectId))
        {
            return false;
        }
        
        // 序列化类型信息
        CString TypeName = Archive.GetContext()->GetTypeName(typeid(*Object));
        if (!Archive.SerializeValue(Name + "_Type", TypeName))
        {
            return false;
        }
        
        // 序列化对象数据
        if (auto Serializable = CObject::Cast<ISerializable>(Object))
        {
            if (!Archive.BeginObject(Name))
            {
                return false;
            }
            
            Serializable->Serialize(Archive);
            
            if (!Archive.EndObject())
            {
                return false;
            }
        }
    }
    else
    {
        // 反序列化
        uint64_t ObjectId = 0;
        if (!Archive.SerializeValue(Name + "_Id", ObjectId))
        {
            return false;
        }
        
        if (ObjectId == 0)
        {
            Object.Reset();
            return true;
        }
        
        // 检查是否已经反序列化过
        CObject* ExistingObject = Archive.GetContext()->FindObject(ObjectId);
        if (ExistingObject)
        {
            Object = ExistingObject->AsShared();
            return true;
        }
        
        // 读取类型信息
        CString TypeName;
        if (!Archive.SerializeValue(Name + "_Type", TypeName))
        {
            return false;
        }
        
        // 这里需要类型工厂来创建对象实例
        // 简化实现：暂时返回空对象
        Archive.GetContext()->AddWarning(
            CString::Format("Cannot deserialize object of type: {}", TypeName)
        );
        
        Object.Reset();
    }
    
    return true;
}

bool NSerializationUtils::SerializeObject(CArchive& Archive, const CString& Name, CObject* Object)
{
    TSharedPtr<CObject> SharedObject = Object ? Object->AsShared() : nullptr;
    return SerializeObject(Archive, Name, SharedObject);
}

// === 其他工具函数实现 ===

CString NSerializationAttribute::NSerializationAttribute::ToString() const
{
    CString Result;
    
    if (HasFlag(SkipSerialization))
        Result += "SkipSerialization ";
    if (HasFlag(SkipDeserialization))
        Result += "SkipDeserialization ";
    if (HasFlag(Required))
        Result += "Required ";
    if (HasFlag(Optional))
        Result += "Optional ";
    if (HasFlag(Deprecated))
        Result += "Deprecated ";
    if (HasFlag(Transient))
        Result += "Transient ";
    
    if (!Name.IsEmpty())
        Result += CString::Format("Name={} ", Name);
    
    Result += CString::Format("Version={}..{}", Version, MaxVersion);
    
    if (!DefaultValue.IsEmpty())
        Result += CString::Format(" Default={}", DefaultValue);
    
    return Result.TrimEnd();
}

} // namespace NLib