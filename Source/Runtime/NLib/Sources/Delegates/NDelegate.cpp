#include "CDelegate.h"
#include "CDelegate.generate.h"

#include "Logging/CLogger.h"

namespace NLib
{

// NDelegateBase 实现
const char* NDelegateBase::GetStaticTypeName()
{
	return "NDelegateBase";
}

// 常用委托类型的显式实例化
template class CDelegate<void>;
template class CDelegate<void, int32_t>;
template class CDelegate<void, float>;
template class CDelegate<void, const CString&>;
template class CDelegate<void, bool>;

template class CDelegate<bool>;
template class CDelegate<bool, int32_t>;
template class CDelegate<int32_t>;
template class CDelegate<float>;
template class CDelegate<CString>;

template class CMulticastDelegate<>;
template class CMulticastDelegate<int32_t>;
template class CMulticastDelegate<float>;
template class CMulticastDelegate<const CString&>;
template class CMulticastDelegate<bool>;

} // namespace NLib