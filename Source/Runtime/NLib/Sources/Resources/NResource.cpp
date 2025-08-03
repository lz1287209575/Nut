#include "Resources/NResource.h"
#include "Core/CLogger.h"
#include "Core/CAllocator.h"
#include "FileSystem/NFileSystem.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NResource 基类实现 ===

CAtomic<uint64_t> NResource::NextResourceId(1);

NResource::NResource()
    : ResourceId(NextResourceId++)
    , LoadState(EResourceLoadState::Unloaded)
    , Priority(EResourcePriority::Normal)
    , MemoryUsage(0)
    , DiskSize(0)
    , ReferenceCount(0)
{
    LoadTime = NDateTime::Now();
    LastAccessTime = LoadTime;
}

NResource::NResource(const CString& InResourcePath)
    : NResource()
{
    ResourcePath = InResourcePath;
    ResourceName = NPath::GetFileNameWithoutExtension(InResourcePath);
}

NResource::~NResource()
{
    if (IsLoaded())
    {
        Unload();
    }
}

bool NResource::Load()
{
    if (IsLoaded())
    {
        UpdateLastAccessTime();
        return true;
    }
    
    if (IsLoading())
    {
        CLogger::Get().LogWarning("Resource {} is already loading", ResourcePath);
        return false;
    }
    
    SetLoadState(EResourceLoadState::Loading);
    ClearErrors();
    
    // 检查依赖项
    if (!AreDependenciesLoaded())
    {
        SetLastError("Dependencies are not loaded");
        SetLoadState(EResourceLoadState::Failed);
        return false;
    }
    
    bool Success = false;
    try
    {
        Success = LoadInternal();
        
        if (Success)
        {
            SetLoadState(EResourceLoadState::Loaded);
            LoadTime = NDateTime::Now();
            UpdateLastAccessTime();
            OnLoadCompleted();
            OnLoaded.Broadcast(AsShared());
        }
        else
        {
            SetLoadState(EResourceLoadState::Failed);
            OnLoadError("LoadInternal returned false");
            OnLoadFailed.Broadcast(AsShared(), GetLastError());
        }
    }
    catch (...)
    {
        SetLastError("Exception during resource loading");
        SetLoadState(EResourceLoadState::Failed);
        OnLoadError(GetLastError());
        OnLoadFailed.Broadcast(AsShared(), GetLastError());
        Success = false;
    }
    
    return Success;
}

void NResource::Unload()
{
    if (!IsLoaded())
    {
        return;
    }
    
    SetLoadState(EResourceLoadState::Unloading);
    
    try
    {
        UnloadInternal();
        SetLoadState(EResourceLoadState::Unloaded);
        SetMemoryUsage(0);
        OnUnloadCompleted();
        OnUnloaded.Broadcast(AsShared());
    }
    catch (...)
    {
        SetLastError("Exception during resource unloading");
        CLogger::Get().LogError("Failed to unload resource {}: {}", ResourcePath, GetLastError());
    }
}

bool NResource::Reload()
{
    if (IsLoaded())
    {
        Unload();
    }
    
    bool Success = Load();
    if (Success)
    {
        OnReloaded.Broadcast(AsShared());
    }
    
    return Success;
}

TSharedPtr<NAsyncTask<bool>> NResource::LoadAsync()
{
    return NAsyncTask<bool>::Run([this](NCancellationToken& Token) -> bool {
        return Load();
    });
}

TSharedPtr<NAsyncTask<void>> NResource::UnloadAsync()
{
    return NAsyncTask<void>::Run([this](NCancellationToken& Token) -> void {
        Unload();
    });
}

void NResource::AddDependency(TSharedPtr<NResource> Dependency)
{
    if (Dependency && !Dependencies.Contains(Dependency))
    {
        Dependencies.PushBack(Dependency);
    }
}

void NResource::RemoveDependency(TSharedPtr<NResource> Dependency)
{
    Dependencies.Remove(Dependency);
}

bool NResource::AreDependenciesLoaded() const
{
    for (const auto& Dependency : Dependencies)
    {
        if (!Dependency || !Dependency->IsLoaded())
        {
            return false;
        }
    }
    return true;
}

void NResource::SetMetadata(const CString& Key, const CString& Value)
{
    Metadata[Key] = Value;
}

CString NResource::GetMetadata(const CString& Key, const CString& DefaultValue) const
{
    auto It = Metadata.Find(Key);
    return (It != Metadata.End()) ? It->Value : DefaultValue;
}

bool NResource::HasMetadata(const CString& Key) const
{
    return Metadata.Contains(Key);
}

void NResource::RemoveMetadata(const CString& Key)
{
    Metadata.Remove(Key);
}

void NResource::AddTag(const CString& Tag)
{
    if (!HasTag(Tag))
    {
        Tags.PushBack(Tag);
    }
}

void NResource::RemoveTag(const CString& Tag)
{
    Tags.Remove(Tag);
}

bool NResource::HasTag(const CString& Tag) const
{
    return Tags.Contains(Tag);
}

void NResource::ClearTags()
{
    Tags.Clear();
}

CString NResource::ToString() const
{
    return CString::Format(
        "Resource(Id={}, Path={}, Type={}, State={}, Size={}KB)",
        ResourceId,
        ResourcePath,
        GetResourceTypeName(),
        static_cast<int32_t>(LoadState),
        MemoryUsage / 1024
    );
}

void NResource::SetLoadState(EResourceLoadState NewState)
{
    if (LoadState != NewState)
    {
        LoadState = NewState;
        
        switch (NewState)
        {
        case EResourceLoadState::Loading:
            CLogger::Get().LogInfo("Loading resource: {}", ResourcePath);
            break;
        case EResourceLoadState::Loaded:
            CLogger::Get().LogInfo("Loaded resource: {} ({}KB)", ResourcePath, MemoryUsage / 1024);
            break;
        case EResourceLoadState::Failed:
            CLogger::Get().LogError("Failed to load resource: {} - {}", ResourcePath, LastError);
            break;
        case EResourceLoadState::Unloading:
            CLogger::Get().LogInfo("Unloading resource: {}", ResourcePath);
            break;
        case EResourceLoadState::Unloaded:
            CLogger::Get().LogInfo("Unloaded resource: {}", ResourcePath);
            break;
        }
    }
}

// === NDataResource 实现 ===

NDataResource::NDataResource()
{
}

NDataResource::NDataResource(const CString& ResourcePath)
    : NResource(ResourcePath)
{
}

NDataResource::~NDataResource()
{
}

void NDataResource::SetData(const CArray<uint8_t>& InData)
{
    Data = InData;
    SetMemoryUsage(Data.GetSize());
}

void NDataResource::SetData(CArray<uint8_t>&& InData)
{
    Data = NLib::Move(InData);
    SetMemoryUsage(Data.GetSize());
}

void NDataResource::ClearData()
{
    Data.Clear();
    SetMemoryUsage(0);
}

bool NDataResource::LoadInternal()
{
    if (GetResourcePath().IsEmpty())
    {
        SetLastError("Resource path is empty");
        return false;
    }
    
    if (!NFile::Exists(GetResourcePath()))
    {
        SetLastError("File does not exist: " + GetResourcePath());
        return false;
    }
    
    try
    {
        Data = NFile::ReadAllBytes(GetResourcePath());
        SetMemoryUsage(Data.GetSize());
        SetDiskSize(Data.GetSize());
        return true;
    }
    catch (...)
    {
        SetLastError("Failed to read file: " + GetResourcePath());
        return false;
    }
}

void NDataResource::UnloadInternal()
{
    ClearData();
}

// === NTextResource 实现 ===

NTextResource::NTextResource()
{
}

NTextResource::NTextResource(const CString& ResourcePath)
    : NResource(ResourcePath)
{
}

NTextResource::~NTextResource()
{
}

void NTextResource::SetText(const CString& InText)
{
    Text = InText;
    SetMemoryUsage(Text.GetLength() * sizeof(char));
}

void NTextResource::ClearText()
{
    Text.Clear();
    SetMemoryUsage(0);
}

CArray<CString> NTextResource::GetLines() const
{
    return Text.Split('\n');
}

void NTextResource::SetLines(const CArray<CString>& Lines)
{
    Text = CString::Join(Lines, "\n");
    SetMemoryUsage(Text.GetLength() * sizeof(char));
}

bool NTextResource::LoadInternal()
{
    if (GetResourcePath().IsEmpty())
    {
        SetLastError("Resource path is empty");
        return false;
    }
    
    if (!NFile::Exists(GetResourcePath()))
    {
        SetLastError("File does not exist: " + GetResourcePath());
        return false;
    }
    
    try
    {
        Text = NFile::ReadAllText(GetResourcePath());
        SetMemoryUsage(Text.GetLength() * sizeof(char));
        
        NFileInfo FileInfo = NFile::GetFileInfo(GetResourcePath());
        SetDiskSize(FileInfo.Size);
        
        return true;
    }
    catch (...)
    {
        SetLastError("Failed to read text file: " + GetResourcePath());
        return false;
    }
}

void NTextResource::UnloadInternal()
{
    ClearText();
}

// === NConfigResource 实现 ===

NConfigResource::NConfigResource()
{
}

NConfigResource::NConfigResource(const CString& ResourcePath)
    : NResource(ResourcePath)
{
}

NConfigResource::~NConfigResource()
{
}

const CConfigValue& NConfigResource::GetConfig() const
{
    static CConfigValue EmptyConfig;
    return Config ? *Config : EmptyConfig;
}

void NConfigResource::SetConfig(const CConfigValue& InConfig)
{
    Config = NewNObject<CConfigValue>(InConfig);
}

bool NConfigResource::HasValue(const CString& Path) const
{
    if (!Config)
    {
        return false;
    }
    
    return Config->HasPath(Path);
}

bool NConfigResource::LoadInternal()
{
    if (GetResourcePath().IsEmpty())
    {
        SetLastError("Resource path is empty");
        return false;
    }
    
    if (!NFile::Exists(GetResourcePath()))
    {
        SetLastError("File does not exist: " + GetResourcePath());
        return false;
    }
    
    try
    {
        Config = NewNObject<CConfigValue>();
        bool Success = Config->LoadFromFile(GetResourcePath());
        
        if (Success)
        {
            NFileInfo FileInfo = NFile::GetFileInfo(GetResourcePath());
            SetDiskSize(FileInfo.Size);
            SetMemoryUsage(FileInfo.Size); // 估算内存使用
        }
        else
        {
            SetLastError("Failed to parse config file: " + GetResourcePath());
            Config.Reset();
        }
        
        return Success;
    }
    catch (...)
    {
        SetLastError("Exception while loading config file: " + GetResourcePath());
        Config.Reset();
        return false;
    }
}

void NConfigResource::UnloadInternal()
{
    Config.Reset();
}

} // namespace NLib