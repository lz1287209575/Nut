#include "CObjectReflection.h"

#include "Core/CObject.h"

#include <algorithm>

namespace NLib
{

CObjectReflection& CObjectReflection::GetInstance()
{
	static CObjectReflection instance;
	return instance;
}

void CObjectReflection::RegisterClass(const std::string& ClassName, const NClassReflection& Reflection)
{
	ClassReflections[ClassName] = Reflection;
	TypeInfoToClassName[Reflection.TypeInfo] = ClassName;
}

const NClassReflection* CObjectReflection::GetClassReflection(const std::string& ClassName) const
{
	auto It = ClassReflections.find(ClassName);
	return (It != ClassReflections.end()) ? &It->second : nullptr;
}

const NClassReflection* CObjectReflection::GetClassReflection(const std::type_info& TypeInfo) const
{
	auto It = TypeInfoToClassName.find(&TypeInfo);
	if (It != TypeInfoToClassName.end())
	{
		return GetClassReflection(It->second);
	}
	return nullptr;
}

std::vector<std::string> CObjectReflection::GetAllClassNames() const
{
	std::vector<std::string> ClassNames;
	ClassNames.reserve(ClassReflections.size());

	for (const auto& Pair : ClassReflections)
	{
		ClassNames.push_back(Pair.first);
	}

	std::sort(ClassNames.begin(), ClassNames.end());
	return ClassNames;
}

CObject* CObjectReflection::CreateInstance(const std::string& ClassName) const
{
	const NClassReflection* Reflection = GetClassReflection(ClassName);
	if (Reflection && Reflection->Factory)
	{
		return Reflection->Factory();
	}
	return nullptr;
}

bool CObjectReflection::IsChildOf(const std::string& ClassName, const std::string& BaseClassName) const
{
	if (ClassName == BaseClassName)
	{
		return true;
	}

	const NClassReflection* Reflection = GetClassReflection(ClassName);
	if (!Reflection || Reflection->BaseClassName.empty())
	{
		return false;
	}

	// 递归检查基类
	return IsChildOf(Reflection->BaseClassName, BaseClassName);
}

} // namespace NLib