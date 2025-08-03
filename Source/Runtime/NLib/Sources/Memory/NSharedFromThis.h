#pragma once

#include "Memory/TSharedPtr.h"
#include "Memory/TWeakPtr.h"

namespace NLib
{

/**
 * @brief 允许对象从自身获取NSharedPtr的基类
 * 
 * 特点：
 * - 类似std::enable_shared_from_this
 * - 允许对象安全地创建指向自身的NSharedPtr
 * - 避免双重管理同一对象
 * - 提供线程安全的实现
 * 
 * 使用方法：
 * class MyClass : public CSharedFromThis<MyClass>
 * {
 * public:
 *     void SomeMethod()
 *     {
 *         auto self = SharedFromThis();
 *         // 使用self...
 *     }
 * };
 * 
 * auto obj = MakeShared<MyClass>();
 */
template<typename TType>
class CSharedFromThis
{
public:
    CSharedFromThis() noexcept = default;
    CSharedFromThis(const CSharedFromThis&) noexcept = default;
    CSharedFromThis& operator=(const CSharedFromThis&) noexcept = default;
    virtual ~CSharedFromThis() = default;

    /**
     * @brief 获取指向此对象的NSharedPtr
     * @return 指向此对象的NSharedPtr
     * @throws std::bad_weak_ptr 如果对象未被NSharedPtr管理
     */
    TSharedPtr<T> SharedFromThis()
    {
        TSharedPtr<T> Result = WeakThis.Lock();
        if (!Result.IsValid())
        {
            // 对象未被NSharedPtr管理
            checkf(false, TEXT("SharedFromThis() called on object not managed by TSharedPtr"));
        }
        return Result;
    }

    TSharedPtr<const T> SharedFromThis() const
    {
        TSharedPtr<const T> Result = WeakThis.Lock();
        if (!Result.IsValid())
        {
            // 对象未被NSharedPtr管理
            checkf(false, TEXT("SharedFromThis() called on object not managed by TSharedPtr"));
        }
        return Result;
    }

    /**
     * @brief 获取指向此对象的NSharedRef
     * @return 指向此对象的NSharedRef
     * @throws std::bad_weak_ptr 如果对象未被NSharedPtr管理
     */
    TSharedRef<T> SharedRefFromThis()
    {
        TSharedPtr<T> Ptr = SharedFromThis();
        return TSharedRef<T>(Ptr);
    }

    TSharedRef<const T> SharedRefFromThis() const
    {
        TSharedPtr<const T> Ptr = SharedFromThis();
        return TSharedRef<const T>(Ptr);
    }

    /**
     * @brief 尝试获取指向此对象的NSharedPtr（不抛出异常）
     * @return 如果对象被NSharedPtr管理则返回有效指针，否则返回空指针
     */
    TSharedPtr<T> TrySharedFromThis() noexcept
    {
        return WeakThis.Lock();
    }

    TSharedPtr<const T> TrySharedFromThis() const noexcept
    {
        return WeakThis.Lock();
    }

    /**
     * @brief 尝试获取指向此对象的NSharedRef
     * @return 如果对象被NSharedPtr管理则返回有效引用，否则返回空的Optional
     */
    TOptional<TSharedRef<T>> TrySharedRefFromThis() noexcept
    {
        TSharedPtr<T> Ptr = TrySharedFromThis();
        if (Ptr.IsValid())
        {
            return TSharedRef<T>(Ptr);
        }
        return {};
    }

    TOptional<TSharedRef<const T>> TrySharedRefFromThis() const noexcept
    {
        TSharedPtr<const T> Ptr = TrySharedFromThis();
        if (Ptr.IsValid())
        {
            return TSharedRef<const T>(Ptr);
        }
        return {};
    }

    /**
     * @brief 获取弱引用
     * @return 指向此对象的NWeakPtr
     */
    TWeakPtr<T> WeakFromThis() noexcept
    {
        return WeakThis;
    }

    TWeakPtr<const T> WeakFromThis() const noexcept
    {
        return WeakThis;
    }

    /**
     * @brief 获取弱引用
     * @return 指向此对象的NWeakRef
     */
    TWeakRef<T> WeakRefFromThis() noexcept
    {
        return TWeakRef<T>(WeakThis);
    }

    TWeakRef<const T> WeakRefFromThis() const noexcept
    {
        return TWeakRef<const T>(WeakThis);
    }

private:
    template<typename U> friend class TSharedPtr;
    template<typename U> friend class TSharedRef;

    // 这个方法由NSharedPtr在构造时调用，用于初始化弱引用
    void InitWeakThis(const TSharedPtr<T>& SharedPtr) noexcept
    {
        WeakThis = SharedPtr;
    }

    mutable TWeakPtr<T> WeakThis;
};

/**
 * @brief NSharedFromThis的特化版本，用于支持NSharedPtr的自动初始化
 */
namespace Detail
{
    // 检测类型是否继承自NSharedFromThis
    template<typename TType>
    struct IsSharedFromThis : std::false_type {};

    template<typename TType>
    struct IsSharedFromThis<CSharedFromThis<T>> : std::true_type {};

    template<typename TType>
    struct InheritsFromSharedFromThis
    {
        template<typename U>
        static auto Test(int) -> decltype(static_cast<CSharedFromThis<U>*>(static_cast<U*>(nullptr)), std::true_type{});
        
        template<typename>
        static std::false_type Test(...);

        static constexpr bool Value = decltype(Test<T>(0))::value;
    };

    template<typename TType>
    constexpr bool InheritsFromSharedFromThis_v = InheritsFromSharedFromThis<T>::Value;

    // 初始化NSharedFromThis的辅助函数
    template<typename TType>
    void InitSharedFromThis(T* Ptr, const TSharedPtr<T>& SharedPtr) noexcept
    {
        if constexpr (InheritsFromSharedFromThis_v<T>)
        {
            if (Ptr)
            {
                static_cast<CSharedFromThis<T>*>(Ptr)->InitWeakThis(SharedPtr);
            }
        }
    }
}

} // namespace NLib