#pragma once

#include "Memory/CAllocator.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <typeinfo>
#include <vector>

namespace NLib
{

// 前向声明
class CGarbageCollector;
class CObject;
class CString;
struct NClassReflection;

// 容器前向声明
template <typename TType>
class CArray;

// 智能指针类型定义
template <typename TType>
class TSharedPtr;

/**
 * @brief CObject - 所有托管对象的基类
 *
 * 提供引用计数、垃圾回收支持和基础对象功能
 * 所有需要被GC管理的对象都应该继承自这个类
 *
 */
class CObject
{
public:
	CObject();
	virtual ~CObject();

	// === 销毁函数 ===
	/**
	 * @brief 销毁对象并释放内存
	 * 不应该直接调用delete，而应该调用这个方法
	 */
	void Destroy();

	// === 内存管理 ===
	// 注意：NObject不应该使用标准的new创建，应该使用工厂函数NewNObject<T>()
	// 但需要保留delete用于虚析构函数

private:
	static void* operator new(size_t Size) = delete;
	static void* operator new[](size_t Size) = delete;
	static void* operator new(size_t Size, std::align_val_t Alignment) = delete;

public:
	// 允许placement new（用于在已分配内存上构造对象）
	static void* operator new(size_t Size, void* Ptr) noexcept;

	static void operator delete(void* Ptr) noexcept;
	static void operator delete[](void* Ptr) noexcept;
	static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;
	static void operator delete(void* Ptr, void* PlacementPtr) noexcept;

	// 禁用拷贝构造和赋值操作符，强制通过指针管理
	CObject(const CObject&) = delete;
	CObject& operator=(const CObject&) = delete;

	// 允许移动构造（但需要小心处理引用计数）
	CObject(CObject&& Other) noexcept;
	CObject& operator=(CObject&& Other) noexcept;

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
	virtual void CollectReferences(CArray<CObject*>& OutReferences) const
	{
		(void)OutReferences;
	}

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
	 * @brief 获取类的反射信息
	 * @return 类反射信息指针
	 */
	virtual const struct NClassReflection* GetClassReflection() const;

	/**
	 * @brief 获取静态类型名称
	 * @return 静态类型名称字符串
	 */
	static const char* GetStaticTypeName()
	{
		return "CObject";
	}

	/**
	 * @brief 获取对象的唯一ID
	 * @return 对象ID
	 */
	uint64_t GetObjectId() const
	{
		return ObjectId;
	}

	/**
	 * @brief 检查对象是否有效（未被销毁）
	 * @return true if valid, false otherwise
	 */
	bool IsValid() const
	{
		return bIsValid;
	}

	/**
	 * @brief 比较两个对象是否相等
	 * @param Other 要比较的对象
	 * @return true if equal, false otherwise
	 */
	virtual bool Equals(const CObject* Other) const;

	/**
	 * @brief 获取对象的哈希码
	 * @return 哈希码
	 */
	virtual size_t GetHashCode() const;

	/**
	 * @brief 获取对象的字符串表示
	 * @return 字符串表示
	 */
	virtual CString ToString() const;

	// === 注意：不提供Create方法，请使用全局NewNObject<T>()函数 ===

protected:
	// === 内部状态 ===

	std::atomic<int32_t> RefCount; // 引用计数
	std::atomic<bool> bMarked;     // GC标记
	std::atomic<bool> bIsValid;    // 对象是否有效
	uint64_t ObjectId;             // 对象唯一ID

	static std::atomic<uint64_t> NextObjectId; // 下一个对象ID

private:
	friend class CGarbageCollector;
	friend class CObjectRegistry;

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
template <typename TType>
class TSharedPtr
{
public:
	TSharedPtr()
	    : Ptr(nullptr)
	{
		// 在构造函数中进行类型检查，避免循环依赖
		static_assert(std::is_base_of_v<CObject, T>, "T must inherit from CObject");
	}
	TSharedPtr(T* INSharedPtr)
	    : Ptr(INSharedPtr)
	{
		if (Ptr)
			Ptr->AddRef();
	}

	TSharedPtr(const TSharedPtr& Other)
	    : Ptr(Other.Ptr)
	{
		if (Ptr)
			Ptr->AddRef();
	}

	TSharedPtr(TSharedPtr&& Other) noexcept
	    : Ptr(Other.Ptr)
	{
		Other.Ptr = nullptr;
	}

	~TSharedPtr()
	{
		if (Ptr)
			Ptr->Release();
	}

	TSharedPtr& operator=(const TSharedPtr& Other)
	{
		if (this != &Other)
		{
			if (Ptr)
				Ptr->Release();
			Ptr = Other.Ptr;
			if (Ptr)
				Ptr->AddRef();
		}
		return *this;
	}

	TSharedPtr& operator=(TSharedPtr&& Other) noexcept
	{
		if (this != &Other)
		{
			if (Ptr)
				Ptr->Release();
			Ptr = Other.Ptr;
			Other.Ptr = nullptr;
		}
		return *this;
	}

	TSharedPtr& operator=(T* INSharedPtr)
	{
		if (Ptr != INSharedPtr)
		{
			if (Ptr)
				Ptr->Release();
			Ptr = INSharedPtr;
			if (Ptr)
				Ptr->AddRef();
		}
		return *this;
	}

	T* operator->() const
	{
		return Ptr;
	}
	T& operator*() const
	{
		return *Ptr;
	}
	T* Get() const
	{
		return Ptr;
	}

	bool IsValid() const
	{
		return Ptr != nullptr && Ptr->IsValid();
	}
	operator bool() const
	{
		return IsValid();
	}

	void Reset()
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
	}

	// 类型转换 - 类似UE的StaticCastSharedPtr
	template <typename U>
	TSharedPtr<U> StaticCast() const
	{
		static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
		return TSharedPtr<U>(static_cast<U*>(Ptr));
	}

	// 动态类型转换 - 类似UE的DynamicCastSharedPtr
	template <typename U>
	TSharedPtr<U> DynamicCast() const
	{
		static_assert(std::is_base_of_v<CObject, U>, "U must inherit from CObject");
		return TSharedPtr<U>(dynamic_cast<U*>(Ptr));
	}

	// 比较操作符
	bool operator==(const TSharedPtr& Other) const
	{
		return Ptr == Other.Ptr;
	}
	bool operator!=(const TSharedPtr& Other) const
	{
		return Ptr != Other.Ptr;
	}
	bool operator==(const T* Other) const
	{
		return Ptr == Other;
	}
	bool operator!=(const T* Other) const
	{
		return Ptr != Other;
	}

private:
	T* Ptr;

	template <typename U>
	friend class TSharedPtr;
	friend class CObject;
};

// === 全局工厂函数 ===

/**
 * @brief 创建NObject的全局工厂函数
 * @tparam TType 对象类型（必须继承自NObject）
 * @tparam ArgsType 构造函数参数类型
 * @param Args 构造函数参数
 * @return 托管对象的智能指针
 */
template <typename TType, typename... ArgsType>
TSharedPtr<TType> NewNObject(ArgsType&&... Args)
{
	static_assert(std::is_base_of_v<CObject, TType>, "TType must inherit from CObject");

	// 使用NAllocator模板分配内存
	CAllocator<TType> Allocator;
	TType* Memory = Allocator.allocate(1);
	if (!Memory)
	{
		throw std::bad_alloc();
	}

	try
	{
		// 使用placement new在分配的内存上构造对象
		TType* Obj = new (Memory) TType(std::forward<ArgsType>(Args)...);
		return TSharedPtr<TType>(Obj);
	}
	catch (...)
	{
		// 构造失败时释放内存
		Allocator.deallocate(Memory, 1);
		throw;
	}
}

// === 类型别名 ===
using NObjectPtr = TSharedPtr<CObject>;

// === 辅助宏定义（类似UE的DECLARE_CLASS） ===

/**
 * @brief 在类中声明NObject相关的基础功能
 * 用于子类中声明类型信息等
 */
#define NCLASS()
class ClassName : public SuperClass
{
    GENERATED_BODY()                                                                   \
public:                                                                                                                \
	using Super = SuperClass;                                                                                          \
	virtual const std::type_info& GetTypeInfo() const override                                                         \
	{                                                                                                                  \
		return typeid(ClassName);                                                                                      \
	}                                                                                                                  \
	virtual const char* GetTypeName() const override                                                                   \
	{                                                                                                                  \
		return #ClassName;                                                                                             \
	}                                                                                                                  \
                                                                                                                       \
private:

/**
 * @brief 声明抽象NObject类
 */
#define DECLARE_NOBJECT_ABSTRACT_CLASS(ClassName, SuperClass)                                                          \
public:                                                                                                                \
	using Super = SuperClass;                                                                                          \
	virtual const std::type_info& GetTypeInfo() const override                                                         \
	{                                                                                                                  \
		return typeid(ClassName);                                                                                      \
	}                                                                                                                  \
	virtual const char* GetTypeName() const override                                                                   \
	{                                                                                                                  \
		return #ClassName;                                                                                             \
	}                                                                                                                  \
                                                                                                                       \
private:

// === NCLASS 宏系统（类似UE的UCLASS系统） ===

/**
 * @brief NCLASS 宏 - 标记类需要代码生成和反射支持
 * 使用方式：NCLASS(meta = (Category="Core", BlueprintType=true))
 * 类定义前使用此宏标记，NutHeaderTools会自动生成对应的 .generate.h 文件
 */
#define NCLASS(...)

/**
 * @brief NPROPERTY 宏 - 标记属性需要反射支持
 * 使用方式：NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
 */
#define NPROPERTY(...)

/**
 * @brief NFUNCTION 宏 - 标记函数需要反射支持
 * 使用方式：NFUNCTION(BlueprintCallable, Category="Utility")
 */
#define NFUNCTION(...)

/**
 * @brief GENERATED_BODY 宏 - 在类中插入生成的代码
 * 会被 NutHeaderTools 替换为实际的生成代码
 */
#define GENERATED_BODY()                                                                                               \
private:                                                                                                               \
	friend class CObjectReflection;                                                                                    \
                                                                                                                       \
public:                                                                                                                \
	using Super = CObject;                                                                                             \
	virtual const std::type_info& GetTypeInfo() const override                                                         \
	{                                                                                                                  \
		return typeid(*this);                                                                                          \
	}                                                                                                                  \
	virtual const char* GetTypeName() const override                                                                   \
	{                                                                                                                  \
		return GetStaticTypeName();                                                                                    \
	}                                                                                                                  \
	virtual const NClassReflection* GetClassReflection() const override;                                               \
	static const char* GetStaticTypeName();

/**
 * @brief 生成包含语句的辅助宏
 * 由于 #include 是预处理器指令，不能通过宏生成
 * 请直接使用: #include "ClassName.generate.h"
 *
 * 这个宏仅用于文档目的，提醒包含生成的头文件
 */
#define GENERATED_INCLUDE_REMINDER() /* 请在类定义后添加: #include "ClassName.generate.h" */

} // namespace NLib
