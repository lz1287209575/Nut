#include "CSet.h"
#include "CSet.generate.h"

#include "Containers/CArray.h" // 需要完整定义用于ToArray()方法
#include "Containers/CString.h"
#include "Memory/CGarbageCollector.h" // 需要用于GC注册

namespace NLib
{

// 注意：CSet 是模板类，不支持直接反射注册
// 模板类的反射需要在具体实例化时处理

// ToString方法的显式模板实例化，避免循环依赖
template <>
CString CSet<int32_t>::ToString() const
{
	CString result = "CSet<int32_t>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromInt32(*it);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CSet<int64_t>::ToString() const
{
	CString result = "CSet<int64_t>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromInt64(*it);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CSet<float>::ToString() const
{
	CString result = "CSet<float>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromFloat(*it);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CSet<double>::ToString() const
{
	CString result = "CSet<double>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromDouble(*it);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CSet<CString>::ToString() const
{
	CString result = "CSet<CString>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString("\"") + *it + "\"";
		first = false;
	}
	result += "}";
	return result;
}

// 常用类型的显式实例化
template class CSet<int32_t>;
template class CSet<int64_t>;
template class CSet<float>;
template class CSet<double>;
template class CSet<CString>;

} // namespace NLib
