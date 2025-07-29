#pragma once

#include "NObject.h"

namespace Nut
{

/**
 * @brief 示例类 - 展示NCLASS宏系统的使用
 * 这个类展示了如何使用新的NCLASS、NPROPERTY和NFUNCTION宏
 */
NCLASS(meta = (Category="Examples", BlueprintType=true))
class NExampleClass : public NObject
{
    GENERATED_BODY()

public:
    NExampleClass();
    virtual ~NExampleClass();

    // 使用NPROPERTY标记的属性
    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basic")
    int32_t ExampleIntProperty = 42;

    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basic")
    float ExampleFloatProperty = 3.14f;

    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basic")
    std::string ExampleStringProperty = "Hello Nut Engine";

    // 使用NFUNCTION标记的函数
    NFUNCTION(BlueprintCallable, Category="Utility")
    void ExampleFunction();

    NFUNCTION(BlueprintCallable, Category="Utility")
    int32_t GetSum(int32_t A, int32_t B);

    NFUNCTION(BlueprintCallable, Category="Utility")
    float CalculateCircleArea(float Radius);

private:
    // 私有成员不需要反射
    bool bInitialized = false;
};

} // namespace Nut

// 包含生成的反射代码
#include "NExampleClass.generate.h"