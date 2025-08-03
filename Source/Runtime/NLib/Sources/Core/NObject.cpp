#include "CObject.h"
#include "CObject.generate.h"

#include "Containers/CString.h"
#include "Logging/CLogger.h"
#include "Memory/CGarbageCollector.h"
#include "Memory/CMemoryManager.h"

namespace NLib
{

// 静态成员初始化
std::atomic<uint64_t> CObject::NextObjectId{1};

CObject::CObject()
    : RefCount(1), // 创建时引用计数为1
      bMarked(false),
      bIsValid(true),
      ObjectId(NextObjectId.fetch_add(1))
{
	// 注册到GC系统
	RegisterWithGC();

	CLogger::Debug("CObject created with ID: " + std::to_string(ObjectId));
}

CObject::~CObject()
{
	bIsValid = false;

	// 从GC系统注销
	UnregisterFromGC();

	CLogger::Debug("CObject destroyed with ID: " + std::to_string(ObjectId));
}

// === operator new/delete 实现 ===

void* CObject::operator new(size_t Size, void* Ptr) noexcept
{
	(void)Size; // 忽略size参数，因为placement new不分配内存
	return Ptr;
}

void CObject::operator delete(void* Ptr) noexcept
{
	if (!Ptr)
	{
		return;
	}

	// 使用NMemoryManager释放内存
	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.Deallocate(Ptr);
}

void CObject::operator delete[](void* Ptr) noexcept
{
	if (!Ptr)
	{
		return;
	}

	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.Deallocate(Ptr);
}

void CObject::operator delete(void* Ptr, std::align_val_t Alignment) noexcept
{
	(void)Alignment; // 忽略对齐参数
	if (!Ptr)
	{
		return;
	}

	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.Deallocate(Ptr);
}

void CObject::operator delete(void* Ptr, void* PlacementPtr) noexcept
{
	(void)Ptr;          // 忽略参数
	(void)PlacementPtr; // placement delete不释放内存
}

// === 销毁函数实现 ===

void CObject::Destroy()
{
	if (!bIsValid)
	{
		CLogger::Warn("Attempted to destroy invalid object ID: " + std::to_string(ObjectId));
		return;
	}

	CLogger::Debug("Destroying CObject with ID: " + std::to_string(ObjectId));

	// 调用析构函数
	this->~CObject();

	// 使用NMemoryManager直接释放内存
	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.Deallocate(this);
}

CObject::CObject(CObject&& Other) noexcept
    : RefCount(Other.RefCount.load()),
      bMarked(Other.bMarked.load()),
      bIsValid(Other.bIsValid.load()),
      ObjectId(Other.ObjectId)
{
	// 移动构造后，原对象失效
	Other.bIsValid = false;
	Other.RefCount = 0;

	// 重新注册到GC（因为对象地址改变了）
	RegisterWithGC();

	CLogger::Debug("CObject move constructed with ID: " + std::to_string(ObjectId));
}

CObject& CObject::operator=(CObject&& Other) noexcept
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

		CLogger::Debug("CObject move assigned with ID: " + std::to_string(ObjectId));
	}
	return *this;
}

int32_t CObject::AddRef()
{
	if (!bIsValid)
	{
		CLogger::Error("Attempted to AddRef on invalid object ID: " + std::to_string(ObjectId));
		return 0;
	}

	int32_t NewRefCount = RefCount.fetch_add(1) + 1;
	CLogger::Debug("AddRef object ID " + std::to_string(ObjectId) + ", RefCount: " + std::to_string(NewRefCount));
	return NewRefCount;
}

int32_t CObject::Release()
{
	if (!bIsValid)
	{
		CLogger::Error("Attempted to Release on invalid object ID: " + std::to_string(ObjectId));
		return 0;
	}

	int32_t NewRefCount = RefCount.fetch_sub(1) - 1;
	CLogger::Debug("Release object ID " + std::to_string(ObjectId) + ", RefCount: " + std::to_string(NewRefCount));

	if (NewRefCount <= 0)
	{
		// 引用计数归零，销毁对象
		CLogger::Debug("Object ID " + std::to_string(ObjectId) + " RefCount reached 0, destroying");
		Destroy();
		return 0;
	}

	return NewRefCount;
}

int32_t CObject::GetRefCount() const
{
	return RefCount.load();
}

void CObject::Mark()
{
	if (!bIsValid)
	{
		return;
	}

	bool Expected = false;
	if (bMarked.compare_exchange_strong(Expected, true))
	{
		// 首次标记，记录日志
		CLogger::Debug("Marked object ID: " + std::to_string(ObjectId));
	}
}

bool CObject::IsMarked() const
{
	return bMarked.load();
}

void CObject::UnMark()
{
	if (!bIsValid)
	{
		return;
	}

	bMarked = false;
	CLogger::Debug("Unmarked object ID: " + std::to_string(ObjectId));
}

const std::type_info& CObject::GetTypeInfo() const
{
	return typeid(CObject);
}

const char* CObject::GetTypeName() const
{
	return "CObject";
}

const NClassReflection* CObject::GetClassReflection() const
{
	return nullptr; // 基类返回空指针
}

void CObject::RegisterWithGC()
{
	auto& GC = CGarbageCollector::GetInstance();
	if (GC.IsInitialize())
	{
		GC.RegisterObject(this);
	}
	CLogger::Debug("Registered object ID " + std::to_string(ObjectId) + " with GC");
}

void CObject::UnregisterFromGC()
{
	auto& GC = CGarbageCollector::GetInstance();
	if (GC.IsInitialize())
	{
		GC.UnregisterObject(this);
	}
	CLogger::Debug("Unregistered object ID " + std::to_string(ObjectId) + " from GC");
}

// 新增的虚函数实现
bool CObject::Equals(const CObject* Other) const
{
	return this == Other;
}

size_t CObject::GetHashCode() const
{
	return reinterpret_cast<size_t>(this);
}

CString CObject::ToString() const
{
	return CString("CObject(ID: " + std::to_string(ObjectId) + ")");
}

} // namespace NLib
