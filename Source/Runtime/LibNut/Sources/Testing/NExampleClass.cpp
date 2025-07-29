#include "NExampleClass.h"
#include "NLogger.h"
#include <cmath>

namespace Nut
{

NExampleClass::NExampleClass()
    : bInitialized(true)
{
    // 构造函数实现
}

const char* NExampleClass::GetStaticTypeName()
{
    return "NExampleClass";
}

const NClassReflection* NExampleClass::GetClassReflection() const
{
    return NObjectReflection::GetInstance().GetClassReflection("NExampleClass");
}

NExampleClass::~NExampleClass()
{
    // 析构函数实现
}

void NExampleClass::ExampleFunction()
{
    NLogger::Info("NExampleClass::ExampleFunction() called!");
    NLogger::Info("IntProperty: {}, FloatProperty: {:.2f}", 
                 ExampleIntProperty, ExampleFloatProperty);
    NLogger::Info("StringProperty: {}", ExampleStringProperty);
}

int32_t NExampleClass::GetSum(int32_t A, int32_t B)
{
    int32_t result = A + B;
    NLogger::Info("GetSum({}, {}) = {}", A, B, result);
    return result;
}

float NExampleClass::CalculateCircleArea(float Radius)
{
    float area = 3.14159f * Radius * Radius;
    NLogger::Info("CalculateCircleArea({:.2f}) = {:.2f}", Radius, area);
    return area;
}

} // namespace Nut