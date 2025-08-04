#pragma once

/**
 * @file SmartPointers.h
 * @brief Nut引擎智能指针系统
 *
 * 提供类似std::shared_ptr但与NObject系统集成的智能指针实现
 * 特性：
 * - 线程安全的引用计数
 * - 与tcmalloc内存管理器集成
 * - 支持弱引用和循环引用检测
 * - 自定义删除器支持
 * - 高性能的实现
 */

#include "Logging/LogCategory.h"
#include "Memory/MemoryManager.h"

#include <atomic>
#include <functional>
#include <type_traits>
#include <utility>

namespace NLib
{
// 前向声明
template <typename TType>
class TSharedPtr;
template <typename TType>
class TUniquePtr;
template <typename TType>
class TWeakPtr;
template <typename TType>
class TSharedRef;
template <typename TType>
class TWeakRef;
class TSharedFromThis;

namespace Internal
{
/**
 * @brief 引用计数控制块
 *
 * 管理对象的强引用和弱引用计数，线程安全
 */
struct SRefCountBlock
{
	std::atomic<int32_t> StrongRefCount; // 强引用计数
	std::atomic<int32_t> WeakRefCount;   // 弱引用计数
	std::function<void(void*)> Deleter;  // 自定义删除器
	void* ObjectPtr;                     // 被管理对象指针

	SRefCountBlock(void* InObjectPtr, std::function<void(void*)> InDeleter = nullptr)
	    : StrongRefCount(1),
	      WeakRefCount(1),
	      Deleter(InDeleter ? std::move(InDeleter) : DefaultDeleter),
	      ObjectPtr(InObjectPtr)
	{
		NLOG(LogCore, Debug, "RefCountBlock created for object at {}", ObjectPtr);
	}

	~SRefCountBlock()
	{
		NLOG(LogCore, Debug, "RefCountBlock destroyed for object at {}", ObjectPtr);
	}

	// 增加强引用计数
	void AddStrongRef()
	{
		StrongRefCount.fetch_add(1, std::memory_order_relaxed);
	}

	// 减少强引用计数，如果为0则删除对象
	void ReleaseStrongRef()
	{
		if (StrongRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			// 最后一个强引用，删除对象
			if (ObjectPtr && Deleter)
			{
				Deleter(ObjectPtr);
				ObjectPtr = nullptr;
			}

			// 减少弱引用计数（控制块本身的引用）
			ReleaseWeakRef();
		}
	}

	// 增加弱引用计数
	void AddWeakRef()
	{
		WeakRefCount.fetch_add(1, std::memory_order_relaxed);
	}

	// 减少弱引用计数，如果为0则删除控制块
	void ReleaseWeakRef()
	{
		if (WeakRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			// 最后一个弱引用，删除控制块
			delete this;
		}
	}

	// 尝试获取强引用（用于weak_ptr->shared_ptr转换）
	bool TryAddStrongRef()
	{
		int32_t Current = StrongRefCount.load(std::memory_order_relaxed);
		while (Current > 0)
		{
			if (StrongRefCount.compare_exchange_weak(Current, Current + 1, std::memory_order_acq_rel))
			{
				return true;
			}
		}
		return false;
	}

	// 获取当前强引用计数
	int32_t GetStrongRefCount() const
	{
		return StrongRefCount.load(std::memory_order_relaxed);
	}

	// 获取当前弱引用计数
	int32_t GetWeakRefCount() const
	{
		return WeakRefCount.load(std::memory_order_relaxed);
	}

	// 检查对象是否还存活
	bool IsAlive() const
	{
		return StrongRefCount.load(std::memory_order_relaxed) > 0;
	}

private:
	// 默认删除器
	static void DefaultDeleter(void* Ptr)
	{
		if (Ptr)
		{
			// 使用内存管理器释放内存
			extern CMemoryManager& GetMemoryManager();
			auto& MemMgr = GetMemoryManager();
			MemMgr.DeallocateObject(Ptr);
		}
	}
};

/**
 * @brief 智能指针基类
 * 提供共同的功能和接口
 */
template <typename TType>
class TSmartPtrBase
{
protected:
	TType* Ptr;               // 指向的对象
	SRefCountBlock* RefBlock; // 引用计数控制块

public:
	using ElementType = TType;
	using PointerType = TType*;

protected:
	TSmartPtrBase()
	    : Ptr(nullptr),
	      RefBlock(nullptr)
	{}

	TSmartPtrBase(TType* InPtr, SRefCountBlock* InRefBlock)
	    : Ptr(InPtr),
	      RefBlock(InRefBlock)
	{}

	~TSmartPtrBase() = default;

	// 获取原始指针
	TType* GetPtr() const
	{
		return Ptr;
	}

	// 获取引用计数块
	SRefCountBlock* GetRefBlock() const
	{
		return RefBlock;
	}

	// 重置指针和引用计数块
	void Reset(TType* InPtr = nullptr, SRefCountBlock* InRefBlock = nullptr)
	{
		Ptr = InPtr;
		RefBlock = InRefBlock;
	}
};

/**
 * @brief 用于enable_shared_from_this功能的弱引用存储
 */
template <typename TType>
struct SWeakThisStorage
{
	mutable TWeakPtr<TType> WeakThis;
};

} // namespace Internal

/**
 * @brief 共享智能指针
 *
 * 类似std::shared_ptr，支持引用计数的共享所有权
 */
template <typename TType>
class TSharedPtr : public Internal::TSmartPtrBase<TType>
{
	using BaseType = Internal::TSmartPtrBase<TType>;
	using BaseType::Ptr;
	using BaseType::RefBlock;

public:
	using ElementType = typename BaseType::ElementType;
	using PointerType = typename BaseType::PointerType;

	// === 构造函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TSharedPtr()
	    : BaseType()
	{}

	/**
	 * @brief nullptr构造函数
	 */
	TSharedPtr(std::nullptr_t)
	    : BaseType()
	{}

	/**
	 * @brief 从原始指针构造
	 * @param InPtr 原始指针
	 */
	explicit TSharedPtr(TType* InPtr)
	    : BaseType()
	{
		if (InPtr)
		{
			RefBlock = new Internal::SRefCountBlock(InPtr);
			Ptr = InPtr;

			// 如果支持shared_from_this，设置弱引用
			SetupSharedFromThis(InPtr);
		}
	}

	/**
	 * @brief 从原始指针和自定义删除器构造
	 * @param InPtr 原始指针
	 * @param InDeleter 自定义删除器
	 */
	template <typename TDeleter>
	TSharedPtr(TType* InPtr, TDeleter&& InDeleter)
	    : BaseType()
	{
		if (InPtr)
		{
			RefBlock = new Internal::SRefCountBlock(
			    InPtr, [Deleter = std::forward<TDeleter>(InDeleter)](void* Ptr) { Deleter(static_cast<TType*>(Ptr)); });
			Ptr = InPtr;
			SetupSharedFromThis(InPtr);
		}
	}

	/**
	 * @brief 拷贝构造函数
	 * @param Other 其他shared_ptr
	 */
	TSharedPtr(const TSharedPtr& Other)
	    : BaseType(Other.Ptr, Other.RefBlock)
	{
		if (RefBlock)
		{
			RefBlock->AddStrongRef();
		}
	}

	/**
	 * @brief 从不同类型的shared_ptr拷贝构造（支持继承转换）
	 * @param Other 其他类型的shared_ptr
	 */
	template <typename TOther>
	TSharedPtr(const TSharedPtr<TOther>& Other,
	           typename std::enable_if_t<std::is_convertible_v<TOther*, TType*>>* = nullptr)
	    : BaseType(Other.Get(), Other.GetRefCountBlock())
	{
		if (RefBlock)
		{
			RefBlock->AddStrongRef();
		}
	}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的shared_ptr
	 */
	TSharedPtr(TSharedPtr&& Other) noexcept
	    : BaseType(Other.Ptr, Other.RefBlock)
	{
		Other.Reset();
	}

	/**
	 * @brief 从weak_ptr构造
	 * @param WeakPtr 弱引用指针
	 */
	explicit TSharedPtr(const TWeakPtr<TType>& WeakPtr);

	/**
	 * @brief 析构函数
	 */
	~TSharedPtr()
	{
		if (RefBlock)
		{
			RefBlock->ReleaseStrongRef();
		}
	}

	// === 赋值操作符 ===

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 其他shared_ptr
	 * @return 自身引用
	 */
	TSharedPtr& operator=(const TSharedPtr& Other)
	{
		if (this != &Other)
		{
			TSharedPtr Temp(Other);
			Swap(Temp);
		}
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的shared_ptr
	 * @return 自身引用
	 */
	TSharedPtr& operator=(TSharedPtr&& Other) noexcept
	{
		if (this != &Other)
		{
			TSharedPtr Temp(std::move(Other));
			Swap(Temp);
		}
		return *this;
	}

	/**
	 * @brief nullptr赋值
	 * @return 自身引用
	 */
	TSharedPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	// === 访问操作符 ===

	/**
	 * @brief 解引用操作符
	 * @return 对象引用
	 */
	TType& operator*() const
	{
		return *Ptr;
	}

	/**
	 * @brief 箭头操作符
	 * @return 对象指针
	 */
	TType* operator->() const
	{
		return Ptr;
	}

	/**
	 * @brief 获取原始指针
	 * @return 原始指针
	 */
	TType* Get() const
	{
		return Ptr;
	}

	/**
	 * @brief 检查是否为有效指针
	 * @return true if valid, false otherwise
	 */
	explicit operator bool() const
	{
		return Ptr != nullptr;
	}

	// === 引用计数管理 ===

	/**
	 * @brief 获取引用计数
	 * @return 当前引用计数
	 */
	int32_t GetRefCount() const
	{
		return RefBlock ? RefBlock->GetStrongRefCount() : 0;
	}

	/**
	 * @brief 检查是否是唯一引用
	 * @return true if unique, false otherwise
	 */
	bool IsUnique() const
	{
		return GetRefCount() == 1;
	}

	/**
	 * @brief 重置智能指针
	 * @param InPtr 新的原始指针
	 */
	void Reset(TType* InPtr = nullptr)
	{
		if (InPtr)
		{
			TSharedPtr Temp(InPtr);
			Swap(Temp);
		}
		else
		{
			TSharedPtr Temp;
			Swap(Temp);
		}
	}

	/**
	 * @brief 交换两个智能指针
	 * @param Other 要交换的智能指针
	 */
	void Swap(TSharedPtr& Other) noexcept
	{
		std::swap(Ptr, Other.Ptr);
		std::swap(RefBlock, Other.RefBlock);
	}

	// === 比较操作符 ===

	template <typename TOther>
	bool operator==(const TSharedPtr<TOther>& Other) const
	{
		return Ptr == Other.Get();
	}

	template <typename TOther>
	bool operator!=(const TSharedPtr<TOther>& Other) const
	{
		return !(*this == Other);
	}

	template <typename TOther>
	bool operator<(const TSharedPtr<TOther>& Other) const
	{
		return Ptr < Other.Get();
	}

	bool operator==(std::nullptr_t) const
	{
		return Ptr == nullptr;
	}

	bool operator!=(std::nullptr_t) const
	{
		return Ptr != nullptr;
	}

	// === 类型转换 ===

	/**
	 * @brief 静态类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的智能指针
	 */
	template <typename TTarget>
	TSharedPtr<TTarget> StaticCast() const
	{
		TSharedPtr<TTarget> Result;
		if (Ptr)
		{
			Result.Ptr = static_cast<TTarget*>(Ptr);
			Result.RefBlock = RefBlock;
			if (RefBlock)
			{
				RefBlock->AddStrongRef();
			}
		}
		return Result;
	}

	/**
	 * @brief 动态类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的智能指针，失败返回空指针
	 */
	template <typename TTarget>
	TSharedPtr<TTarget> DynamicCast() const
	{
		TSharedPtr<TTarget> Result;
		if (Ptr)
		{
			TTarget* CastedPtr = dynamic_cast<TTarget*>(Ptr);
			if (CastedPtr)
			{
				Result.Ptr = CastedPtr;
				Result.RefBlock = RefBlock;
				if (RefBlock)
				{
					RefBlock->AddStrongRef();
				}
			}
		}
		return Result;
	}

	/**
	 * @brief 常量类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的智能指针
	 */
	template <typename TTarget>
	TSharedPtr<TTarget> ConstCast() const
	{
		TSharedPtr<TTarget> Result;
		if (Ptr)
		{
			Result.Ptr = const_cast<TTarget*>(Ptr);
			Result.RefBlock = RefBlock;
			if (RefBlock)
			{
				RefBlock->AddStrongRef();
			}
		}
		return Result;
	}

	// === 内部访问 ===

	/**
	 * @brief 获取引用计数块（内部使用）
	 * @return 引用计数块指针
	 */
	Internal::SRefCountBlock* GetRefCountBlock() const
	{
		return RefBlock;
	}

private:
	// 设置shared_from_this支持
	template <typename TPtr>
	void SetupSharedFromThis(TPtr* InPtr)
	{
		if constexpr (std::is_base_of_v<TSharedFromThis, TPtr>)
		{
			if (InPtr)
			{
				auto* SharedFromThis = static_cast<TSharedFromThis*>(InPtr);
				SharedFromThis->Internal_SetWeakThis(TSharedPtr<TPtr>(*this));
			}
		}
	}

	// 友元声明
	template <typename TOther>
	friend class TSharedPtr;
	template <typename TOther>
	friend class TWeakPtr;
	friend class TSharedFromThis;
};

// === TSharedPtr 工厂函数 ===

/**
 * @brief 创建shared_ptr
 * @tparam TType 对象类型
 * @tparam TArgs 构造参数类型
 * @param Args 构造参数
 * @return 新创建的shared_ptr
 */
template <typename TType, typename... TArgs>
TSharedPtr<TType> MakeShared(TArgs&&... Args)
{
	// 使用内存管理器分配内存
	extern CMemoryManager& GetMemoryManager();
	auto& MemMgr = GetMemoryManager();

	void* Memory = MemMgr.AllocateObject(sizeof(TType));
	if (!Memory)
	{
		throw std::bad_alloc();
	}

	try
	{
		// 使用placement new构造对象
		TType* Obj = new (Memory) TType(std::forward<TArgs>(Args)...);
		return TSharedPtr<TType>(Obj);
	}
	catch (...)
	{
		MemMgr.DeallocateObject(Memory);
		throw;
	}
}

/**
 * @brief 弱引用智能指针
 *
 * 类似std::weak_ptr，不影响引用计数的观察者指针
 */
template <typename TType>
class TWeakPtr : public Internal::TSmartPtrBase<TType>
{
	using BaseType = Internal::TSmartPtrBase<TType>;
	using BaseType::Ptr;
	using BaseType::RefBlock;

public:
	using ElementType = typename BaseType::ElementType;

	// === 构造函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TWeakPtr()
	    : BaseType()
	{}

	/**
	 * @brief 从shared_ptr构造
	 * @param SharedPtr 共享指针
	 */
	TWeakPtr(const TSharedPtr<TType>& SharedPtr)
	    : BaseType(SharedPtr.Get(), SharedPtr.GetRefCountBlock())
	{
		if (RefBlock)
		{
			RefBlock->AddWeakRef();
		}
	}

	/**
	 * @brief 拷贝构造函数
	 * @param Other 其他weak_ptr
	 */
	TWeakPtr(const TWeakPtr& Other)
	    : BaseType(Other.Ptr, Other.RefBlock)
	{
		if (RefBlock)
		{
			RefBlock->AddWeakRef();
		}
	}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的weak_ptr
	 */
	TWeakPtr(TWeakPtr&& Other) noexcept
	    : BaseType(Other.Ptr, Other.RefBlock)
	{
		Other.Reset();
	}

	/**
	 * @brief 析构函数
	 */
	~TWeakPtr()
	{
		if (RefBlock)
		{
			RefBlock->ReleaseWeakRef();
		}
	}

	// === 赋值操作符 ===

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 其他weak_ptr
	 * @return 自身引用
	 */
	TWeakPtr& operator=(const TWeakPtr& Other)
	{
		if (this != &Other)
		{
			TWeakPtr Temp(Other);
			Swap(Temp);
		}
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的weak_ptr
	 * @return 自身引用
	 */
	TWeakPtr& operator=(TWeakPtr&& Other) noexcept
	{
		if (this != &Other)
		{
			TWeakPtr Temp(std::move(Other));
			Swap(Temp);
		}
		return *this;
	}

	/**
	 * @brief 从shared_ptr赋值
	 * @param SharedPtr 共享指针
	 * @return 自身引用
	 */
	TWeakPtr& operator=(const TSharedPtr<TType>& SharedPtr)
	{
		TWeakPtr Temp(SharedPtr);
		Swap(Temp);
		return *this;
	}

	// === 观察功能 ===

	/**
	 * @brief 检查对象是否还存活
	 * @return true if alive, false otherwise
	 */
	bool IsValid() const
	{
		return RefBlock && RefBlock->IsAlive();
	}

	/**
	 * @brief 检查weak_ptr是否已过期
	 * @return true if expired, false otherwise
	 */
	bool IsExpired() const
	{
		return !IsValid();
	}

	/**
	 * @brief 获取引用计数
	 * @return 当前强引用计数
	 */
	int32_t GetRefCount() const
	{
		return RefBlock ? RefBlock->GetStrongRefCount() : 0;
	}

	/**
	 * @brief 尝试锁定获取shared_ptr
	 * @return 如果对象存活返回shared_ptr，否则返回空指针
	 */
	TSharedPtr<TType> Lock() const
	{
		TSharedPtr<TType> Result;
		if (RefBlock && RefBlock->TryAddStrongRef())
		{
			Result.Ptr = Ptr;
			Result.RefBlock = RefBlock;
		}
		return Result;
	}

	/**
	 * @brief 重置weak_ptr
	 */
	void Reset()
	{
		TWeakPtr Temp;
		Swap(Temp);
	}

	/**
	 * @brief 交换两个weak_ptr
	 * @param Other 要交换的weak_ptr
	 */
	void Swap(TWeakPtr& Other) noexcept
	{
		std::swap(Ptr, Other.Ptr);
		std::swap(RefBlock, Other.RefBlock);
	}

	// === 比较操作符 ===

	template <typename TOther>
	bool operator==(const TWeakPtr<TOther>& Other) const
	{
		return Ptr == Other.Ptr;
	}

	template <typename TOther>
	bool operator!=(const TWeakPtr<TOther>& Other) const
	{
		return !(*this == Other);
	}

	template <typename TOther>
	bool operator<(const TWeakPtr<TOther>& Other) const
	{
		return Ptr < Other.Ptr;
	}

private:
	// 友元声明
	template <typename TOther>
	friend class TWeakPtr;
	template <typename TOther>
	friend class TSharedPtr;
	friend class TSharedFromThis;
};

// TSharedPtr从TWeakPtr构造的实现
template <typename TType>
TSharedPtr<TType>::TSharedPtr(const TWeakPtr<TType>& WeakPtr)
    : BaseType()
{
	if (WeakPtr.RefBlock && WeakPtr.RefBlock->TryAddStrongRef())
	{
		Ptr = WeakPtr.Ptr;
		RefBlock = WeakPtr.RefBlock;
	}
}

/**
 * @brief 独占智能指针
 *
 * 类似std::unique_ptr，独占所有权的智能指针
 */
template <typename TType, typename TDeleter = std::default_delete<TType>>
class TUniquePtr
{
public:
	using ElementType = TType;
	using PointerType = TType*;
	using DeleterType = TDeleter;

private:
	TType* Ptr;
	TDeleter Deleter;

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TUniquePtr()
	    : Ptr(nullptr),
	      Deleter()
	{}

	/**
	 * @brief nullptr构造函数
	 */
	TUniquePtr(std::nullptr_t)
	    : Ptr(nullptr),
	      Deleter()
	{}

	/**
	 * @brief 从原始指针构造
	 * @param InPtr 原始指针
	 */
	explicit TUniquePtr(TType* InPtr)
	    : Ptr(InPtr),
	      Deleter()
	{}

	/**
	 * @brief 从原始指针和删除器构造
	 * @param InPtr 原始指针
	 * @param InDeleter 删除器
	 */
	TUniquePtr(TType* InPtr, const TDeleter& InDeleter)
	    : Ptr(InPtr),
	      Deleter(InDeleter)
	{}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的unique_ptr
	 */
	TUniquePtr(TUniquePtr&& Other) noexcept
	    : Ptr(Other.Release()),
	      Deleter(std::move(Other.Deleter))
	{}

	/**
	 * @brief 从不同类型的unique_ptr移动构造
	 * @param Other 其他类型的unique_ptr
	 */
	template <typename TOther, typename TOtherDeleter>
	TUniquePtr(TUniquePtr<TOther, TOtherDeleter>&& Other,
	           typename std::enable_if_t<std::is_convertible_v<TOther*, TType*> &&
	                                     std::is_convertible_v<TOtherDeleter, TDeleter>>* = nullptr)
	    : Ptr(Other.Release()),
	      Deleter(std::move(Other.GetDeleter()))
	{}

	/**
	 * @brief 析构函数
	 */
	~TUniquePtr()
	{
		if (Ptr)
		{
			Deleter(Ptr);
		}
	}

	// 禁用拷贝
	TUniquePtr(const TUniquePtr&) = delete;
	TUniquePtr& operator=(const TUniquePtr&) = delete;

	// === 赋值操作符 ===

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的unique_ptr
	 * @return 自身引用
	 */
	TUniquePtr& operator=(TUniquePtr&& Other) noexcept
	{
		if (this != &Other)
		{
			Reset(Other.Release());
			Deleter = std::move(Other.Deleter);
		}
		return *this;
	}

	/**
	 * @brief nullptr赋值
	 * @return 自身引用
	 */
	TUniquePtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	// === 访问操作符 ===

	/**
	 * @brief 解引用操作符
	 * @return 对象引用
	 */
	TType& operator*() const
	{
		return *Ptr;
	}

	/**
	 * @brief 箭头操作符
	 * @return 对象指针
	 */
	TType* operator->() const
	{
		return Ptr;
	}

	/**
	 * @brief 获取原始指针
	 * @return 原始指针
	 */
	TType* Get() const
	{
		return Ptr;
	}

	/**
	 * @brief 检查是否为有效指针
	 * @return true if valid, false otherwise
	 */
	explicit operator bool() const
	{
		return Ptr != nullptr;
	}

	// === 所有权管理 ===

	/**
	 * @brief 释放所有权并返回原始指针
	 * @return 原始指针
	 */
	TType* Release()
	{
		TType* Temp = Ptr;
		Ptr = nullptr;
		return Temp;
	}

	/**
	 * @brief 重置unique_ptr
	 * @param InPtr 新的原始指针
	 */
	void Reset(TType* InPtr = nullptr)
	{
		if (Ptr != InPtr)
		{
			if (Ptr)
			{
				Deleter(Ptr);
			}
			Ptr = InPtr;
		}
	}

	/**
	 * @brief 交换两个unique_ptr
	 * @param Other 要交换的unique_ptr
	 */
	void Swap(TUniquePtr& Other) noexcept
	{
		std::swap(Ptr, Other.Ptr);
		std::swap(Deleter, Other.Deleter);
	}

	/**
	 * @brief 获取删除器
	 * @return 删除器引用
	 */
	TDeleter& GetDeleter()
	{
		return Deleter;
	}
	const TDeleter& GetDeleter() const
	{
		return Deleter;
	}

	// === 比较操作符 ===

	template <typename TOther, typename TOtherDeleter>
	bool operator==(const TUniquePtr<TOther, TOtherDeleter>& Other) const
	{
		return Ptr == Other.Get();
	}

	template <typename TOther, typename TOtherDeleter>
	bool operator!=(const TUniquePtr<TOther, TOtherDeleter>& Other) const
	{
		return !(*this == Other);
	}

	template <typename TOther, typename TOtherDeleter>
	bool operator<(const TUniquePtr<TOther, TOtherDeleter>& Other) const
	{
		return Ptr < Other.Get();
	}

	bool operator==(std::nullptr_t) const
	{
		return Ptr == nullptr;
	}

	bool operator!=(std::nullptr_t) const
	{
		return Ptr != nullptr;
	}

	// 友元声明
	template <typename TOther, typename TOtherDeleter>
	friend class TUniquePtr;
};

// === TUniquePtr 工厂函数 ===

/**
 * @brief 创建unique_ptr
 * @tparam TType 对象类型
 * @tparam TArgs 构造参数类型
 * @param Args 构造参数
 * @return 新创建的unique_ptr
 */
template <typename TType, typename... TArgs>
TUniquePtr<TType> MakeUnique(TArgs&&... Args)
{
	// 使用内存管理器分配内存
	extern CMemoryManager& GetMemoryManager();
	auto& MemMgr = GetMemoryManager();

	void* Memory = MemMgr.AllocateObject(sizeof(TType));
	if (!Memory)
	{
		throw std::bad_alloc();
	}

	try
	{
		// 使用placement new构造对象
		TType* Obj = new (Memory) TType(std::forward<TArgs>(Args)...);
		return TUniquePtr<TType>(Obj);
	}
	catch (...)
	{
		MemMgr.DeallocateObject(Memory);
		throw;
	}
}

/**
 * @brief TSharedFromThis混入类
 *
 * 允许对象从this指针获取shared_ptr，类似std::enable_shared_from_this
 */
class TSharedFromThis
{
public:
	/**
	 * @brief 从this获取shared_ptr
	 * @tparam TType 派生类类型
	 * @return this的shared_ptr
	 */
	template <typename TType>
	TSharedPtr<TType> SharedFromThis()
	{
		auto* Storage = GetWeakThisStorage<TType>();
		return Storage ? Storage->WeakThis.Lock() : TSharedPtr<TType>();
	}

	/**
	 * @brief 从this获取shared_ptr（常量版本）
	 * @tparam TType 派生类类型
	 * @return this的shared_ptr
	 */
	template <typename TType>
	TSharedPtr<const TType> SharedFromThis() const
	{
		auto* Storage = GetWeakThisStorage<TType>();
		return Storage ? Storage->WeakThis.Lock() : TSharedPtr<const TType>();
	}

	/**
	 * @brief 从this获取weak_ptr
	 * @tparam TType 派生类类型
	 * @return this的weak_ptr
	 */
	template <typename TType>
	TWeakPtr<TType> WeakFromThis()
	{
		auto* Storage = GetWeakThisStorage<TType>();
		return Storage ? Storage->WeakThis : TWeakPtr<TType>();
	}

	/**
	 * @brief 从this获取weak_ptr（常量版本）
	 * @tparam TType 派生类类型
	 * @return this的weak_ptr
	 */
	template <typename TType>
	TWeakPtr<const TType> WeakFromThis() const
	{
		auto* Storage = GetWeakThisStorage<TType>();
		return Storage ? Storage->WeakThis : TWeakPtr<const TType>();
	}

protected:
	TSharedFromThis() = default;
	TSharedFromThis(const TSharedFromThis&) = default;
	TSharedFromThis& operator=(const TSharedFromThis&) = default;
	virtual ~TSharedFromThis() = default;

	// 内部方法，由TSharedPtr调用
	template <typename TType>
	void Internal_SetWeakThis(const TSharedPtr<TType>& SharedPtr)
	{
		auto* Storage = GetWeakThisStorage<TType>();
		if (Storage)
		{
			Storage->WeakThis = SharedPtr;
		}
	}

private:
	template <typename TType>
	Internal::SWeakThisStorage<TType>* GetWeakThisStorage()
	{
		static_assert(std::is_base_of_v<TSharedFromThis, TType>, "TType must inherit from TSharedFromThis");

		// 使用静态成员变量存储weak_ptr
		static thread_local Internal::SWeakThisStorage<TType> Storage;
		return &Storage;
	}

	template <typename TType>
	const Internal::SWeakThisStorage<TType>* GetWeakThisStorage() const
	{
		return const_cast<TSharedFromThis*>(this)->GetWeakThisStorage<TType>();
	}

	// 友元声明
	template <typename TType>
	friend class TSharedPtr;
};

/**
 * @brief 非空共享引用
 *
 * 类似TSharedPtr但保证永不为空的引用语义
 */
template <typename TType>
class TSharedRef
{
private:
	TSharedPtr<TType> SharedPtr;

public:
	using ElementType = TType;
	using PointerType = TType*;

	// === 构造函数 ===

	/**
	 * @brief 从TSharedPtr构造（必须非空）
	 * @param InSharedPtr 共享指针
	 */
	explicit TSharedRef(const TSharedPtr<TType>& InSharedPtr)
	    : SharedPtr(InSharedPtr)
	{
		if (!SharedPtr)
		{
			NLOG(LogCore, Error, "TSharedRef constructed with null TSharedPtr");
			throw std::invalid_argument("TSharedRef cannot be constructed with null TSharedPtr");
		}
	}

	/**
	 * @brief 从TSharedPtr移动构造（必须非空）
	 * @param InSharedPtr 要移动的共享指针
	 */
	explicit TSharedRef(TSharedPtr<TType>&& InSharedPtr)
	    : SharedPtr(std::move(InSharedPtr))
	{
		if (!SharedPtr)
		{
			NLOG(LogCore, Error, "TSharedRef constructed with null TSharedPtr");
			throw std::invalid_argument("TSharedRef cannot be constructed with null TSharedPtr");
		}
	}

	/**
	 * @brief 从原始指针构造
	 * @param InPtr 原始指针（必须非空）
	 */
	explicit TSharedRef(TType* InPtr)
	    : SharedPtr(InPtr)
	{
		if (!SharedPtr)
		{
			NLOG(LogCore, Error, "TSharedRef constructed with null pointer");
			throw std::invalid_argument("TSharedRef cannot be constructed with null pointer");
		}
	}

	/**
	 * @brief 拷贝构造函数
	 * @param Other 其他TSharedRef
	 */
	TSharedRef(const TSharedRef& Other)
	    : SharedPtr(Other.SharedPtr)
	{}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的TSharedRef
	 */
	TSharedRef(TSharedRef&& Other) noexcept
	    : SharedPtr(std::move(Other.SharedPtr))
	{}

	/**
	 * @brief 从不同类型的TSharedRef构造
	 * @param Other 其他类型的TSharedRef
	 */
	template <typename TOther>
	TSharedRef(const TSharedRef<TOther>& Other,
	           typename std::enable_if_t<std::is_convertible_v<TOther*, TType*>>* = nullptr)
	    : SharedPtr(Other.ToSharedPtr())
	{}

	// === 赋值操作符 ===

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 其他TSharedRef
	 * @return 自身引用
	 */
	TSharedRef& operator=(const TSharedRef& Other)
	{
		SharedPtr = Other.SharedPtr;
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的TSharedRef
	 * @return 自身引用
	 */
	TSharedRef& operator=(TSharedRef&& Other) noexcept
	{
		SharedPtr = std::move(Other.SharedPtr);
		return *this;
	}

	// === 访问操作符 ===

	/**
	 * @brief 解引用操作符
	 * @return 对象引用
	 */
	TType& operator*() const
	{
		return *SharedPtr;
	}

	/**
	 * @brief 箭头操作符
	 * @return 对象指针
	 */
	TType* operator->() const
	{
		return SharedPtr.Get();
	}

	/**
	 * @brief 获取原始指针
	 * @return 原始指针（保证非空）
	 */
	TType* Get() const
	{
		return SharedPtr.Get();
	}

	// === 转换操作 ===

	/**
	 * @brief 转换为TSharedPtr
	 * @return 对应的TSharedPtr
	 */
	TSharedPtr<TType> ToSharedPtr() const
	{
		return SharedPtr;
	}

	/**
	 * @brief 隐式转换为TSharedPtr
	 * @return 对应的TSharedPtr
	 */
	operator TSharedPtr<TType>() const
	{
		return SharedPtr;
	}

	// === 引用计数管理 ===

	/**
	 * @brief 获取引用计数
	 * @return 当前引用计数
	 */
	int32_t GetRefCount() const
	{
		return SharedPtr.GetRefCount();
	}

	/**
	 * @brief 检查是否是唯一引用
	 * @return true if unique, false otherwise
	 */
	bool IsUnique() const
	{
		return SharedPtr.IsUnique();
	}

	// === 比较操作符 ===

	template <typename TOther>
	bool operator==(const TSharedRef<TOther>& Other) const
	{
		return SharedPtr == Other.ToSharedPtr();
	}

	template <typename TOther>
	bool operator!=(const TSharedRef<TOther>& Other) const
	{
		return !(*this == Other);
	}

	template <typename TOther>
	bool operator<(const TSharedRef<TOther>& Other) const
	{
		return SharedPtr < Other.ToSharedPtr();
	}

	template <typename TOther>
	bool operator==(const TSharedPtr<TOther>& Other) const
	{
		return SharedPtr == Other;
	}

	template <typename TOther>
	bool operator!=(const TSharedPtr<TOther>& Other) const
	{
		return SharedPtr != Other;
	}

	// === 类型转换 ===

	/**
	 * @brief 静态类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的TSharedRef
	 */
	template <typename TTarget>
	TSharedRef<TTarget> StaticCast() const
	{
		return TSharedRef<TTarget>(SharedPtr.template StaticCast<TTarget>());
	}

	/**
	 * @brief 动态类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的TSharedPtr（可能为空）
	 */
	template <typename TTarget>
	TSharedPtr<TTarget> DynamicCast() const
	{
		return SharedPtr.template DynamicCast<TTarget>();
	}

	/**
	 * @brief 常量类型转换
	 * @tparam TTarget 目标类型
	 * @return 转换后的TSharedRef
	 */
	template <typename TTarget>
	TSharedRef<TTarget> ConstCast() const
	{
		return TSharedRef<TTarget>(SharedPtr.template ConstCast<TTarget>());
	}

private:
	// 友元声明
	template <typename TOther>
	friend class TSharedRef;
};

/**
 * @brief 非空弱引用
 *
 * 类似TWeakPtr但在构造时保证引用的对象存在
 */
template <typename TType>
class TWeakRef
{
private:
	TWeakPtr<TType> WeakPtr;

public:
	using ElementType = TType;

	// === 构造函数 ===

	/**
	 * @brief 从TSharedRef构造
	 * @param SharedRef 共享引用
	 */
	explicit TWeakRef(const TSharedRef<TType>& SharedRef)
	    : WeakPtr(SharedRef.ToSharedPtr())
	{}

	/**
	 * @brief 从TSharedPtr构造（必须非空）
	 * @param SharedPtr 共享指针
	 */
	explicit TWeakRef(const TSharedPtr<TType>& SharedPtr)
	    : WeakPtr(SharedPtr)
	{
		if (!SharedPtr)
		{
			NLOG(LogCore, Error, "TWeakRef constructed with null TSharedPtr");
			throw std::invalid_argument("TWeakRef cannot be constructed with null TSharedPtr");
		}
	}

	/**
	 * @brief 拷贝构造函数
	 * @param Other 其他TWeakRef
	 */
	TWeakRef(const TWeakRef& Other)
	    : WeakPtr(Other.WeakPtr)
	{}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的TWeakRef
	 */
	TWeakRef(TWeakRef&& Other) noexcept
	    : WeakPtr(std::move(Other.WeakPtr))
	{}

	// === 赋值操作符 ===

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 其他TWeakRef
	 * @return 自身引用
	 */
	TWeakRef& operator=(const TWeakRef& Other)
	{
		WeakPtr = Other.WeakPtr;
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的TWeakRef
	 * @return 自身引用
	 */
	TWeakRef& operator=(TWeakRef&& Other) noexcept
	{
		WeakPtr = std::move(Other.WeakPtr);
		return *this;
	}

	/**
	 * @brief 从TSharedRef赋值
	 * @param SharedRef 共享引用
	 * @return 自身引用
	 */
	TWeakRef& operator=(const TSharedRef<TType>& SharedRef)
	{
		WeakPtr = SharedRef.ToSharedPtr();
		return *this;
	}

	/**
	 * @brief 从TSharedPtr赋值（必须非空）
	 * @param SharedPtr 共享指针
	 * @return 自身引用
	 */
	TWeakRef& operator=(const TSharedPtr<TType>& SharedPtr)
	{
		if (!SharedPtr)
		{
			NLOG(LogCore, Error, "TWeakRef assigned with null TSharedPtr");
			throw std::invalid_argument("TWeakRef cannot be assigned with null TSharedPtr");
		}
		WeakPtr = SharedPtr;
		return *this;
	}

	// === 观察功能 ===

	/**
	 * @brief 检查对象是否还存活
	 * @return true if alive, false otherwise
	 */
	bool IsValid() const
	{
		return WeakPtr.IsValid();
	}

	/**
	 * @brief 检查weak_ref是否已过期
	 * @return true if expired, false otherwise
	 */
	bool IsExpired() const
	{
		return WeakPtr.IsExpired();
	}

	/**
	 * @brief 获取引用计数
	 * @return 当前强引用计数
	 */
	int32_t GetRefCount() const
	{
		return WeakPtr.GetRefCount();
	}

	/**
	 * @brief 尝试锁定获取TSharedPtr
	 * @return 如果对象存活返回TSharedPtr，否则返回空指针
	 */
	TSharedPtr<TType> Lock() const
	{
		return WeakPtr.Lock();
	}

	/**
	 * @brief 尝试锁定获取TSharedRef
	 * @return 如果对象存活返回TSharedRef，否则抛出异常
	 */
	TSharedRef<TType> LockRef() const
	{
		auto LockedPtr = WeakPtr.Lock();
		if (!LockedPtr)
		{
			NLOG(LogCore, Error, "TWeakRef::LockRef() failed: object has been destroyed");
			throw std::runtime_error("TWeakRef::LockRef() failed: object has been destroyed");
		}
		return TSharedRef<TType>(std::move(LockedPtr));
	}

	/**
	 * @brief 转换为TWeakPtr
	 * @return 对应的TWeakPtr
	 */
	TWeakPtr<TType> ToWeakPtr() const
	{
		return WeakPtr;
	}

	/**
	 * @brief 隐式转换为TWeakPtr
	 * @return 对应的TWeakPtr
	 */
	operator TWeakPtr<TType>() const
	{
		return WeakPtr;
	}

	// === 比较操作符 ===

	template <typename TOther>
	bool operator==(const TWeakRef<TOther>& Other) const
	{
		return WeakPtr == Other.ToWeakPtr();
	}

	template <typename TOther>
	bool operator!=(const TWeakRef<TOther>& Other) const
	{
		return !(*this == Other);
	}

	template <typename TOther>
	bool operator<(const TWeakRef<TOther>& Other) const
	{
		return WeakPtr < Other.ToWeakPtr();
	}

private:
	// 友元声明
	template <typename TOther>
	friend class TWeakRef;
};

// === 工厂函数和辅助函数 ===

/**
 * @brief 创建TSharedRef
 * @tparam TType 对象类型
 * @tparam TArgs 构造参数类型
 * @param Args 构造参数
 * @return 新创建的TSharedRef
 */
template <typename TType, typename... TArgs>
TSharedRef<TType> MakeSharedRef(TArgs&&... Args)
{
	auto SharedPtr = MakeShared<TType>(std::forward<TArgs>(Args)...);
	return TSharedRef<TType>(std::move(SharedPtr));
}

// === 全局操作符重载 ===

// TSharedPtr 比较操作符
template <typename TType>
bool operator==(std::nullptr_t, const TSharedPtr<TType>& Ptr)
{
	return Ptr == nullptr;
}

template <typename TType>
bool operator!=(std::nullptr_t, const TSharedPtr<TType>& Ptr)
{
	return Ptr != nullptr;
}

// TUniquePtr 比较操作符
template <typename TType, typename TDeleter>
bool operator==(std::nullptr_t, const TUniquePtr<TType, TDeleter>& Ptr)
{
	return Ptr == nullptr;
}

template <typename TType, typename TDeleter>
bool operator!=(std::nullptr_t, const TUniquePtr<TType, TDeleter>& Ptr)
{
	return Ptr != nullptr;
}

// === 类型别名 ===

// 常用智能指针类型
template <typename TType>
using TSharedPtrType = TSharedPtr<TType>;

template <typename TType>
using TUniquePtrType = TUniquePtr<TType>;

template <typename TType>
using TWeakPtrType = TWeakPtr<TType>;

template <typename TType>
using TSharedRefType = TSharedRef<TType>;

template <typename TType>
using TWeakRefType = TWeakRef<TType>;

} // namespace NLib