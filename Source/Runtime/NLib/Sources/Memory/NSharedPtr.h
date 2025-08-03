#pragma once

#include "Core/CObject.h"
#include "Memory/CAllocator.h"
#include "Threading/CAtomic.h"

namespace NLib
{

// 前向声明
template<typename TType> class TWeakPtr;
template<typename TType> class TSharedRef;
template<typename TType> class TWeakRef;

/**
 * @brief 共享指针控制块 - 管理引用计数和对象生命周期
 */
class LIBNLIB_API NSharedPtrControlBlock
{
public:
    NSharedPtrControlBlock() : SharedRefCount(1), WeakRefCount(1), ObjectPtr(nullptr) {}
    virtual ~NSharedPtrControlBlock() = default;

    // 共享引用计数操作
    void AddSharedRef() noexcept
    {
        SharedRefCount.fetch_add(1, std::memory_order_relaxed);
    }

    bool TryAddSharedRef() noexcept
    {
        int32_t CurrentCount = SharedRefCount.load(std::memory_order_acquire);
        while (CurrentCount > 0)
        {
            if (SharedRefCount.compare_exchange_weak(CurrentCount, CurrentCount + 1, std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    void ReleaseSharedRef() noexcept
    {
        if (SharedRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            // 最后一个共享引用，销毁对象
            DestroyObject();
            ReleaseWeakRef();
        }
    }

    int32_t GetSharedRefCount() const noexcept
    {
        return SharedRefCount.load(std::memory_order_acquire);
    }

    // 弱引用计数操作
    void AddWeakRef() noexcept
    {
        WeakRefCount.fetch_add(1, std::memory_order_relaxed);
    }

    void ReleaseWeakRef() noexcept
    {
        if (WeakRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            // 最后一个弱引用，销毁控制块
            DestroyControlBlock();
        }
    }

    int32_t GetWeakRefCount() const noexcept
    {
        return WeakRefCount.load(std::memory_order_acquire);
    }

    // 获取对象指针
    void* GetObjectPtr() const noexcept { return ObjectPtr; }

protected:
    virtual void DestroyObject() = 0;
    virtual void DestroyControlBlock() = 0;

    void SetObjectPtr(void* Ptr) noexcept { ObjectPtr = Ptr; }

private:
    CAtomic<int32_t> SharedRefCount;
    CAtomic<int32_t> WeakRefCount;
    void* ObjectPtr;
};

/**
 * @brief 指针控制块 - 用于已存在的对象指针
 */
template<typename TType, typename TDeleter = NAllocatorDeleter<T>>
class CPointerControlBlock : public NSharedPtrControlBlock
{
public:
    explicit CPointerControlBlock(T* InPtr, TDeleter InDeleter = TDeleter())
        : Ptr(InPtr), Deleter(InDeleter)
    {
        SetObjectPtr(InPtr);
    }

protected:
    virtual void DestroyObject() override
    {
        if (Ptr)
        {
            Deleter(Ptr);
            Ptr = nullptr;
        }
    }

    virtual void DestroyControlBlock() override
    {
        delete this;
    }

private:
    T* Ptr;
    TDeleter Deleter;
};

/**
 * @brief 原地控制块 - 对象和控制块在同一内存块中
 */
template<typename TType>
class CInPlaceControlBlock : public NSharedPtrControlBlock
{
public:
    template<typename... TArgs>
    CInPlaceControlBlock(TArgs&&... Args)
    {
        T* ObjectPtr = reinterpret_cast<T*>(&ObjectStorage);
        new(ObjectPtr) T(Forward<TArgs>(Args)...);
        SetObjectPtr(ObjectPtr);
    }

    T* GetObject() const noexcept
    {
        return reinterpret_cast<T*>(const_cast<typename AlignedStorage<sizeof(T), alignof(T)>::Type*>(&ObjectStorage));
    }

protected:
    virtual void DestroyObject() override
    {
        T* ObjectPtr = GetObject();
        if (ObjectPtr)
        {
            ObjectPtr->~T();
        }
    }

    virtual void DestroyControlBlock() override
    {
        CAllocator* Allocator = CAllocator::GetDefaultAllocator();
        this->~CInPlaceControlBlock();
        Allocator->Free(this);
    }

private:
    typename AlignedStorage<sizeof(T), alignof(T)>::Type ObjectStorage;
};

/**
 * @brief 共享智能指针 - 改进版本，支持控制块和弱引用
 * 
 * 特点：
 * - 引用计数管理
 * - 支持弱引用
 * - 线程安全
 * - 支持自定义删除器
 * - 兼容现有的NObject系统
 */
template<typename TType>
class TSharedPtr
{
public:
    using ElementType = T;

    // 构造函数
    constexpr TSharedPtr() noexcept : Ptr(nullptr), ControlBlock(nullptr) {}
    constexpr TSharedPtr(NullPtr) noexcept : Ptr(nullptr), ControlBlock(nullptr) {}

    explicit TSharedPtr(T* InPtr) : Ptr(InPtr), ControlBlock(nullptr)
    {
        if (InPtr)
        {
            ControlBlock = new CPointerControlBlock<T>(InPtr);
            // 初始化NSharedFromThis（如果适用）
            InitSharedFromThisIfNeeded(InPtr);
        }
    }

    template<typename TDeleter>
    TSharedPtr(T* InPtr, TDeleter Deleter) : Ptr(InPtr), ControlBlock(nullptr)
    {
        if (InPtr)
        {
            ControlBlock = new CPointerControlBlock<T, TDeleter>(InPtr, Deleter);
            // 初始化NSharedFromThis（如果适用）
            InitSharedFromThisIfNeeded(InPtr);
        }
    }

    // 内部构造函数（用于从控制块构造）
    TSharedPtr(T* InPtr, NSharedPtrControlBlock* InControlBlock) noexcept
        : Ptr(InPtr), ControlBlock(InControlBlock) {}

    // 拷贝构造函数
    TSharedPtr(const TSharedPtr& Other) noexcept : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        if (ControlBlock)
        {
            ControlBlock->AddSharedRef();
        }
    }

    template<typename U>
    TSharedPtr(const TSharedPtr<U>& Other) noexcept 
        : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        if (ControlBlock)
        {
            ControlBlock->AddSharedRef();
        }
    }

    // 移动构造函数
    TSharedPtr(TSharedPtr&& Other) noexcept : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
    }

    template<typename U>
    TSharedPtr(TSharedPtr<U>&& Other) noexcept : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
    }

    // 从弱指针构造
    template<typename U>
    explicit TSharedPtr(const TWeakPtr<U>& WeakPtr) : Ptr(nullptr), ControlBlock(nullptr)
    {
        if (WeakPtr.GetControlBlock() && WeakPtr.GetControlBlock()->TryAddSharedRef())
        {
            Ptr = static_cast<T*>(WeakPtr.GetControlBlock()->GetObjectPtr());
            ControlBlock = WeakPtr.GetControlBlock();
        }
    }

    // 析构函数
    ~TSharedPtr()
    {
        if (ControlBlock)
        {
            ControlBlock->ReleaseSharedRef();
        }
    }

    // 赋值操作符
    TSharedPtr& operator=(const TSharedPtr& Other) noexcept
    {
        if (this != &Other)
        {
            if (ControlBlock)
            {
                ControlBlock->ReleaseSharedRef();
            }
            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;
            if (ControlBlock)
            {
                ControlBlock->AddSharedRef();
            }
        }
        return *this;
    }

    template<typename U>
    TSharedPtr& operator=(const TSharedPtr<U>& Other) noexcept
    {
        if (ControlBlock)
        {
            ControlBlock->ReleaseSharedRef();
        }
        Ptr = Other.Ptr;
        ControlBlock = Other.ControlBlock;
        if (ControlBlock)
        {
            ControlBlock->AddSharedRef();
        }
        return *this;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            if (ControlBlock)
            {
                ControlBlock->ReleaseSharedRef();
            }
            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;
            Other.Ptr = nullptr;
            Other.ControlBlock = nullptr;
        }
        return *this;
    }

    template<typename U>
    TSharedPtr& operator=(TSharedPtr<U>&& Other) noexcept
    {
        if (ControlBlock)
        {
            ControlBlock->ReleaseSharedRef();
        }
        Ptr = Other.Ptr;
        ControlBlock = Other.ControlBlock;
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
        return *this;
    }

    TSharedPtr& operator=(NullPtr) noexcept
    {
        Reset();
        return *this;
    }

    // 访问操作符
    T& operator*() const noexcept
    {
        checkSlow(Ptr != nullptr);
        return *Ptr;
    }

    T* operator->() const noexcept
    {
        checkSlow(Ptr != nullptr);
        return Ptr;
    }

    // 获取原始指针
    T* Get() const noexcept { return Ptr; }

    // 检查是否有效
    explicit operator bool() const noexcept { return Ptr != nullptr; }
    bool IsValid() const noexcept { return Ptr != nullptr; }

    // 引用计数
    int32_t GetSharedReferenceCount() const noexcept
    {
        return ControlBlock ? ControlBlock->GetSharedRefCount() : 0;
    }

    bool IsUnique() const noexcept
    {
        return GetSharedReferenceCount() == 1;
    }

    // 重置
    void Reset() noexcept
    {
        if (ControlBlock)
        {
            ControlBlock->ReleaseSharedRef();
            ControlBlock = nullptr;
        }
        Ptr = nullptr;
    }

    void Reset(T* NewPtr)
    {
        TSharedPtr Temp(NewPtr);
        Swap(Temp);
    }

    template<typename TDeleter>
    void Reset(T* NewPtr, TDeleter Deleter)
    {
        TSharedPtr Temp(NewPtr, Deleter);
        Swap(Temp);
    }

    // 交换
    void Swap(TSharedPtr& Other) noexcept
    {
        NLib::Swap(Ptr, Other.Ptr);
        NLib::Swap(ControlBlock, Other.ControlBlock);
    }

    // 获取控制块（内部使用）
    NSharedPtrControlBlock* GetControlBlock() const noexcept
    {
        return ControlBlock;
    }

    // 所有者比较（基于控制块地址）
    template<typename U>
    bool OwnerBefore(const TSharedPtr<U>& Other) const noexcept
    {
        return ControlBlock < Other.ControlBlock;
    }

    template<typename U>
    bool OwnerBefore(const TWeakPtr<U>& Other) const noexcept
    {
        return ControlBlock < Other.GetControlBlock();
    }

private:
    T* Ptr;
    NSharedPtrControlBlock* ControlBlock;

    // 允许其他模板实例和相关类访问私有成员
    template<typename U> friend class TSharedPtr;
    template<typename U> friend class TWeakPtr;
    template<typename U> friend class TSharedRef;
    template<typename U> friend class TWeakRef;

private:
    // 初始化NSharedFromThis的辅助方法
    void InitSharedFromThisIfNeeded(T* InPtr)
    {
        // 使用SFINAE检测是否继承自NSharedFromThis
        InitSharedFromThisImpl(InPtr, 0);
    }

private:
    // 当类型继承自NSharedFromThis时调用此版本
    template<typename U = T>
    auto InitSharedFromThisImpl(U* InPtr, int) -> decltype(InPtr->InitWeakThis(*this), void())
    {
        if (InPtr)
        {
            InPtr->InitWeakThis(*this);
        }
    }

    // 当类型不继承自NSharedFromThis时调用此版本
    void InitSharedFromThisImpl(T* InPtr, ...)
    {
        // 什么都不做
        (void)InPtr;
    }
};

// 比较操作符
template<typename TType, typename U>
bool operator==(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return Left.Get() == Right.Get();
}

template<typename TType, typename U>
bool operator!=(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator<(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return Left.Get() < Right.Get();
}

template<typename TType, typename U>
bool operator<=(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return !(Right < Left);
}

template<typename TType, typename U>
bool operator>(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return Right < Left;
}

template<typename TType, typename U>
bool operator>=(const TSharedPtr<T>& Left, const TSharedPtr<U>& Right) noexcept
{
    return !(Left < Right);
}

template<typename TType>
bool operator==(const TSharedPtr<T>& Left, NullPtr) noexcept
{
    return !Left;
}

template<typename TType>
bool operator==(NullPtr, const TSharedPtr<T>& Right) noexcept
{
    return !Right;
}

template<typename TType>
bool operator!=(const TSharedPtr<T>& Left, NullPtr) noexcept
{
    return static_cast<bool>(Left);
}

template<typename TType>
bool operator!=(NullPtr, const TSharedPtr<T>& Right) noexcept
{
    return static_cast<bool>(Right);
}

// 工厂函数
template<typename TType, typename... TArgs>
TSharedPtr<T> MakeShared(TArgs&&... Args)
{
    CAllocator* Allocator = CAllocator::GetDefaultAllocator();
    auto* ControlBlock = static_cast<CInPlaceControlBlock<T>*>(
        Allocator->Allocate(sizeof(CInPlaceControlBlock<T>), alignof(CInPlaceControlBlock<T>))
    );
    
    new(ControlBlock) CInPlaceControlBlock<T>(Forward<TArgs>(Args)...);
    TSharedPtr<T> Result(ControlBlock->GetObject(), ControlBlock);
    
    // 初始化NSharedFromThis（如果适用）
    Result.InitSharedFromThisIfNeeded(Result.Get());
    
    return Result;
}

template<typename TType, typename... TArgs>
TSharedPtr<T> MakeSharedWithAllocator(CAllocator* Allocator, TArgs&&... Args)
{
    auto* ControlBlock = static_cast<CInPlaceControlBlock<T>*>(
        Allocator->Allocate(sizeof(CInPlaceControlBlock<T>), alignof(CInPlaceControlBlock<T>))
    );
    
    new(ControlBlock) CInPlaceControlBlock<T>(Forward<TArgs>(Args)...);
    TSharedPtr<T> Result(ControlBlock->GetObject(), ControlBlock);
    
    // 初始化NSharedFromThis（如果适用）
    Result.InitSharedFromThisIfNeeded(Result.Get());
    
    return Result;
}

// 类型转换函数
template<typename TType, typename U>
TSharedPtr<T> StaticCastSharedPtr(const TSharedPtr<U>& InSharedPtr) noexcept
{
    T* CastedPtr = static_cast<T*>(InSharedPtr.Get());
    return TSharedPtr<T>(CastedPtr, InSharedPtr.GetControlBlock());
}

template<typename TType, typename U>
TSharedPtr<T> DynamicCastSharedPtr(const TSharedPtr<U>& InSharedPtr) noexcept
{
    T* CastedPtr = dynamic_cast<T*>(InSharedPtr.Get());
    if (CastedPtr)
    {
        return TSharedPtr<T>(CastedPtr, InSharedPtr.GetControlBlock());
    }
    return TSharedPtr<T>();
}

template<typename TType, typename U>
TSharedPtr<T> ConstCastSharedPtr(const TSharedPtr<U>& InSharedPtr) noexcept
{
    T* CastedPtr = const_cast<T*>(InSharedPtr.Get());
    return TSharedPtr<T>(CastedPtr, InSharedPtr.GetControlBlock());
}

// 交换函数
template<typename TType>
void Swap(TSharedPtr<T>& Left, TSharedPtr<T>& Right) noexcept
{
    Left.Swap(Right);
}

} // namespace NLib