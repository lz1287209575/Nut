#include "NObjectReflection.h"
#include "Core/NObject.h"
#include <algorithm>

namespace Nut
{

NObjectReflection& NObjectReflection::GetInstance()
{
    static NObjectReflection instance;
    return instance;
}

void NObjectReflection::RegisterClass(const std::string& className, const NClassReflection& reflection)
{
    ClassReflections[className] = reflection;
    TypeInfoToClassName[reflection.TypeInfo] = className;
}

const NClassReflection* NObjectReflection::GetClassReflection(const std::string& className) const
{
    auto it = ClassReflections.find(className);
    return (it != ClassReflections.end()) ? &it->second : nullptr;
}

const NClassReflection* NObjectReflection::GetClassReflection(const std::type_info& typeInfo) const
{
    auto it = TypeInfoToClassName.find(&typeInfo);
    if (it != TypeInfoToClassName.end())
    {
        return GetClassReflection(it->second);
    }
    return nullptr;
}

std::vector<std::string> NObjectReflection::GetAllClassNames() const
{
    std::vector<std::string> classNames;
    classNames.reserve(ClassReflections.size());
    
    for (const auto& pair : ClassReflections)
    {
        classNames.push_back(pair.first);
    }
    
    std::sort(classNames.begin(), classNames.end());
    return classNames;
}

NObject* NObjectReflection::CreateInstance(const std::string& className) const
{
    const NClassReflection* reflection = GetClassReflection(className);
    if (reflection && reflection->Factory)
    {
        return reflection->Factory();
    }
    return nullptr;
}

bool NObjectReflection::IsChildOf(const std::string& className, const std::string& baseClassName) const
{
    if (className == baseClassName)
    {
        return true;
    }
    
    const NClassReflection* reflection = GetClassReflection(className);
    if (!reflection || reflection->BaseClassName.empty())
    {
        return false;
    }
    
    // 递归检查基类
    return IsChildOf(reflection->BaseClassName, baseClassName);
}

} // namespace Nut