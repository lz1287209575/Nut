#include "NObject.h"
#include "Memory/NGarbageCollector.h"
#include "Logging/NLogger.h"
#include "Memory/NMemoryManager.h"

namespace Nut
{

// 静态成员初始化
std::atomic<uint64_t> NObject::NextObjectId{1};

NObject::NObject()
    : RefCount(1), // 创建时引用计数为1
    bMarked(false), bIsValid(true), ObjectId(NextObjectId.fetch_add(1))
{
    // 注册到GC系统
    RegisterWithGC();

    NLogger::Debug("NObject created with ID: " + std::to_string(ObjectId));
}

NObject::~NObject()
{
    bIsValid = false;

    // 从GC系统注销
    UnregisterFromGC();

    NLogger::Debug("NObject destroyed with ID: " + std::to_string(ObjectId));
}

// === tcmalloc 内存分配器重载实现 ===

void *NObject::operator new(size_t Size)
{
    auto &MemMgr = NMemoryManager::GetInstance();
    void *Ptr = MemMgr.Allocate(Size);

    if (!Ptr)
    {
        throw std::bad_alloc();
    }

    NLogger::Debug("NObject::new allocated " + std::to_string(Size) + " bytes at " +
                   std::to_string(reinterpret_cast<uintptr_t>(Ptr)));
    return Ptr;
}

void NObject::operator delete(void *Ptr) noexcept
{
    if (!Ptr)
    {
        return;
    }

    NLogger::Debug("NObject::delete releasing memory at " + std::to_string(reinterpret_cast<uintptr_t>(Ptr)));

    auto &MemMgr = NMemoryManager::GetInstance();
    MemMgr.Deallocate(Ptr);
}

void *NObject::operator new[](size_t Size)
{
    auto &MemMgr = NMemoryManager::GetInstance();
    void *Ptr = MemMgr.Allocate(Size);

    if (!Ptr)
    {
        throw std::bad_alloc();
    }

    NLogger::Debug("NObject::new[] allocated " + std::to_string(Size) + " bytes at " +
                   std::to_string(reinterpret_cast<uintptr_t>(Ptr)));
    return Ptr;
}

void NObject::operator delete[](void *Ptr) noexcept
{
    if (!Ptr)
    {
        return;
    }

    NLogger::Debug("NObject::delete[] releasing memory at " + std::to_string(reinterpret_cast<uintptr_t>(Ptr)));

    auto &MemMgr = NMemoryManager::GetInstance();
    MemMgr.Deallocate(Ptr);
}

void *NObject::operator new(size_t Size, std::align_val_t Alignment)
{
    auto &MemMgr = NMemoryManager::GetInstance();
    void *Ptr = MemMgr.Allocate(Size, static_cast<size_t>(Alignment));

    if (!Ptr)
    {
        throw std::bad_alloc();
    }

    NLogger::Debug("NObject::new(aligned) allocated " + std::to_string(Size) + " bytes with alignment " +
                   std::to_string(static_cast<size_t>(Alignment)) + " at " +
                   std::to_string(reinterpret_cast<uintptr_t>(Ptr)));
    return Ptr;
}

void NObject::operator delete(void *Ptr, std::align_val_t Alignment) noexcept
{
    if (!Ptr)
    {
        return;
    }

    NLogger::Debug("NObject::delete(aligned) releasing memory at " + std::to_string(reinterpret_cast<uintptr_t>(Ptr)) +
                   " with alignment " + std::to_string(static_cast<size_t>(Alignment)));

    auto &MemMgr = NMemoryManager::GetInstance();
    MemMgr.Deallocate(Ptr);
}

NObject::NObject(NObject &&Other) noexcept
    : RefCount(Other.RefCount.load()), bMarked(Other.bMarked.load()), bIsValid(Other.bIsValid.load()),
      ObjectId(Other.ObjectId)
{
    // 移动构造后，原对象失效
    Other.bIsValid = false;
    Other.RefCount = 0;

    // 重新注册到GC（因为对象地址改变了）
    RegisterWithGC();

    NLogger::Debug("NObject move constructed with ID: " + std::to_string(ObjectId));
}

NObject &NObject::operator=(NObject &&Other) noexcept
{
    if (this != &Other)
    {
        // 先注销当前对象
        UnregisterFromGC();

        // 移动数据
        RefCount = Other.RefCount.load();
        bMarked = Other.bMarked.load();
        bIsValid = Other.bIsValid.load();
        ObjectId = Other.ObjectId;

        // 原对象失效
        Other.bIsValid = false;
        Other.RefCount = 0;

        // 重新注册
        RegisterWithGC();

        NLogger::Debug("NObject move assigned with ID: " + std::to_string(ObjectId));
    }
    return *this;
}

int32_t NObject::AddRef()
{
    if (!bIsValid)
    {
        NLogger::Error("Attempted to AddRef on invalid object ID: " + std::to_string(ObjectId));
        return 0;
    }

    int32_t NewRefCount = RefCount.fetch_add(1) + 1;
    NLogger::Debug("AddRef object ID " + std::to_string(ObjectId) + ", RefCount: " + std::to_string(NewRefCount));
    return NewRefCount;
}

int32_t NObject::Release()
{
    if (!bIsValid)
    {
        NLogger::Error("Attempted to Release on invalid object ID: " + std::to_string(ObjectId));
        return 0;
    }

    int32_t NewRefCount = RefCount.fetch_sub(1) - 1;
    NLogger::Debug("Release object ID " + std::to_string(ObjectId) + ", RefCount: " + std::to_string(NewRefCount));

    if (NewRefCount <= 0)
    {
        // 引用计数归零，删除对象
        NLogger::Debug("Object ID " + std::to_string(ObjectId) + " RefCount reached 0, deleting");
        delete this;
        return 0;
    }

    return NewRefCount;
}

int32_t NObject::GetRefCount() const
{
    return RefCount.load();
}

void NObject::Mark()
{
    if (!bIsValid)
    {
        return;
    }

    bool Expected = false;
    if (bMarked.compare_exchange_strong(Expected, true))
    {
        // 首次标记，记录日志
        NLogger::Debug("Marked object ID: " + std::to_string(ObjectId));
    }
}

bool NObject::IsMarked() const
{
    return bMarked.load();
}

void NObject::UnMark()
{
    if (!bIsValid)
    {
        return;
    }

    bMarked = false;
    NLogger::Debug("Unmarked object ID: " + std::to_string(ObjectId));
}

const std::type_info &NObject::GetTypeInfo() const
{
    return typeid(NObject);
}

const char *NObject::GetTypeName() const
{
    return "NObject";
}

const NClassReflection* NObject::GetClassReflection() const
{
    return nullptr; // 基类返回空指针
}

void NObject::RegisterWithGC()
{
    auto &GC = NGarbageCollector::GetInstance();
    if (GC.IsInitialized())
    {
        GC.RegisterObject(this);
    }
    NLogger::Debug("Registered object ID " + std::to_string(ObjectId) + " with GC");
}

void NObject::UnregisterFromGC()
{
    auto &GC = NGarbageCollector::GetInstance();
    if (GC.IsInitialized())
    {
        GC.UnregisterObject(this);
    }
    NLogger::Debug("Unregistered object ID " + std::to_string(ObjectId) + " from GC");
}

} // namespace Nut
