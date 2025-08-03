#include "CHashMap.h"
#include "CHashMap.generate.h"

#include "Containers/CString.h"

namespace NLib
{

// 注意：CHashMap 是模板类，不支持直接反射注册
// 模板类的反射需要在具体实例化时处理

// ToString方法的显式模板实例化，避免循环依赖
template <>
CString CHashMap<int32_t, int32_t>::ToString() const
{
	CString result = "CHashMap<int32_t, int32_t>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromInt32(it->Key) + ": " + CString::FromInt32(it->Value);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CHashMap<CString, int32_t>::ToString() const
{
	CString result = "CHashMap<CString, int32_t>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString("\"") + it->Key + "\": " + CString::FromInt32(it->Value);
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CHashMap<int32_t, CString>::ToString() const
{
	CString result = "CHashMap<int32_t, CString>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString::FromInt32(it->Key) + ": \"" + it->Value + "\"";
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CHashMap<CString, CString>::ToString() const
{
	CString result = "CHashMap<CString, CString>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
			result += CString("\"") + it->Key + "\": \"" + it->Value + "\"";
		}
		first = false;
	}
	result += "}";
	return result;
}

template <>
CString CHashMap<CString, float>::ToString() const
{
	CString result = "CHashMap<CString, float>{";
	bool first = true;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!first)
		{
			result += ", ";
		}
		result += CString("\"") + it->Key + "\": " + CString::FromFloat(it->Value);
		first = false;
	}
	result += "}";
	return result;
}

// 常用类型的显式实例化
template class CHashMap<int32_t, int32_t>;
template class CHashMap<int64_t, int64_t>;
template class CHashMap<CString, int32_t>;
template class CHashMap<int32_t, CString>;
template class CHashMap<CString, CString>;
template class CHashMap<CString, float>;
template class CHashMap<CString, double>;

} // namespace NLib
