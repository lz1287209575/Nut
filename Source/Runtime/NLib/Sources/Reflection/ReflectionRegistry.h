#pragma once

#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "ReflectionStructures.h"

#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace NLib
{
/**
 * @brief 反射注册表
 *
 * 管理所有反射信息的全局注册表，提供类型查找、创建实例等功能
 */
class CReflectionRegistry
{
public:
	/**
	 * @brief 获取反射注册表的全局实例
	 * @return 注册表实例引用
	 */
	static CReflectionRegistry& GetInstance();

	// 禁用拷贝和赋值
	CReflectionRegistry(const CReflectionRegistry&) = delete;
	CReflectionRegistry& operator=(const CReflectionRegistry&) = delete;

public:
	// === 类注册 ===

	/**
	 * @brief 注册类反射信息
	 * @param ClassReflection 类反射信息
	 * @return 是否注册成功
	 */
	bool RegisterClass(const SClassReflection* ClassReflection);

	/**
	 * @brief 注销类反射信息
	 * @param ClassName 类名
	 * @return 是否注销成功
	 */
	bool UnregisterClass(const char* ClassName);

	/**
	 * @brief 根据名称查找类反射信息
	 * @param ClassName 类名
	 * @return 类反射信息指针，如果未找到返回nullptr
	 */
	const SClassReflection* FindClass(const char* ClassName) const;

	/**
	 * @brief 根据类型信息查找类反射信息
	 * @param TypeInfo 类型信息
	 * @return 类反射信息指针，如果未找到返回nullptr
	 */
	const SClassReflection* FindClass(const std::type_info& TypeInfo) const;

	/**
	 * @brief 检查类是否已注册
	 * @param ClassName 类名
	 * @return true if registered, false otherwise
	 */
	bool IsClassRegistered(const char* ClassName) const;

public:
	// === 枚举注册 ===

	/**
	 * @brief 注册枚举反射信息
	 * @param EnumReflection 枚举反射信息
	 * @return 是否注册成功
	 */
	bool RegisterEnum(const SEnumReflection* EnumReflection);

	/**
	 * @brief 根据名称查找枚举反射信息
	 * @param EnumName 枚举名
	 * @return 枚举反射信息指针，如果未找到返回nullptr
	 */
	const SEnumReflection* FindEnum(const char* EnumName) const;

public:
	// === 结构体注册 ===

	/**
	 * @brief 注册结构体反射信息
	 * @param StructReflection 结构体反射信息
	 * @return 是否注册成功
	 */
	bool RegisterStruct(const SStructReflection* StructReflection);

	/**
	 * @brief 根据名称查找结构体反射信息
	 * @param StructName 结构体名
	 * @return 结构体反射信息指针，如果未找到返回nullptr
	 */
	const SStructReflection* FindStruct(const char* StructName) const;

public:
	// === 对象创建 ===

	/**
	 * @brief 根据类名创建对象实例
	 * @param ClassName 类名
	 * @return 创建的对象指针，失败返回nullptr
	 */
	NObject* CreateObject(const char* ClassName) const;

	/**
	 * @brief 根据类反射信息创建对象实例
	 * @param ClassReflection 类反射信息
	 * @return 创建的对象指针，失败返回nullptr
	 */
	NObject* CreateObject(const SClassReflection* ClassReflection) const;

	/**
	 * @brief 创建对象的智能指针
	 * @param ClassName 类名
	 * @return 创建的对象智能指针
	 */
	template <typename TType = NObject>
	TSharedPtr<TType> CreateObjectPtr(const char* ClassName) const
	{
		NObject* Object = CreateObject(ClassName);
		if (Object)
		{
			return TSharedPtr<TType>(static_cast<TType*>(Object));
		}
		return TSharedPtr<TType>();
	}

public:
	// === 类型查询 ===

	/**
	 * @brief 获取所有已注册的类名
	 * @return 类名列表
	 */
	std::vector<std::string> GetAllClassNames() const;

	/**
	 * @brief 获取所有已注册的枚举名
	 * @return 枚举名列表
	 */
	std::vector<std::string> GetAllEnumNames() const;

	/**
	 * @brief 获取所有已注册的结构体名
	 * @return 结构体名列表
	 */
	std::vector<std::string> GetAllStructNames() const;

	/**
	 * @brief 根据基类查找所有子类
	 * @param BaseClassName 基类名
	 * @return 子类反射信息列表
	 */
	std::vector<const SClassReflection*> FindDerivedClasses(const char* BaseClassName) const;

	/**
	 * @brief 根据标志查找所有类
	 * @param Flags 类标志
	 * @return 匹配的类反射信息列表
	 */
	std::vector<const SClassReflection*> FindClassesWithFlag(EClassFlags Flags) const;

public:
	// === 属性和函数查询 ===

	/**
	 * @brief 在所有类中搜索指定名称的属性
	 * @param PropertyName 属性名
	 * @return 找到的属性信息列表
	 */
	std::vector<std::pair<const SClassReflection*, const SPropertyReflection*>> FindPropertiesNamed(
	    const char* PropertyName) const;

	/**
	 * @brief 在所有类中搜索指定名称的函数
	 * @param FunctionName 函数名
	 * @return 找到的函数信息列表
	 */
	std::vector<std::pair<const SClassReflection*, const SFunctionReflection*>> FindFunctionsNamed(
	    const char* FunctionName) const;

	/**
	 * @brief 根据标志搜索所有属性
	 * @param Flags 属性标志
	 * @return 找到的属性信息列表
	 */
	std::vector<std::pair<const SClassReflection*, const SPropertyReflection*>> FindPropertiesWithFlag(
	    EPropertyFlags Flags) const;

	/**
	 * @brief 根据标志搜索所有函数
	 * @param Flags 函数标志
	 * @return 找到的函数信息列表
	 */
	std::vector<std::pair<const SClassReflection*, const SFunctionReflection*>> FindFunctionsWithFlag(
	    EFunctionFlags Flags) const;

public:
	// === 类型检查 ===

	/**
	 * @brief 检查类是否为另一个类的子类
	 * @param ChildClassName 子类名
	 * @param ParentClassName 父类名
	 * @return true if is child of, false otherwise
	 */
	bool IsChildOf(const char* ChildClassName, const char* ParentClassName) const;

	/**
	 * @brief 检查对象是否为指定类型
	 * @param Object 要检查的对象
	 * @param ClassName 类名
	 * @return true if object is of type, false otherwise
	 */
	bool IsA(const NObject* Object, const char* ClassName) const;

	/**
	 * @brief 安全的类型转换
	 * @param Object 要转换的对象
	 * @param ClassName 目标类名
	 * @return 转换后的对象指针，失败返回nullptr
	 */
	template <typename TType>
	TType* Cast(NObject* Object, const char* ClassName) const
	{
		if (IsA(Object, ClassName))
		{
			return static_cast<TType*>(Object);
		}
		return nullptr;
	}

public:
	// === 统计和调试 ===

	/**
	 * @brief 获取注册统计信息
	 */
	struct SRegistryStats
	{
		size_t ClassCount = 0;
		size_t EnumCount = 0;
		size_t StructCount = 0;
		size_t TotalPropertyCount = 0;
		size_t TotalFunctionCount = 0;
	};

	SRegistryStats GetStats() const;

	/**
	 * @brief 打印所有注册信息
	 */
	void PrintRegistryInfo() const;

	/**
	 * @brief 验证反射系统的完整性
	 * @return 验证结果
	 */
	bool ValidateRegistry() const;

	/**
	 * @brief 清除所有注册信息（仅用于测试）
	 */
	void Clear();

public:
	// === 序列化支持 ===

	/**
	 * @brief 将对象序列化为字符串
	 * @param Object 要序列化的对象
	 * @return 序列化结果
	 */
	std::string SerializeObject(const NObject* Object) const;

	/**
	 * @brief 从字符串反序列化对象
	 * @param Data 序列化数据
	 * @param ClassName 目标类名
	 * @return 反序列化的对象
	 */
	NObject* DeserializeObject(const std::string& Data, const char* ClassName) const;

private:
	// === 构造函数 ===
	CReflectionRegistry() = default;
	~CReflectionRegistry() = default;

	// === 内部方法 ===
	void LogRegistration(const char* TypeName, const char* Name) const;
	std::string GetFullTypeName(const SClassReflection* ClassReflection) const;

private:
	// === 成员变量 ===
	mutable std::mutex RegistryMutex; // 线程安全锁

	// 注册表
	std::unordered_map<std::string, const SClassReflection*> ClassRegistry;
	std::unordered_map<std::type_index, const SClassReflection*> TypeIndexRegistry;
	std::unordered_map<std::string, const SEnumReflection*> EnumRegistry;
	std::unordered_map<std::string, const SStructReflection*> StructRegistry;

	// 继承关系缓存
	mutable std::unordered_map<std::string, std::unordered_set<std::string>> InheritanceCache;

	// 统计信息
	mutable SRegistryStats CachedStats;
	mutable bool bStatsCacheValid = false;
};

// === 全局便捷函数 ===

/**
 * @brief 获取反射注册表实例
 */
inline CReflectionRegistry& GetReflectionRegistry()
{
	return CReflectionRegistry::GetInstance();
}

/**
 * @brief 根据类名创建对象
 */
template <typename TType = NObject>
TSharedPtr<TType> CreateObjectByName(const char* ClassName)
{
	return GetReflectionRegistry().CreateObjectPtr<TType>(ClassName);
}

/**
 * @brief 查找类反射信息
 */
inline const SClassReflection* FindClassReflection(const char* ClassName)
{
	return GetReflectionRegistry().FindClass(ClassName);
}

/**
 * @brief 查找类反射信息
 */
template <typename TType>
const SClassReflection* FindClassReflection()
{
	return GetReflectionRegistry().FindClass(typeid(TType));
}

/**
 * @brief 检查类型关系
 */
inline bool IsChildOfClass(const char* ChildClassName, const char* ParentClassName)
{
	return GetReflectionRegistry().IsChildOf(ChildClassName, ParentClassName);
}

/**
 * @brief 运行时类型检查
 */
inline bool IsObjectOfType(const NObject* Object, const char* ClassName)
{
	return GetReflectionRegistry().IsA(Object, ClassName);
}

} // namespace NLib