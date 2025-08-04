#pragma once

#include "Macros/ReflectionMacros.h"

#include <any>
#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace NLib
{
// 前向声明
class NObject;
class CReflectionRegistry;

/**
 * @brief 反射属性信息
 */
struct SPropertyReflection
{
	const char* Name;               // 属性名称
	const char* TypeName;           // 类型名称
	size_t Offset;                  // 在类中的偏移量
	size_t Size;                    // 属性大小
	const std::type_info* TypeInfo; // 类型信息
	EPropertyFlags Flags;           // 属性标志
	const char* Category;           // 分类
	const char* DisplayName;        // 显示名称
	const char* ToolTip;            // 工具提示
	std::any DefaultValue;          // 默认值

	// 属性访问器函数
	std::function<std::any(const NObject*)> Getter;        // 获取属性值
	std::function<void(NObject*, const std::any&)> Setter; // 设置属性值

	/**
	 * @brief 检查属性是否有指定标志
	 */
	bool HasFlag(EPropertyFlags Flag) const
	{
		return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Flag)) != 0;
	}

	/**
	 * @brief 获取属性值
	 */
	template <typename TType>
	TType GetValue(const NObject* Object) const
	{
		if (Getter)
		{
			std::any Value = Getter(Object);
			return std::any_cast<TType>(Value);
		}

		// 直接内存访问
		const char* ObjectPtr = reinterpret_cast<const char*>(Object);
		return *reinterpret_cast<const TType*>(ObjectPtr + Offset);
	}

	/**
	 * @brief 设置属性值
	 */
	template <typename TType>
	void SetValue(NObject* Object, const TType& Value) const
	{
		if (Setter)
		{
			Setter(Object, std::make_any<TType>(Value));
			return;
		}

		// 直接内存访问
		char* ObjectPtr = reinterpret_cast<char*>(Object);
		*reinterpret_cast<TType*>(ObjectPtr + Offset) = Value;
	}
};

/**
 * @brief 反射函数参数信息
 */
struct SParameterReflection
{
	const char* Name;               // 参数名称
	const char* TypeName;           // 类型名称
	const std::type_info* TypeInfo; // 类型信息
	bool bIsConst;                  // 是否为常量
	bool bIsReference;              // 是否为引用
	bool bIsPointer;                // 是否为指针
	std::any DefaultValue;          // 默认值
};

/**
 * @brief 反射函数信息
 */
struct SFunctionReflection
{
	const char* Name;                     // 函数名称
	const char* ReturnTypeName;           // 返回类型名称
	const std::type_info* ReturnTypeInfo; // 返回类型信息
	EFunctionFlags Flags;                 // 函数标志
	const char* Category;                 // 分类
	const char* DisplayName;              // 显示名称
	const char* ToolTip;                  // 工具提示

	// 参数信息
	std::vector<SParameterReflection> Parameters;

	// 函数调用器
	std::function<std::any(NObject*, const std::vector<std::any>&)> Invoker;

	/**
	 * @brief 检查函数是否有指定标志
	 */
	bool HasFlag(EFunctionFlags Flag) const
	{
		return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Flag)) != 0;
	}

	/**
	 * @brief 调用函数
	 */
	template <typename TReturnType, typename... TArgs>
	TReturnType Invoke(NObject* Object, TArgs&&... Args) const
	{
		std::vector<std::any> Arguments;
		(Arguments.emplace_back(std::forward<TArgs>(Args)), ...);

		std::any Result = Invoker(Object, Arguments);

		if constexpr (std::is_void_v<TReturnType>)
		{
			return;
		}
		else
		{
			return std::any_cast<TReturnType>(Result);
		}
	}

	/**
	 * @brief 获取参数数量
	 */
	size_t GetParameterCount() const
	{
		return Parameters.size();
	}

	/**
	 * @brief 根据名称查找参数
	 */
	const SParameterReflection* FindParameter(const char* ParameterName) const
	{
		for (const auto& Param : Parameters)
		{
			if (strcmp(Param.Name, ParameterName) == 0)
			{
				return &Param;
			}
		}
		return nullptr;
	}
};

/**
 * @brief 反射类信息
 */
struct SClassReflection
{
	const char* Name;               // 类名
	const char* BaseClassName;      // 基类名
	size_t Size;                    // 类大小
	const std::type_info* TypeInfo; // 类型信息
	EClassFlags Flags;              // 类标志
	const char* Category;           // 分类
	const char* DisplayName;        // 显示名称
	const char* ToolTip;            // 工具提示

	// 属性和函数
	const SPropertyReflection* Properties; // 属性数组
	size_t PropertyCount;                  // 属性数量
	const SFunctionReflection* Functions;  // 函数数组
	size_t FunctionCount;                  // 函数数量

	// 类构造器
	std::function<NObject*()> Constructor; // 默认构造函数

	/**
	 * @brief 检查类是否有指定标志
	 */
	bool HasFlag(EClassFlags Flag) const
	{
		return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Flag)) != 0;
	}

	/**
	 * @brief 根据名称查找属性
	 */
	const SPropertyReflection* FindProperty(const char* PropertyName) const
	{
		for (size_t i = 0; i < PropertyCount; ++i)
		{
			if (strcmp(Properties[i].Name, PropertyName) == 0)
			{
				return &Properties[i];
			}
		}
		return nullptr;
	}

	/**
	 * @brief 根据名称查找函数
	 */
	const SFunctionReflection* FindFunction(const char* FunctionName) const
	{
		for (size_t i = 0; i < FunctionCount; ++i)
		{
			if (strcmp(Functions[i].Name, FunctionName) == 0)
			{
				return &Functions[i];
			}
		}
		return nullptr;
	}

	/**
	 * @brief 获取所有属性
	 */
	std::vector<const SPropertyReflection*> GetProperties() const
	{
		std::vector<const SPropertyReflection*> Result;
		Result.reserve(PropertyCount);
		for (size_t i = 0; i < PropertyCount; ++i)
		{
			Result.push_back(&Properties[i]);
		}
		return Result;
	}

	/**
	 * @brief 获取所有函数
	 */
	std::vector<const SFunctionReflection*> GetFunctions() const
	{
		std::vector<const SFunctionReflection*> Result;
		Result.reserve(FunctionCount);
		for (size_t i = 0; i < FunctionCount; ++i)
		{
			Result.push_back(&Functions[i]);
		}
		return Result;
	}

	/**
	 * @brief 根据标志筛选属性
	 */
	std::vector<const SPropertyReflection*> GetPropertiesWithFlag(EPropertyFlags Flag) const
	{
		std::vector<const SPropertyReflection*> Result;
		for (size_t i = 0; i < PropertyCount; ++i)
		{
			if (Properties[i].HasFlag(Flag))
			{
				Result.push_back(&Properties[i]);
			}
		}
		return Result;
	}

	/**
	 * @brief 根据标志筛选函数
	 */
	std::vector<const SFunctionReflection*> GetFunctionsWithFlag(EFunctionFlags Flag) const
	{
		std::vector<const SFunctionReflection*> Result;
		for (size_t i = 0; i < FunctionCount; ++i)
		{
			if (Functions[i].HasFlag(Flag))
			{
				Result.push_back(&Functions[i]);
			}
		}
		return Result;
	}

	/**
	 * @brief 创建类的默认实例
	 */
	NObject* CreateDefaultObject() const
	{
		if (Constructor)
		{
			return Constructor();
		}
		return nullptr;
	}

	/**
	 * @brief 检查是否为指定类的子类
	 */
	bool IsChildOf(const SClassReflection* ParentClass) const
	{
		if (!ParentClass)
			return false;

		if (this == ParentClass)
			return true;

		// 递归检查基类
		// 注意：这里需要通过反射注册表查找基类信息
		return false; // 简化实现
	}
};

/**
 * @brief 反射枚举值信息
 */
struct SEnumValueReflection
{
	const char* Name;        // 枚举值名称
	int64_t Value;           // 枚举值
	const char* DisplayName; // 显示名称
	const char* ToolTip;     // 工具提示
};

/**
 * @brief 反射枚举信息
 */
struct SEnumReflection
{
	const char* Name;               // 枚举名称
	const std::type_info* TypeInfo; // 类型信息
	const char* Category;           // 分类
	const char* DisplayName;        // 显示名称
	const char* ToolTip;            // 工具提示

	// 枚举值
	const SEnumValueReflection* Values; // 枚举值数组
	size_t ValueCount;                  // 枚举值数量

	/**
	 * @brief 根据名称查找枚举值
	 */
	const SEnumValueReflection* FindValue(const char* ValueName) const
	{
		for (size_t i = 0; i < ValueCount; ++i)
		{
			if (strcmp(Values[i].Name, ValueName) == 0)
			{
				return &Values[i];
			}
		}
		return nullptr;
	}

	/**
	 * @brief 根据数值查找枚举值
	 */
	const SEnumValueReflection* FindValueByNumber(int64_t Value) const
	{
		for (size_t i = 0; i < ValueCount; ++i)
		{
			if (Values[i].Value == Value)
			{
				return &Values[i];
			}
		}
		return nullptr;
	}

	/**
	 * @brief 获取所有枚举值
	 */
	std::vector<const SEnumValueReflection*> GetValues() const
	{
		std::vector<const SEnumValueReflection*> Result;
		Result.reserve(ValueCount);
		for (size_t i = 0; i < ValueCount; ++i)
		{
			Result.push_back(&Values[i]);
		}
		return Result;
	}
};

/**
 * @brief 反射结构体信息
 */
struct SStructReflection
{
	const char* Name;               // 结构体名称
	size_t Size;                    // 结构体大小
	const std::type_info* TypeInfo; // 类型信息
	const char* Category;           // 分类
	const char* DisplayName;        // 显示名称
	const char* ToolTip;            // 工具提示

	// 属性
	const SPropertyReflection* Properties; // 属性数组
	size_t PropertyCount;                  // 属性数量

	/**
	 * @brief 根据名称查找属性
	 */
	const SPropertyReflection* FindProperty(const char* PropertyName) const
	{
		for (size_t i = 0; i < PropertyCount; ++i)
		{
			if (strcmp(Properties[i].Name, PropertyName) == 0)
			{
				return &Properties[i];
			}
		}
		return nullptr;
	}

	/**
	 * @brief 获取所有属性
	 */
	std::vector<const SPropertyReflection*> GetProperties() const
	{
		std::vector<const SPropertyReflection*> Result;
		Result.reserve(PropertyCount);
		for (size_t i = 0; i < PropertyCount; ++i)
		{
			Result.push_back(&Properties[i]);
		}
		return Result;
	}
};

} // namespace NLib