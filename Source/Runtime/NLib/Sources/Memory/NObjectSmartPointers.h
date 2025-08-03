#pragma once

/**
 * @file NObjectSmartPointers.h
 * @brief NObject专用智能指针扩展
 * 
 * 这个文件提供了与现有NObject系统兼容的智能指针实现，
 * 同时扩展了新的智能指针系统功能。
 */

#include "Core/CObject.h"
#include "Memory/NSmartPointers.h"

namespace NLib
{

/**
 * @brief NObject专用的共享指针控制块
 * 
 * 与NObject的引用计数系统集成，避免双重引用计数
 */
template<typename TType>
class CObjectControlBlock : public NSharedPtrControlBlock
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");

public:
    explicit CObjectControlBlock(T* InObject) : Object(InObject)
    {
        SetObjectPtr(InObject);
        // NObject已有自己的引用计数，我们需要协调
        if (Object)
        {
            Object->AddRef();
        }
    }

protected:
    virtual void DestroyObject() override
    {
        if (Object)
        {
            Object->Release(); // 使用NObject的Release机制
            Object = nullptr;
        }
    }

    virtual void DestroyControlBlock() override
    {
        delete this;
    }

private:
    T* Object;
};

/**
 * @brief 从现有NObject指针创建新式智能指针
 */
template<typename TType>
TSharedPtr<T> ToNewSharedPtr(T* ObjectPtr)
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
    
    if (!ObjectPtr)
    {
        return TSharedPtr<T>();
    }
    
    auto* ControlBlock = new CObjectControlBlock<T>(ObjectPtr);
    return TSharedPtr<T>(ObjectPtr, ControlBlock);
}

/**
 * @brief 从新式智能指针转换为旧式NSharedPtr
 */
template<typename TType>
typename std::enable_if_t<std::is_base_of_v<CObject, T>, NLib::TSharedPtr<T>> ToLegacySharedPtr(const TSharedPtr<T>& ModernPtr)
{
    return NLib::TSharedPtr<T>(ModernPtr.Get());
}

/**
 * @brief NObject的智能指针工厂函数扩展
 */
template<typename TType, typename... ArgsType>
TSharedPtr<TType> NewNObjectModern(ArgsType&&... Args)
{
    static_assert(std::is_base_of_v<CObject, TType>, "TType must inherit from CObject");
    
    // 使用MakeShared创建对象
    return MakeShared<TType>(Forward<ArgsType>(Args)...);
}

template<typename TType, typename... ArgsType>
TSharedRef<TType> NewNObjectRef(ArgsType&&... Args)
{
    static_assert(std::is_base_of_v<CObject, TType>, "TType must inherit from CObject");
    
    return MakeSharedRef<TType>(Forward<ArgsType>(Args)...);
}

template<typename TType, typename... ArgsType>
CUniquePtr<TType> NewNObjectUnique(ArgsType&&... Args)
{
    static_assert(std::is_base_of_v<CObject, TType>, "TType must inherit from CObject");
    
    return MakeUnique<TType>(Forward<ArgsType>(Args)...);
}

/**
 * @brief NObject智能指针类型别名
 */
using NObjectSharedPtr = TSharedPtr<CObject>;
using NObjectSharedRef = TSharedRef<CObject>;
using NObjectWeakPtr = TWeakPtr<CObject>;
using NObjectWeakRef = TWeakRef<CObject>;
using NObjectUniquePtr = CUniquePtr<CObject>;

/**
 * @brief 智能指针转换函数 - NObject特化版本
 */
template<typename TType, typename U>
TSharedPtr<T> StaticCastNObject(const TSharedPtr<U>& InPtr)
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
    static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
    
    return StaticCastSharedPtr<T>(InPtr);
}

template<typename TType, typename U>
TSharedPtr<T> DynamicCastNObject(const TSharedPtr<U>& InPtr)
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
    static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
    
    return DynamicCastSharedPtr<T>(InPtr);
}

template<typename TType, typename U>
TSharedRef<T> StaticCastNObjectRef(const TSharedRef<U>& InRef)
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
    static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
    
    return StaticCastSharedRef<T>(InRef);
}

template<typename TType, typename U>
TSharedRef<T> DynamicCastNObjectRef(const TSharedRef<U>& InRef)
{
    static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
    static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
    
    return DynamicCastSharedRef<T>(InRef);
}

/**
 * @brief 便利宏定义
 */
#define DECLARE_NOBJECT_SMART_PTRS(ClassName) \
    using ClassName##Ptr = TSharedPtr<ClassName>; \
    using ClassName##Ref = TSharedRef<ClassName>; \
    using ClassName##WeakPtr = TWeakPtr<ClassName>; \
    using ClassName##WeakRef = TWeakRef<ClassName>; \
    using ClassName##UniquePtr = CUniquePtr<ClassName>;

/**
 * @brief 智能指针兼容性检查
 * 
 * 在过渡期间，可以同时使用新旧两套智能指针系统
 */
template<typename TType>
struct IsLegacyNSharedPtr : std::false_type {};

template<typename TType>
struct IsLegacyNSharedPtr<NLib::TSharedPtr<T>> : std::true_type {};

template<typename TType>
struct IsModernNSharedPtr : std::false_type {};

template<typename TType>
struct IsModernNSharedPtr<TSharedPtr<T>> : std::true_type {};

/**
 * @brief 智能指针统一接口
 * 
 * 提供统一的访问接口，不管是旧式还是新式智能指针
 */
template<typename TPtrType>
auto GetRawPtr(const PtrType& Ptr) -> decltype(Ptr.Get())
{
    return Ptr.Get();
}

template<typename TPtrType>
bool IsValidPtr(const PtrType& Ptr)
{
    return Ptr.IsValid();
}

template<typename TPtrType>
int32_t GetRefCount(const PtrType& Ptr)
{
    if constexpr (IsModernNSharedPtr<PtrType>::value)
    {
        return Ptr.GetSharedReferenceCount();
    }
    else if constexpr (IsLegacyNSharedPtr<PtrType>::value)
    {
        return Ptr.Get() ? Ptr.Get()->GetRefCount() : 0;
    }
    else
    {
        return 1; // UniquePtr 或其他类型
    }
}

} // namespace NLib