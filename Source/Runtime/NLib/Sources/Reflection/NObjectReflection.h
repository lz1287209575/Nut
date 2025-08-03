#pragma once

#include "Memory/CAllocator.h"

#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace NLib
{

// 前向声明
class CObject;

/**
 * @brief 属性反射信息
 */
struct NPropertyReflection
{
	std::string Name;
	std::string Type;
	size_t Offset;
	std::function<void*(CObject*)> Getter;
	std::function<void(CObject*, const void*)> Setter;
};

/**
 * @brief 函数反射信息
 */
struct NFunctionReflection
{
	std::string Name;
	std::string ReturnType;
	std::vector<std::string> ParameterTypes;
	std::function<void(CObject*, void*, void**)> Invoker;
};

/**
 * @brief 类反射信息
 */
struct NClassReflection
{
	std::string ClassName;
	std::string BaseClassName;
	const std::type_info* TypeInfo;
	std::function<CObject*()> Factory;
	std::function<void*(size_t)> Allocator; // 专用分配器，用于NewNObject
	std::vector<NPropertyReflection> Properties;
	std::vector<NFunctionReflection> Functions;
};

/**
 * @brief 反射系统管理器
 * 提供运行时类型信息
 */
class CObjectReflection
{
public:
	/**
	 * @brief 获取反射系统单例
	 */
	static CObjectReflection& GetInstance();

	/**
	 * @brief 注册类反射信息
	 */
	void RegisterClass(const std::string& className, const NClassReflection& reflection);

	/**
	 * @brief 根据类名获取反射信息
	 */
	const NClassReflection* GetClassReflection(const std::string& className) const;

	/**
	 * @brief 根据类型信息获取反射信息
	 */
	const NClassReflection* GetClassReflection(const std::type_info& typeInfo) const;

	/**
	 * @brief 获取所有注册的类
	 */
	std::vector<std::string> GetAllClassNames() const;

	/**
	 * @brief 通过类名创建对象实例
	 */
	CObject* CreateInstance(const std::string& className) const;

	/**
	 * @brief 检查类是否继承自指定基类
	 */
	bool IsChildOf(const std::string& className, const std::string& baseClassName) const;

private:
	std::unordered_map<std::string, NClassReflection> ClassReflections;
	std::unordered_map<const std::type_info*, std::string> TypeInfoToClassName;
};

/**
 * @brief 自动注册类反射信息的辅助类
 */
template <typename TType>
class CClassRegistrar
{
public:
	CClassRegistrar(const std::string& className, const std::string& baseClassName)
	{
		NClassReflection reflection;
		reflection.ClassName = className;
		reflection.BaseClassName = baseClassName;
		reflection.TypeInfo = &typeid(T);

		// 设置专用分配器
		reflection.Allocator = [](size_t Size) -> void* {
			CAllocator<T> Allocator;
			return Allocator.allocate(Size / sizeof(T));
		};

		// 检查是否为抽象类，如果是则提供空的工厂函数
		if constexpr (std::is_abstract_v<T>)
		{
			reflection.Factory = []() -> CObject* { return nullptr; };
		}
		else
		{
			reflection.Factory = []() -> CObject* {
				// 使用NAllocator分配内存并构造对象
				CAllocator<T> Allocator;
				T* Memory = Allocator.allocate(1);
				if (!Memory)
				{
					return nullptr;
				}
				try
				{
					return new (Memory) T();
				}
				catch (...)
				{
					Allocator.deallocate(Memory, 1);
					return nullptr;
				}
			};
		}

		CObjectReflection::GetInstance().RegisterClass(className, reflection);
	}
};

/**
 * @brief 宏：注册NCLASS到反射系统
 */
#define REGISTER_NCLASS_REFLECTION(ClassName)                                                                          \
	static CClassRegistrar<ClassName> ClassName##_Registrar(#ClassName, "CObject");

/**
 * @brief 宏：在NCLASS中生成静态类型信息
 */
#define NCLASS_REFLECTION_BODY(ClassName)                                                                              \
public:                                                                                                                \
	static const char* GetStaticTypeName()                                                                             \
	{                                                                                                                  \
		return #ClassName;                                                                                             \
	}                                                                                                                  \
	virtual const NClassReflection* GetClassReflection() const override                                                \
	{                                                                                                                  \
		return CObjectReflection::GetInstance().GetClassReflection(#ClassName);                                        \
	}                                                                                                                  \
                                                                                                                       \
private:

} // namespace NLib
