#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Async/NAsyncTask.h"
#include "Delegates/CDelegate.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

/**
 * @brief 资源加载状态
 */
enum class EResourceLoadState : uint32_t
{
    Unloaded,       // 未加载
    Loading,        // 加载中
    Loaded,         // 已加载
    Failed,         // 加载失败
    Unloading       // 卸载中
};

/**
 * @brief 资源类型
 */
enum class EResourceType : uint32_t
{
    Unknown,        // 未知类型
    BinData,        // 数据BinData
};

/**
 * @brief 资源优先级
 */
enum class EResourcePriority : uint32_t
{
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief 资源基类 - 所有资源的基础类
 */
NCLASS()
class LIBNLIB_API NResource : public CObject
{
    GENERATED_BODY()

public:
    NResource();
    explicit NResource(const CString& InResourcePath);
    virtual ~NResource();
    
    // 资源标识
    const CString& GetResourcePath() const { return ResourcePath; }
    void SetResourcePath(const CString& InPath) { ResourcePath = InPath; }
    
    const CString& GetResourceName() const { return ResourceName; }
    void SetResourceName(const CString& InName) { ResourceName = InName; }
    
    uint64_t GetResourceId() const { return ResourceId; }
    
    // 资源类型
    virtual EResourceType GetResourceType() const { return EResourceType::Unknown; }
    virtual CString GetResourceTypeName() const { return "Unknown"; }
    
    // 资源状态
    EResourceLoadState GetLoadState() const { return LoadState; }
    bool IsLoaded() const { return LoadState == EResourceLoadState::Loaded; }
    bool IsLoading() const { return LoadState == EResourceLoadState::Loading; }
    bool IsFailed() const { return LoadState == EResourceLoadState::Failed; }
    bool IsUnloaded() const { return LoadState == EResourceLoadState::Unloaded; }
    
    // 资源优先级
    EResourcePriority GetPriority() const { return Priority; }
    void SetPriority(EResourcePriority InPriority) { Priority = InPriority; }
    
    // 资源大小
    virtual size_t GetMemoryUsage() const { return MemoryUsage; }
    virtual size_t GetDiskSize() const { return DiskSize; }
    
    // 时间戳
    const NDateTime& GetLoadTime() const { return LoadTime; }
    const NDateTime& GetLastAccessTime() const { return LastAccessTime; }
    void UpdateLastAccessTime() { LastAccessTime = NDateTime::Now(); }
    
    // 引用计数
    int32_t GetReferenceCount() const { return ReferenceCount; }
    void AddReference() { ++ReferenceCount; }
    void RemoveReference() { --ReferenceCount; }
    
    // 资源加载/卸载
    virtual bool Load();
    virtual void Unload();
    virtual bool Reload();
    
    // 异步加载
    TSharedPtr<NAsyncTask<bool>> LoadAsync();
    TSharedPtr<NAsyncTask<void>> UnloadAsync();
    
    // 资源验证
    virtual bool IsValid() const { return IsLoaded() && !HasLoadErrors(); }
    virtual bool Validate() const { return IsValid(); }
    
    // 依赖管理
    void AddDependency(TSharedPtr<NResource> Dependency);
    void RemoveDependency(TSharedPtr<NResource> Dependency);
    const CArray<TSharedPtr<NResource>>& GetDependencies() const { return Dependencies; }
    bool HasDependencies() const { return !Dependencies.IsEmpty(); }
    bool AreDependenciesLoaded() const;
    
    // 错误处理
    const CString& GetLastError() const { return LastError; }
    bool HasLoadErrors() const { return !LastError.IsEmpty(); }
    void ClearErrors() { LastError.Clear(); }
    
    // 元数据
    void SetMetadata(const CString& Key, const CString& Value);
    CString GetMetadata(const CString& Key, const CString& DefaultValue = "") const;
    bool HasMetadata(const CString& Key) const;
    void RemoveMetadata(const CString& Key);
    const CHashMap<CString, CString>& GetAllMetadata() const { return Metadata; }
    
    // 标签系统
    void AddTag(const CString& Tag);
    void RemoveTag(const CString& Tag);
    bool HasTag(const CString& Tag) const;
    const CArray<CString>& GetTags() const { return Tags; }
    void ClearTags();
    
    // 事件回调
    CMulticastDelegate<void(TSharedPtr<NResource>)> OnLoaded;
    CMulticastDelegate<void(TSharedPtr<NResource>)> OnUnloaded;
    CMulticastDelegate<void(TSharedPtr<NResource>, const CString&)> OnLoadFailed;
    CMulticastDelegate<void(TSharedPtr<NResource>)> OnReloaded;
    
    virtual CString ToString() const override;

protected:
    // 子类实现的加载逻辑
    virtual bool LoadInternal() = 0;
    virtual void UnloadInternal() = 0;
    virtual bool ReloadInternal() { UnloadInternal(); return LoadInternal(); }
    
    // 状态管理
    void SetLoadState(EResourceLoadState NewState);
    void SetMemoryUsage(size_t Usage) { MemoryUsage = Usage; }
    void SetDiskSize(size_t Size) { DiskSize = Size; }
    void SetLastError(const CString& Error) { LastError = Error; }
    
    // 加载完成回调
    virtual void OnLoadCompleted() {}
    virtual void OnUnloadCompleted() {}
    virtual void OnLoadError(const CString& Error) {}

private:
    CString ResourcePath;
    CString ResourceName;
    uint64_t ResourceId;
    EResourceLoadState LoadState;
    EResourcePriority Priority;
    
    size_t MemoryUsage;
    size_t DiskSize;
    
    NDateTime LoadTime;
    NDateTime LastAccessTime;
    
    int32_t ReferenceCount;
    
    CArray<TSharedPtr<NResource>> Dependencies;
    CString LastError;
    
    CHashMap<CString, CString> Metadata;
    CArray<CString> Tags;
    
    static CAtomic<uint64_t> NextResourceId;
    
    // 异步加载任务
    void LoadAsyncInternal();
    void UnloadAsyncInternal();
};

/**
 * @brief 资源工厂接口
 */
class LIBNLIB_API IResourceFactory
{
public:
    virtual ~IResourceFactory() = default;
    
    // 创建资源
    virtual TSharedPtr<NResource> CreateResource(const CString& ResourcePath) = 0;
    
    // 支持的文件扩展名
    virtual bool CanCreate(const CString& ResourcePath) const = 0;
    virtual CArray<CString> GetSupportedExtensions() const = 0;
    
    // 资源类型信息
    virtual EResourceType GetResourceType() const = 0;
    virtual CString GetFactoryName() const = 0;
    
    // 优先级（用于同一扩展名的多个工厂）
    virtual int32_t GetPriority() const { return 0; }
};

/**
 * @brief 泛型资源工厂模板
 */
template<typename TResource>
class CResourceFactory : public IResourceFactory
{
    static_assert(std::is_base_of_v<NResource, TResource>, "TResource must derive from NResource");

public:
    CResourceFactory(const CArray<CString>& InSupportedExtensions)
        : SupportedExtensions(InSupportedExtensions)
    {}
    
    virtual TSharedPtr<NResource> CreateResource(const CString& ResourcePath) override
    {
        return NewNObject<TResource>(ResourcePath);
    }
    
    virtual bool CanCreate(const CString& ResourcePath) const override
    {
        CString Extension = NPath::GetExtension(ResourcePath).ToLower();
        return SupportedExtensions.Contains(Extension);
    }
    
    virtual CArray<CString> GetSupportedExtensions() const override
    {
        return SupportedExtensions;
    }
    
    virtual EResourceType GetResourceType() const override
    {
        // 尝试从TResource获取类型信息
        TResource TempResource;
        return TempResource.GetResourceType();
    }
    
    virtual CString GetFactoryName() const override
    {
        return CString::Format("CResourceFactory<{}>", typeid(TResource).name());
    }

private:
    CArray<CString> SupportedExtensions;
};

/**
 * @brief 简单数据资源 - 存储二进制数据
 */
class LIBNLIB_API NDataResource : public NResource
{
    NCLASS()
class NDataResource : public NResource
{
    GENERATED_BODY()

public:
    NDataResource();
    explicit NDataResource(const CString& ResourcePath);
    virtual ~NDataResource();
    
    virtual EResourceType GetResourceType() const override { return EResourceType::Data; }
    virtual CString GetResourceTypeName() const override { return "Data"; }
    
    // 数据访问
    const CArray<uint8_t>& GetData() const { return Data; }
    void SetData(const CArray<uint8_t>& InData);
    void SetData(CArray<uint8_t>&& InData);
    
    const uint8_t* GetDataPtr() const { return Data.GetData(); }
    size_t GetDataSize() const { return Data.GetSize(); }
    
    bool IsEmpty() const { return Data.IsEmpty(); }
    void ClearData();

protected:
    virtual bool LoadInternal() override;
    virtual void UnloadInternal() override;

private:
    CArray<uint8_t> Data;
};

/**
 * @brief 文本资源 - 存储文本数据
 */
class LIBNLIB_API NTextResource : public NResource
{
    NCLASS()
class NTextResource : public NResource
{
    GENERATED_BODY()

public:
    NTextResource();
    explicit NTextResource(const CString& ResourcePath);
    virtual ~NTextResource();
    
    virtual EResourceType GetResourceType() const override { return EResourceType::Data; }
    virtual CString GetResourceTypeName() const override { return "Text"; }
    
    // 文本访问
    const CString& GetText() const { return Text; }
    void SetText(const CString& InText);
    
    bool IsEmpty() const { return Text.IsEmpty(); }
    void ClearText();
    
    // 文本操作
    CArray<CString> GetLines() const;
    void SetLines(const CArray<CString>& Lines);

protected:
    virtual bool LoadInternal() override;
    virtual void UnloadInternal() override;

private:
    CString Text;
};

/**
 * @brief 配置资源 - 基于NConfig的配置文件资源
 */
class LIBNLIB_API NConfigResource : public NResource
{
    NCLASS()
class NConfigResource : public NResource
{
    GENERATED_BODY()

public:
    NConfigResource();
    explicit NConfigResource(const CString& ResourcePath);
    virtual ~NConfigResource();
    
    virtual EResourceType GetResourceType() const override { return EResourceType::Config; }
    virtual CString GetResourceTypeName() const override { return "Config"; }
    
    // 配置访问
    const class CConfigValue& GetConfig() const;
    void SetConfig(const class CConfigValue& InConfig);
    
    // 配置查询
    template<typename TType>
    TType GetValue(const CString& Path, const TType& DefaultValue = TType{}) const;
    
    bool HasValue(const CString& Path) const;

protected:
    virtual bool LoadInternal() override;
    virtual void UnloadInternal() override;

private:
    TSharedPtr<class CConfigValue> Config;
};

/**
 * @brief 资源句柄 - 资源的轻量级引用
 */
template<typename TResource = NResource>
class CResourceHandle
{
    static_assert(std::is_base_of_v<NResource, TResource>, "TResource must derive from NResource");

public:
    CResourceHandle();
    CResourceHandle(TSharedPtr<TResource> InResource);
    CResourceHandle(const CString& ResourcePath);
    CResourceHandle(const CResourceHandle& Other);
    CResourceHandle(CResourceHandle&& Other) noexcept;
    
    CResourceHandle& operator=(const CResourceHandle& Other);
    CResourceHandle& operator=(CResourceHandle&& Other) noexcept;
    CResourceHandle& operator=(TSharedPtr<TResource> InResource);
    
    ~CResourceHandle();
    
    // 资源访问
    TResource* Get() const;
    TResource* operator->() const;
    TResource& operator*() const;
    
    operator bool() const;
    bool IsValid() const;
    bool IsLoaded() const;
    bool IsLoading() const;
    
    // 资源操作
    bool Load();
    void Unload();
    bool Reload();
    
    TSharedPtr<NAsyncTask<bool>> LoadAsync();
    TSharedPtr<NAsyncTask<void>> UnloadAsync();
    
    // 比较操作
    bool operator==(const CResourceHandle& Other) const;
    bool operator!=(const CResourceHandle& Other) const;
    bool operator<(const CResourceHandle& Other) const;
    
    // 获取原始资源指针
    TSharedPtr<TResource> GetSharedPtr() const;
    
    // 重置句柄
    void Reset();
    void Reset(TSharedPtr<TResource> InResource);
    void Reset(const CString& ResourcePath);

private:
    TSharedPtr<TResource> Resource;
    CString ResourcePath; // 延迟加载时使用
    
    void EnsureLoaded();
};

// === 模板实现 ===

template<typename TType>
TType NConfigResource::GetValue(const CString& Path, const TType& DefaultValue) const
{
    if (!Config)
    {
        return DefaultValue;
    }
    
    if constexpr (std::is_same_v<TType, bool>)
    {
        return Config->GetPath(Path).AsBool(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, int32_t>)
    {
        return Config->GetPath(Path).AsInt(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, int64_t>)
    {
        return Config->GetPath(Path).AsInt64(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, float>)
    {
        return Config->GetPath(Path).AsFloat(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, double>)
    {
        return Config->GetPath(Path).AsDouble(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, CString>)
    {
        return Config->GetPath(Path).AsString(DefaultValue);
    }
    else
    {
        return DefaultValue;
    }
}

// CResourceHandle 模板实现
template<typename TResource>
CResourceHandle<TResource>::CResourceHandle()
{}

template<typename TResource>
CResourceHandle<TResource>::CResourceHandle(TSharedPtr<TResource> InResource)
    : Resource(InResource)
{
    if (Resource)
    {
        Resource->AddReference();
    }
}

template<typename TResource>
CResourceHandle<TResource>::CResourceHandle(const CString& InResourcePath)
    : ResourcePath(InResourcePath)
{}

template<typename TResource>
CResourceHandle<TResource>::CResourceHandle(const CResourceHandle& Other)
    : Resource(Other.Resource), ResourcePath(Other.ResourcePath)
{
    if (Resource)
    {
        Resource->AddReference();
    }
}

template<typename TResource>
CResourceHandle<TResource>::CResourceHandle(CResourceHandle&& Other) noexcept
    : Resource(NLib::Move(Other.Resource)), ResourcePath(NLib::Move(Other.ResourcePath))
{
    Other.Resource.Reset();
    Other.ResourcePath.Clear();
}

template<typename TResource>
CResourceHandle<TResource>& CResourceHandle<TResource>::operator=(const CResourceHandle& Other)
{
    if (this != &Other)
    {
        if (Resource)
        {
            Resource->RemoveReference();
        }
        
        Resource = Other.Resource;
        ResourcePath = Other.ResourcePath;
        
        if (Resource)
        {
            Resource->AddReference();
        }
    }
    return *this;
}

template<typename TResource>
CResourceHandle<TResource>& CResourceHandle<TResource>::operator=(CResourceHandle&& Other) noexcept
{
    if (this != &Other)
    {
        if (Resource)
        {
            Resource->RemoveReference();
        }
        
        Resource = NLib::Move(Other.Resource);
        ResourcePath = NLib::Move(Other.ResourcePath);
        
        Other.Resource.Reset();
        Other.ResourcePath.Clear();
    }
    return *this;
}

template<typename TResource>
CResourceHandle<TResource>& CResourceHandle<TResource>::operator=(TSharedPtr<TResource> InResource)
{
    if (Resource)
    {
        Resource->RemoveReference();
    }
    
    Resource = InResource;
    ResourcePath.Clear();
    
    if (Resource)
    {
        Resource->AddReference();
    }
    
    return *this;
}

template<typename TResource>
CResourceHandle<TResource>::~CResourceHandle()
{
    if (Resource)
    {
        Resource->RemoveReference();
    }
}

template<typename TResource>
TResource* CResourceHandle<TResource>::Get() const
{
    const_cast<CResourceHandle*>(this)->EnsureLoaded();
    return Resource.Get();
}

template<typename TResource>
TResource* CResourceHandle<TResource>::operator->() const
{
    return Get();
}

template<typename TResource>
TResource& CResourceHandle<TResource>::operator*() const
{
    return *Get();
}

template<typename TResource>
CResourceHandle<TResource>::operator bool() const
{
    return IsValid();
}

template<typename TResource>
bool CResourceHandle<TResource>::IsValid() const
{
    const_cast<CResourceHandle*>(this)->EnsureLoaded();
    return Resource && Resource->IsValid();
}

template<typename TResource>
bool CResourceHandle<TResource>::IsLoaded() const
{
    const_cast<CResourceHandle*>(this)->EnsureLoaded();
    return Resource && Resource->IsLoaded();
}

template<typename TResource>
bool CResourceHandle<TResource>::IsLoading() const
{
    const_cast<CResourceHandle*>(this)->EnsureLoaded();
    return Resource && Resource->IsLoading();
}

template<typename TResource>
bool CResourceHandle<TResource>::Load()
{
    EnsureLoaded();
    return Resource ? Resource->Load() : false;
}

template<typename TResource>
void CResourceHandle<TResource>::Unload()
{
    if (Resource)
    {
        Resource->Unload();
    }
}

template<typename TResource>
bool CResourceHandle<TResource>::Reload()
{
    EnsureLoaded();
    return Resource ? Resource->Reload() : false;
}

template<typename TResource>
TSharedPtr<NAsyncTask<bool>> CResourceHandle<TResource>::LoadAsync()
{
    EnsureLoaded();
    return Resource ? Resource->LoadAsync() : NAsyncTask<bool>::FromResult(false);
}

template<typename TResource>
TSharedPtr<NAsyncTask<void>> CResourceHandle<TResource>::UnloadAsync()
{
    EnsureLoaded();
    return Resource ? Resource->UnloadAsync() : NAsyncTask<void>::CompletedTask();
}

template<typename TResource>
bool CResourceHandle<TResource>::operator==(const CResourceHandle& Other) const
{
    return Resource == Other.Resource && ResourcePath == Other.ResourcePath;
}

template<typename TResource>
bool CResourceHandle<TResource>::operator!=(const CResourceHandle& Other) const
{
    return !(*this == Other);
}

template<typename TResource>
bool CResourceHandle<TResource>::operator<(const CResourceHandle& Other) const
{
    if (Resource && Other.Resource)
    {
        return Resource->GetResourceId() < Other.Resource->GetResourceId();
    }
    return ResourcePath < Other.ResourcePath;
}

template<typename TResource>
TSharedPtr<TResource> CResourceHandle<TResource>::GetSharedPtr() const
{
    const_cast<CResourceHandle*>(this)->EnsureLoaded();
    return Resource;
}

template<typename TResource>
void CResourceHandle<TResource>::Reset()
{
    if (Resource)
    {
        Resource->RemoveReference();
        Resource.Reset();
    }
    ResourcePath.Clear();
}

template<typename TResource>
void CResourceHandle<TResource>::Reset(TSharedPtr<TResource> InResource)
{
    *this = InResource;
}

template<typename TResource>
void CResourceHandle<TResource>::Reset(const CString& InResourcePath)
{
    Reset();
    ResourcePath = InResourcePath;
}

template<typename TResource>
void CResourceHandle<TResource>::EnsureLoaded()
{
    if (!Resource && !ResourcePath.IsEmpty())
    {
        // 通过资源管理器加载资源（需要实现）
        // Resource = NResourceManager::GetInstance().LoadResource<TResource>(ResourcePath);
        if (Resource)
        {
            Resource->AddReference();
        }
    }
}

} // namespace NLib
