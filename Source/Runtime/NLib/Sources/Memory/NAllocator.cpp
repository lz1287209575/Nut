#include "CAllocator.h"

#include "CMemoryManager.h"

namespace NLib
{

// 这个文件主要用于确保NMemoryManager的包含不会产生循环依赖
// 模板的实现在头文件中已经定义

// 显式实例化常用类型
template class CAllocator<int>;
template class CAllocator<char>;
template class CAllocator<void*>;

} // namespace NLib
