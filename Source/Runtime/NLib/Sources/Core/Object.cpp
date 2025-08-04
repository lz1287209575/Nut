#include "Object.h"

#include "Containers/TString.h"
#include "Logging/LogCategory.h"
#include "Logging/Logger.h"
#include "Memory/MemoryManager.h"
#include "ObjectFactory.h"

// GC系统头文件
#include "Memory/GarbageCollector.h"

#include <cstdio>   // 用于snprintf
#include <iostream> // 保留用于紧急情况

namespace NLib
{
// === 静态成员初始化 ===
std::atomic<uint64_t> NObject::NextObjectId{1};

// === 构造函数和析构函数 ===

NObject::NObject()
    : RefCount(1) // 创建时引用计数为1
      ,
      bMarked(false) // 初始未标记
      ,
      bIsValid(true) // 初始有效
      ,
      ObjectId(NextObjectId.fetch_add(1)) // 分配唯一ID
{
	// 注册到GC系统
	RegisterWithGC();

	// 使用分类日志记录
	NLOG_CORE(Debug, "NObject created with ID: {}", ObjectId);
}

NObject::~NObject()
{
	// 标记对象无效
	bIsValid = false;

	// 从GC系统注销
	UnregisterFromGC();

	// 使用分类日志记录
	NLOG_CORE(Debug, "NObject destroyed with ID: {}", ObjectId);
}

// === 移动语义 ===

NObject::NObject(NObject&& Other) noexcept
    : RefCount(Other.RefCount.load()),
      bMarked(Other.bMarked.load()),
      bIsValid(Other.bIsValid.load()),
      ObjectId(Other.ObjectId)
{
	// 使原对象失效
	Other.bIsValid = false;
	Other.RefCount = 0;

	// 重新注册到GC（因为对象地址改变了）
	RegisterWithGC();

	NLOG_CORE(Debug, "NObject move constructed with ID: {}", ObjectId);
}

NObject& NObject::operator=(NObject&& Other) noexcept
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

		// 使原对象失效
		Other.bIsValid = false;
		Other.RefCount = 0;

		// 重新注册
		RegisterWithGC();

		NLOG_CORE(Debug, "NObject move assigned with ID: {}", ObjectId);
	}
	return *this;
}

// === 引用计数管理 ===

int32_t NObject::AddRef()
{
	if (!bIsValid.load())
	{
		NLOG_CORE(Error, "Attempted to AddRef on invalid object ID: {}", ObjectId);
		return 0;
	}

	int32_t NewRefCount = RefCount.fetch_add(1) + 1;
	NLOG_CORE(Trace, "AddRef object ID {}, RefCount: {}", ObjectId, NewRefCount);
	return NewRefCount;
}

int32_t NObject::Release()
{
	if (!bIsValid.load())
	{
		NLOG_CORE(Error, "Attempted to Release on invalid object ID: {}", ObjectId);
		return 0;
	}

	int32_t NewRefCount = RefCount.fetch_sub(1) - 1;
	NLOG_CORE(Trace, "Release object ID {}, RefCount: {}", ObjectId, NewRefCount);

	if (NewRefCount <= 0)
	{
		// 引用计数归零，销毁对象
		NLOG_CORE(Debug, "Object ID {} RefCount reached 0, destroying", ObjectId);
		Destroy();
		return 0;
	}

	return NewRefCount;
}

int32_t NObject::GetRefCount() const
{
	return RefCount.load();
}

// === 垃圾回收支持 ===

void NObject::Mark()
{
	if (!bIsValid.load())
	{
		return;
	}

	bool Expected = false;
	if (bMarked.compare_exchange_strong(Expected, true))
	{
		// 首次标记，记录日志
		NLOG_GC(Trace, "Marked object ID: {}", ObjectId);
	}
}

bool NObject::IsMarked() const
{
	return bMarked.load();
}

void NObject::UnMark()
{
	if (!bIsValid.load())
	{
		return;
	}

	bMarked = false;
	NLOG_GC(Trace, "Unmarked object ID: {}", ObjectId);
}

// === 对象信息与反射 ===

const std::type_info& NObject::GetTypeInfo() const
{
	return typeid(NObject);
}

const char* NObject::GetTypeName() const
{
	return "NObject";
}

const SClassReflection* NObject::GetClassReflection() const
{
	return nullptr; // 基类返回空指针
}

// === 对象生命周期 ===

void NObject::Destroy()
{
	if (!bIsValid.load())
	{
		NLOG_CORE(Warning, "Attempted to destroy invalid object ID: {}", ObjectId);
		return;
	}

	NLOG_CORE(Debug, "Destroying object with ID: {}", ObjectId);

	// 调用析构函数
	this->~NObject();

	// 释放内存（暂时使用标准delete，后续用内存管理器替换）
	// 注意：这里需要特殊处理，因为我们禁用了operator delete
	// 实际实现中应该通过内存管理器来处理
}

// === 对象操作 ===

bool NObject::Equals(const NObject* Other) const
{
	return this == Other;
}

size_t NObject::GetHashCode() const
{
	return reinterpret_cast<size_t>(this);
}

CString NObject::ToString() const
{
	// 使用TString构造函数创建格式化字符串
	char Buffer[64];
	std::snprintf(Buffer, sizeof(Buffer), "NObject(ID: %llu)", static_cast<unsigned long long>(ObjectId));
	return CString(Buffer);
}

// === 内存管理操作符 ===

void* NObject::operator new(size_t Size, void* Ptr) noexcept
{
	(void)Size; // 忽略size参数，因为placement new不分配内存
	return Ptr;
}

void NObject::operator delete(void* Ptr) noexcept
{
	if (!Ptr)
	{
		return;
	}

	// 使用内存管理器释放内存
	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.DeallocateObject(Ptr);
	NLOG_MEMORY(Trace, "Deallocated object memory at: {}", Ptr);
}

void NObject::operator delete[](void* Ptr) noexcept
{
	if (!Ptr)
	{
		return;
	}

	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.DeallocateObject(Ptr);
	NLOG_MEMORY(Trace, "Deallocated object array memory at: {}", Ptr);
}

void NObject::operator delete(void* Ptr, std::align_val_t Alignment) noexcept
{
	(void)Alignment; // 忽略对齐参数
	if (!Ptr)
	{
		return;
	}

	auto& MemMgr = CMemoryManager::GetInstance();
	MemMgr.DeallocateObject(Ptr);
	NLOG_MEMORY(Trace, "Deallocated aligned object memory at: {}", Ptr);
}

void NObject::operator delete(void* Ptr, void* PlacementPtr) noexcept
{
	(void)Ptr;          // 忽略参数
	(void)PlacementPtr; // placement delete不释放内存
}

// === GC系统接口 ===

void NObject::RegisterWithGC()
{
	auto& GC = CGarbageCollector::GetInstance();
	if (GC.IsInitialized())
	{
		GC.RegisterObject(this);
	}
	NLOG_GC(Trace, "Registered object ID {} with GC", ObjectId);
}

void NObject::UnregisterFromGC()
{
	auto& GC = CGarbageCollector::GetInstance();
	if (GC.IsInitialized())
	{
		GC.UnregisterObject(this);
	}
	NLOG_GC(Trace, "Unregistered object ID {} from GC", ObjectId);
}

// === 全局函数实现 ===

CMemoryManager& GetMemoryManager()
{
	return CMemoryManager::GetInstance();
}

} // namespace NLib