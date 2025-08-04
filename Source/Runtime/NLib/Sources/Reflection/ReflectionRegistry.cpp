#include "ReflectionRegistry.h"

#include "Core/Object.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <sstream>

namespace NLib
{
// === CReflectionRegistry 实现 ===

CReflectionRegistry& CReflectionRegistry::GetInstance()
{
	static CReflectionRegistry Instance;
	return Instance;
}

// === 类注册 ===

bool CReflectionRegistry::RegisterClass(const SClassReflection* ClassReflection)
{
	if (!ClassReflection || !ClassReflection->Name)
	{
		NLOG(LogReflection, Error, "Attempted to register null class reflection");
		return false;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::string ClassName = ClassReflection->Name;

	// 检查是否已经注册
	if (ClassRegistry.find(ClassName) != ClassRegistry.end())
	{
		NLOG(LogReflection, Warning, "Class '{}' is already registered", ClassName);
		return false;
	}

	// 注册到名称映射
	ClassRegistry[ClassName] = ClassReflection;

	// 注册到类型索引映射
	if (ClassReflection->TypeInfo)
	{
		TypeIndexRegistry[std::type_index(*ClassReflection->TypeInfo)] = ClassReflection;
	}

	// 清除统计缓存
	bStatsCacheValid = false;

	LogRegistration("Class", ClassName.c_str());
	NLOG(LogReflection, Info, "Registered class: {}", ClassName);

	return true;
}

bool CReflectionRegistry::UnregisterClass(const char* ClassName)
{
	if (!ClassName)
	{
		return false;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	auto It = ClassRegistry.find(ClassName);
	if (It != ClassRegistry.end())
	{
		// 从类型索引映射中移除
		if (It->second->TypeInfo)
		{
			TypeIndexRegistry.erase(std::type_index(*It->second->TypeInfo));
		}

		ClassRegistry.erase(It);
		bStatsCacheValid = false;

		NLOG(LogReflection, Info, "Unregistered class: {}", ClassName);
		return true;
	}

	return false;
}

const SClassReflection* CReflectionRegistry::FindClass(const char* ClassName) const
{
	if (!ClassName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	auto It = ClassRegistry.find(ClassName);
	if (It != ClassRegistry.end())
	{
		return It->second;
	}

	return nullptr;
}

const SClassReflection* CReflectionRegistry::FindClass(const std::type_info& TypeInfo) const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	auto It = TypeIndexRegistry.find(std::type_index(TypeInfo));
	if (It != TypeIndexRegistry.end())
	{
		return It->second;
	}

	return nullptr;
}

bool CReflectionRegistry::IsClassRegistered(const char* ClassName) const
{
	return FindClass(ClassName) != nullptr;
}

// === 枚举注册 ===

bool CReflectionRegistry::RegisterEnum(const SEnumReflection* EnumReflection)
{
	if (!EnumReflection || !EnumReflection->Name)
	{
		NLOG(LogReflection, Error, "Attempted to register null enum reflection");
		return false;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::string EnumName = EnumReflection->Name;

	if (EnumRegistry.find(EnumName) != EnumRegistry.end())
	{
		NLOG(LogReflection, Warning, "Enum '{}' is already registered", EnumName);
		return false;
	}

	EnumRegistry[EnumName] = EnumReflection;
	bStatsCacheValid = false;

	LogRegistration("Enum", EnumName.c_str());
	NLOG(LogReflection, Info, "Registered enum: {}", EnumName);

	return true;
}

const SEnumReflection* CReflectionRegistry::FindEnum(const char* EnumName) const
{
	if (!EnumName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	auto It = EnumRegistry.find(EnumName);
	if (It != EnumRegistry.end())
	{
		return It->second;
	}

	return nullptr;
}

// === 结构体注册 ===

bool CReflectionRegistry::RegisterStruct(const SStructReflection* StructReflection)
{
	if (!StructReflection || !StructReflection->Name)
	{
		NLOG(LogReflection, Error, "Attempted to register null struct reflection");
		return false;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::string StructName = StructReflection->Name;

	if (StructRegistry.find(StructName) != StructRegistry.end())
	{
		NLOG(LogReflection, Warning, "Struct '{}' is already registered", StructName);
		return false;
	}

	StructRegistry[StructName] = StructReflection;
	bStatsCacheValid = false;

	LogRegistration("Struct", StructName.c_str());
	NLOG(LogReflection, Info, "Registered struct: {}", StructName);

	return true;
}

const SStructReflection* CReflectionRegistry::FindStruct(const char* StructName) const
{
	if (!StructName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	auto It = StructRegistry.find(StructName);
	if (It != StructRegistry.end())
	{
		return It->second;
	}

	return nullptr;
}

// === 对象创建 ===

NObject* CReflectionRegistry::CreateObject(const char* ClassName) const
{
	const SClassReflection* ClassReflection = FindClass(ClassName);
	if (!ClassReflection)
	{
		NLOG(LogReflection, Error, "Cannot create object: class '{}' not found", ClassName ? ClassName : "null");
		return nullptr;
	}

	return CreateObject(ClassReflection);
}

NObject* CReflectionRegistry::CreateObject(const SClassReflection* ClassReflection) const
{
	if (!ClassReflection)
	{
		return nullptr;
	}

	if (ClassReflection->HasFlag(EClassFlags::Abstract))
	{
		NLOG(LogReflection, Error, "Cannot create instance of abstract class '{}'", ClassReflection->Name);
		return nullptr;
	}

	if (!ClassReflection->Constructor)
	{
		NLOG(LogReflection, Error, "No constructor available for class '{}'", ClassReflection->Name);
		return nullptr;
	}

	try
	{
		NObject* NewObject = ClassReflection->Constructor();
		if (NewObject)
		{
			NLOG(LogReflection, Debug, "Created object of type '{}'", ClassReflection->Name);
		}
		return NewObject;
	}
	catch (const std::exception& e)
	{
		NLOG(LogReflection, Error, "Failed to create object of type '{}': {}", ClassReflection->Name, e.what());
		return nullptr;
	}
}

// === 类型查询 ===

std::vector<std::string> CReflectionRegistry::GetAllClassNames() const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::vector<std::string> ClassNames;
	ClassNames.reserve(ClassRegistry.size());

	for (const auto& Pair : ClassRegistry)
	{
		ClassNames.push_back(Pair.first);
	}

	std::sort(ClassNames.begin(), ClassNames.end());
	return ClassNames;
}

std::vector<std::string> CReflectionRegistry::GetAllEnumNames() const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::vector<std::string> EnumNames;
	EnumNames.reserve(EnumRegistry.size());

	for (const auto& Pair : EnumRegistry)
	{
		EnumNames.push_back(Pair.first);
	}

	std::sort(EnumNames.begin(), EnumNames.end());
	return EnumNames;
}

std::vector<std::string> CReflectionRegistry::GetAllStructNames() const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	std::vector<std::string> StructNames;
	StructNames.reserve(StructRegistry.size());

	for (const auto& Pair : StructRegistry)
	{
		StructNames.push_back(Pair.first);
	}

	std::sort(StructNames.begin(), StructNames.end());
	return StructNames;
}

std::vector<const SClassReflection*> CReflectionRegistry::FindDerivedClasses(const char* BaseClassName) const
{
	std::vector<const SClassReflection*> DerivedClasses;

	if (!BaseClassName)
	{
		return DerivedClasses;
	}

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	for (const auto& Pair : ClassRegistry)
	{
		const SClassReflection* ClassReflection = Pair.second;
		if (ClassReflection->BaseClassName && strcmp(ClassReflection->BaseClassName, BaseClassName) == 0)
		{
			DerivedClasses.push_back(ClassReflection);
		}
	}

	return DerivedClasses;
}

std::vector<const SClassReflection*> CReflectionRegistry::FindClassesWithFlag(EClassFlags Flags) const
{
	std::vector<const SClassReflection*> MatchingClasses;

	std::lock_guard<std::mutex> Lock(RegistryMutex);

	for (const auto& Pair : ClassRegistry)
	{
		const SClassReflection* ClassReflection = Pair.second;
		if (ClassReflection->HasFlag(Flags))
		{
			MatchingClasses.push_back(ClassReflection);
		}
	}

	return MatchingClasses;
}

// === 类型检查 ===

bool CReflectionRegistry::IsChildOf(const char* ChildClassName, const char* ParentClassName) const
{
	if (!ChildClassName || !ParentClassName)
	{
		return false;
	}

	if (strcmp(ChildClassName, ParentClassName) == 0)
	{
		return true;
	}

	const SClassReflection* ChildClass = FindClass(ChildClassName);
	if (!ChildClass || !ChildClass->BaseClassName)
	{
		return false;
	}

	// 递归检查基类
	return IsChildOf(ChildClass->BaseClassName, ParentClassName);
}

bool CReflectionRegistry::IsA(const NObject* Object, const char* ClassName) const
{
	if (!Object || !ClassName)
	{
		return false;
	}

	const SClassReflection* ObjectClass = FindClass(Object->GetTypeInfo());
	if (!ObjectClass)
	{
		return false;
	}

	return IsChildOf(ObjectClass->Name, ClassName);
}

// === 统计和调试 ===

CReflectionRegistry::SRegistryStats CReflectionRegistry::GetStats() const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	if (!bStatsCacheValid)
	{
		CachedStats.ClassCount = ClassRegistry.size();
		CachedStats.EnumCount = EnumRegistry.size();
		CachedStats.StructCount = StructRegistry.size();

		CachedStats.TotalPropertyCount = 0;
		CachedStats.TotalFunctionCount = 0;

		for (const auto& Pair : ClassRegistry)
		{
			const SClassReflection* ClassReflection = Pair.second;
			CachedStats.TotalPropertyCount += ClassReflection->PropertyCount;
			CachedStats.TotalFunctionCount += ClassReflection->FunctionCount;
		}

		for (const auto& Pair : StructRegistry)
		{
			const SStructReflection* StructReflection = Pair.second;
			CachedStats.TotalPropertyCount += StructReflection->PropertyCount;
		}

		bStatsCacheValid = true;
	}

	return CachedStats;
}

void CReflectionRegistry::PrintRegistryInfo() const
{
	SRegistryStats Stats = GetStats();

	NLOG(LogReflection, Info, "=== Reflection Registry Info ===");
	NLOG(LogReflection, Info, "Classes:    {}", Stats.ClassCount);
	NLOG(LogReflection, Info, "Enums:      {}", Stats.EnumCount);
	NLOG(LogReflection, Info, "Structs:    {}", Stats.StructCount);
	NLOG(LogReflection, Info, "Properties: {}", Stats.TotalPropertyCount);
	NLOG(LogReflection, Info, "Functions:  {}", Stats.TotalFunctionCount);
	NLOG(LogReflection, Info, "===============================");

	// 打印所有类名
	auto ClassNames = GetAllClassNames();
	if (!ClassNames.empty())
	{
		NLOG(LogReflection, Info, "Registered Classes:");
		for (const auto& ClassName : ClassNames)
		{
			const SClassReflection* ClassReflection = FindClass(ClassName.c_str());
			if (ClassReflection)
			{
				NLOG(LogReflection,
				     Info,
				     "  {} (Properties: {}, Functions: {})",
				     ClassName,
				     ClassReflection->PropertyCount,
				     ClassReflection->FunctionCount);
			}
		}
	}
}

bool CReflectionRegistry::ValidateRegistry() const
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	bool bValid = true;

	// 验证类注册
	for (const auto& Pair : ClassRegistry)
	{
		const SClassReflection* ClassReflection = Pair.second;

		if (!ClassReflection->Name || strlen(ClassReflection->Name) == 0)
		{
			NLOG(LogReflection, Error, "Found class with invalid name");
			bValid = false;
		}

		if (!ClassReflection->TypeInfo)
		{
			NLOG(LogReflection, Warning, "Class '{}' has no type info", ClassReflection->Name);
		}

		// 验证基类存在
		if (ClassReflection->BaseClassName && strlen(ClassReflection->BaseClassName) > 0)
		{
			if (strcmp(ClassReflection->BaseClassName, "NObject") != 0 &&
			    ClassRegistry.find(ClassReflection->BaseClassName) == ClassRegistry.end())
			{
				NLOG(LogReflection,
				     Warning,
				     "Class '{}' has unregistered base class '{}'",
				     ClassReflection->Name,
				     ClassReflection->BaseClassName);
			}
		}
	}

	NLOG(LogReflection, Info, "Registry validation {}", bValid ? "passed" : "failed");
	return bValid;
}

void CReflectionRegistry::Clear()
{
	std::lock_guard<std::mutex> Lock(RegistryMutex);

	ClassRegistry.clear();
	TypeIndexRegistry.clear();
	EnumRegistry.clear();
	StructRegistry.clear();
	InheritanceCache.clear();

	bStatsCacheValid = false;

	NLOG(LogReflection, Info, "Reflection registry cleared");
}

// === 序列化支持 ===

std::string CReflectionRegistry::SerializeObject(const NObject* Object) const
{
	if (!Object)
	{
		return "{}";
	}

	const SClassReflection* ClassReflection = FindClass(Object->GetTypeInfo());
	if (!ClassReflection)
	{
		NLOG(LogReflection, Error, "Cannot serialize object: no reflection info found");
		return "{}";
	}

	std::ostringstream Json;
	Json << "{\n";
	Json << "  \"$type\": \"" << ClassReflection->Name << "\",\n";
	Json << "  \"$id\": " << Object->GetObjectId() << ",\n";

	// 序列化属性（简化实现）
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& Property = ClassReflection->Properties[i];
		Json << "  \"" << Property.Name << "\": \"<value>\",\n";
	}

	Json << "}";
	return Json.str();
}

NObject* CReflectionRegistry::DeserializeObject(const std::string& Data, const char* ClassName) const
{
	// 简化实现 - 实际需要JSON解析器
	NLOG(LogReflection, Warning, "Deserialization not fully implemented");
	return CreateObject(ClassName);
}

// === 内部方法 ===

void CReflectionRegistry::LogRegistration(const char* TypeName, const char* Name) const
{
	NLOG(LogReflection, Debug, "Registering {}: {}", TypeName, Name);
}

std::string CReflectionRegistry::GetFullTypeName(const SClassReflection* ClassReflection) const
{
	if (!ClassReflection)
	{
		return "Unknown";
	}

	std::string FullName = "NLib::";
	FullName += ClassReflection->Name;
	return FullName;
}

} // namespace NLib