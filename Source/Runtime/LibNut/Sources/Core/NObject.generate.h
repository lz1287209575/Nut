// 此文件由 NutHeaderTools 自动生成 - 请勿手动修改
// 生成时间: 2025-07-29 13:52:56
// 源文件: NObject.h

#pragma once

// 包含反射系统头文件
#include "NObjectReflection.h"

namespace Nut
{

// === NObjectReflection 反射信息 ===

struct NObjectReflectionTypeInfo
{
    static constexpr const char* ClassName = "NObjectReflection";
    static constexpr const char* BaseClassName = "NObject";
    static constexpr size_t PropertyCount = 0;
    static constexpr size_t FunctionCount = 0;
};

// 注册 NObjectReflection 到反射系统
REGISTER_NCLASS_REFLECTION(NObjectReflection);

} // namespace Nut

// NutHeaderTools 生成结束
