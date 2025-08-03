#include "CContainer.h"
#include "CContainer.generate.h"

#include "Reflection/CObjectReflection.h"

namespace NLib
{

// 注册 CContainer 到反射系统
// 注意：CContainer 是抽象类，不能直接实例化
REGISTER_NCLASS_REFLECTION(CContainer);

} // namespace NLib