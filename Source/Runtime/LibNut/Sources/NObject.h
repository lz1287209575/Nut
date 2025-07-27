#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include <typeinfo>
#include <new>
#include <cstddef>


#include "NAllocator.h"

namespace Nut {

// 前向声明
class NGarbageCollector;
class NObject;

// 智能指针类型定义
template<typename T>
class NSharedPtr;

/**
 * @brief NObject - 所有托管对象的基类
 * 
 * 提供引用计数、垃圾回收支持和基础对象功能
 * 所有需要被GC管理的对象都应该继承自这个类
 * 
 */
class NObject {
public:
    NObject();
    virtual ~NObject();
    
    // === tcmalloc 内存分配器重载 ===
    
    /**
     * @brief 重载new操作符，使用tcmalloc分配内存
     * @param Size 分配的字节数
     * @return 分配的内存指针
     */
    static void* operator new(size_t Size);
    
    /**
     * @brief 重载delete操作符，使用tcmalloc释放内存
     * @param Ptr 要释放的内存指针
     */
    static void operator delete(void* Ptr) noexcept;
    
    /**
     * @brief 重载new[]操作符，使用tcmalloc分配数组内存
     * @param Size 分配的字节数
     * @return 分配的内存指针
     */
    static void* operator new[](size_t Size);
    
    /**
     * @brief 重载delete[]操作符，使用tcmalloc释放数组内存
     * @param Ptr 要释放的内存指针
     */
    static void operator delete[](void* Ptr) noexcept;
    
    /**
     * @brief 带对齐的new操作符重载
     * @param Size 分配的字节数
     * @param Alignment 对齐要求
     * @return 分配的内存指针
     */
    static void* operator new(size_t Size, std::align_val_t Alignment);
    
    /**
     * @brief 带对齐的delete操作符重载
     * @param Ptr 要释放的内存指针
     * @param Alignment 对齐要求
     */
    static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;

    // 禁用拷贝构造和赋值操作符，强制通过指针管理
    NObject(const NObject&) = delete;
    NObject& operator=(const NObject&) = delete;
    
    // 允许移动构造（但需要小心处理引用计数）
    NObject(NObject&& Other) noexcept;
    NObject& operator=(NObject&& Other) noexcept;

public:
    // === 引用计数管理 ===
    
    /**
     * @brief 增加引用计数
     * @return 新的引用计数值
     */
    int32_t AddRef();
    
    /**
     * @brief 减少引用计数，可能触发对象删除
     * @return 新的引用计数值
     */
    int32_t Release();
    
    /**
     * @brief 获取当前引用计数
     * @return 当前引用计数值
     */
    int32_t GetRefCount() const;

public:
    // === GC 支持 ===
    
    /**
     * @brief 标记此对象为可达（GC标记阶段使用）
     */
    void Mark();
    
    /**
     * @brief 检查对象是否被标记
     * @return true if marked, false otherwise
     */
    bool IsMarked() const;
    
    /**
     * @brief 清除标记（GC清理阶段使用）
     */
    void UnMark();
    
    /**
     * @brief 收集此对象引用的其他NObject对象
     * 子类需要重写此方法来报告它们持有的其他NObject引用
     * @param OutReferences 输出收集到的引用对象（使用tcmalloc分配器）
     */
    virtual void CollectReferences(NVector<NObject*>& OutReferences) const { (void)OutReferences; }

public:
    // === 对象信息 ===
    
    /**
     * @brief 获取对象的类型信息
     * @return 类型信息
     */
    virtual const std::type_info& GetTypeInfo() const;
    
    /**
     * @brief 获取对象的类型名称
     * @return 类型名称字符串
     */
    virtual const char* GetTypeName() const;
    
    /**
     * @brief 获取对象的唯一ID
     * @return 对象ID
     */
    uint64_t GetObjectId() const { return ObjectId; }
    
    /**
     * @brief 检查对象是否有效（未被销毁）
     * @return true if valid, false otherwise  
     */
    bool IsValid() const { return bIsValid; }

public:
    // === 工厂方法 ===
    
    /**
     * @brief 创建托管对象的工厂方法
     * @tparam T 对象类型（必须继承自NObject）
     * @tparam Args 构造函数参数类型
     * @param Args 构造函数参数
     * @return 托管对象的智能指针
     */
    template<typename T, typename... ArgsType>
    static NSharedPtr<T> Create(ArgsType&&... Args);

protected:
    // === 内部状态 ===
    
    std::atomic<int32_t> RefCount;      // 引用计数
    std::atomic<bool> bMarked;          // GC标记
    std::atomic<bool> bIsValid;         // 对象是否有效
    uint64_t ObjectId;                  // 对象唯一ID
    
    static std::atomic<uint64_t> NextObjectId;  // 下一个对象ID

private:
    friend class NGarbageCollector;
    friend class NObjectRegistry;
    
    // GC使用的内部方法
    void RegisterWithGC();
    void UnregisterFromGC();
};

/**
 * @brief 智能指针类，用于自动管理NObject的引用计数
 * @tparam T 指向的对象类型
 * 
 * 类似UE的TSharedPtr，但专门为NObject设计
 */
template<typename T>
class NSharedPtr {
    static_assert(std::is_base_of_v<NObject, T>, "T must inherit from NObject");

public:
    NSharedPtr() : Ptr(nullptr) {}
    NSharedPtr(T* INSharedPtr) : Ptr(INSharedPtr) {
        if (Ptr) Ptr->AddRef();
    }
    
    NSharedPtr(const NSharedPtr& Other) : Ptr(Other.Ptr) {
        if (Ptr) Ptr->AddRef();
    }
    
    NSharedPtr(NSharedPtr&& Other) noexcept : Ptr(Other.Ptr) {
        Other.Ptr = nullptr;
    }
    
    ~NSharedPtr() {
        if (Ptr) Ptr->Release();
    }
    
    NSharedPtr& operator=(const NSharedPtr& Other) {
        if (this != &Other) {
            if (Ptr) Ptr->Release();
            Ptr = Other.Ptr;
            if (Ptr) Ptr->AddRef();
        }
        return *this;
    }
    
    NSharedPtr& operator=(NSharedPtr&& Other) noexcept {
        if (this != &Other) {
            if (Ptr) Ptr->Release();
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }
        return *this;
    }
    
    NSharedPtr& operator=(T* INSharedPtr) {
        if (Ptr != INSharedPtr) {
            if (Ptr) Ptr->Release();
            Ptr = INSharedPtr;
            if (Ptr) Ptr->AddRef();
        }
        return *this;
    }
    
    T* operator->() const { return Ptr; }
    T& operator*() const { return *Ptr; }
    T* Get() const { return Ptr; }
    
    bool IsValid() const { return Ptr != nullptr && Ptr->IsValid(); }
    operator bool() const { return IsValid(); }
    
    void Reset() {
        if (Ptr) {
            Ptr->Release();
            Ptr = nullptr;
        }
    }
    
    // 类型转换 - 类似UE的StaticCastSharedPtr
    template<typename U>
    NSharedPtr<U> StaticCast() const {
        static_assert(std::is_base_of_v<NObject, U>, "U must inherit from NObject");
        return NSharedPtr<U>(static_cast<U*>(Ptr));
    }
    
    // 动态类型转换 - 类似UE的DynamicCastSharedPtr  
    template<typename U>
    NSharedPtr<U> DynamicCast() const {
        static_assert(std::is_base_of_v<NObject, U>, "U must inherit from NObject");
        return NSharedPtr<U>(dynamic_cast<U*>(Ptr));
    }
    
    // 比较操作符
    bool operator==(const NSharedPtr& Other) const { return Ptr == Other.Ptr; }
    bool operator!=(const NSharedPtr& Other) const { return Ptr != Other.Ptr; }
    bool operator==(const T* Other) const { return Ptr == Other; }
    bool operator!=(const T* Other) const { return Ptr != Other; }

private:
    T* Ptr;
    
    template<typename U> friend class NSharedPtr;
    friend class NObject;
};

// === 工厂方法实现 ===

template<typename T, typename... ArgsType>
NSharedPtr<T> NObject::Create(ArgsType&&... Args) {
    static_assert(std::is_base_of_v<NObject, T>, "T must inherit from NObject");
    
    // 直接构造对象，构造函数会自动注册到GC
    T* Obj = new T(std::forward<ArgsType>(Args)...);
    return NSharedPtr<T>(Obj);
}

// === 类型别名 ===
using NObjectPtr = NSharedPtr<NObject>;

// === 辅助宏定义（类似UE的DECLARE_CLASS） ===

/**
 * @brief 在类中声明NObject相关的基础功能
 * 用于子类中声明类型信息等
 */
#define DECLARE_NOBJECT_CLASS(ClassName, SuperClass) \
public: \
    using Super = SuperClass; \
    virtual const std::type_info& GetTypeInfo() const override { return typeid(ClassName); } \
    virtual const char* GetTypeName() const override { return #ClassName; } \
private:

/**
 * @brief 声明抽象NObject类
 */
#define DECLARE_NOBJECT_ABSTRACT_CLASS(ClassName, SuperClass) \
public: \
    using Super = SuperClass; \
    virtual const std::type_info& GetTypeInfo() const override { return typeid(ClassName); } \
    virtual const char* GetTypeName() const override { return #ClassName; } \
private:

} // namespace Nut
