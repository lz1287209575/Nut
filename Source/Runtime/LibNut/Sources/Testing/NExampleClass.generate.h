// 此文件由 NutHeaderTools 自动生成 - 请勿手动修改
// 生成时间: 2025-07-29 13:52:56
// 源文件: NExampleClass.h

#pragma once

// 包含反射系统头文件
#include "NObjectReflection.h"

namespace Nut
{

// === NExampleClass 反射信息 ===

struct NExampleClassTypeInfo
{
    static constexpr const char* ClassName = "NExampleClass";
    static constexpr const char* BaseClassName = "NObject";
    static constexpr size_t PropertyCount = 3;
    static constexpr size_t FunctionCount = 3;
};

// NExampleClass 属性访问器
// Property: int32_t ExampleIntProperty
// Property: float ExampleFloatProperty
// Property: std::string ExampleStringProperty

// NExampleClass 函数调用器
// Function: void ExampleFunction()
// Function: int32_t GetSum()
// Function: float CalculateCircleArea()

// 注册 NExampleClass 到反射系统
REGISTER_NCLASS_REFLECTION(NExampleClass);

} // namespace Nut

// NutHeaderTools 生成结束
