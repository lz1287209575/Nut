#include "CArray.h"
#include "CArray.generate.h"

#include "Containers/CString.h"

namespace NLib
{

// 注意：CArray 是模板类，不支持直接反射注册
// 模板类的反射需要在具体实例化时处理

// ToString方法的显式模板实例化，避免循环依赖
template <>
CString CArray<int32_t>::ToString() const
{
	CString Result = "CArray<int32_t>[";
	for (size_t i = 0; i < Size; ++i)
	{
		if (i > 0)
		{
			Result += ", ";
		}
		Result += CString::FromInt32(Data[i]);
	}
	Result += "]";
	return Result;
}

template <>
CString CArray<int64_t>::ToString() const
{
	CString Result = "CArray<int64_t>[";
	for (size_t i = 0; i < Size; ++i)
	{
		if (i > 0)
		{
			Result += ", ";
		}
		Result += CString::FromInt64(Data[i]);
	}
	Result += "]";
	return Result;
}

template <>
CString CArray<float>::ToString() const
{
	CString Result = "CArray<float>[";
	for (size_t i = 0; i < Size; ++i)
	{
		if (i > 0)
		{
			Result += ", ";
		}
		Result += CString::FromFloat(Data[i]);
	}
	Result += "]";
	return Result;
}

template <>
CString CArray<double>::ToString() const
{
	CString Result = "CArray<double>[";
	for (size_t i = 0; i < Size; ++i)
	{
		if (i > 0)
		{
			Result += ", ";
		}
		Result += CString::FromDouble(Data[i]);
	}
	Result += "]";
	return Result;
}

template <>
CString CArray<CString>::ToString() const
{
	CString Result = "CArray<CString>[";
	for (size_t i = 0; i < Size; ++i)
	{
		if (i > 0)
		{
			Result += ", ";
		}
		Result += CString("\"") + Data[i] + "\"";
	}
	Result += "]";
	return Result;
}

// 常用类型的显式实例化
template class CArray<int32_t>;
template class CArray<int64_t>;
template class CArray<float>;
template class CArray<double>;
template class CArray<CString>;

} // namespace NLib
