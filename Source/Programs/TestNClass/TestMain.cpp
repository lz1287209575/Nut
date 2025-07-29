#include "NLib.h" 
#include "NExampleClass.h"
#include "NObjectReflection.h"
#include <iostream>

using namespace Nut;

int main()
{
    // 初始化 NLib
    NLibConfig config;
    config.bEnableBackgroundGC = false; // 暂时禁用GC避免tcmalloc问题
    config.bEnableFileLogging = false;   // 暂时禁用文件日志
    config.LogLevel = NLogger::LogLevel::Info;
    
    if (!NLib::Initialize(config))
    {
        std::cout << "Failed to initialize NLib" << std::endl;
        return -1;
    }
    
    NLogger::Info("🚀 NClass 系统测试开始");
    
    // 测试反射系统
    NLogger::Info("📊 反射系统测试:");
    
    // 获取所有注册的类
    auto allClasses = NObjectReflection::GetInstance().GetAllClassNames();
    NLogger::Info("已注册的类数量: {}", allClasses.size());
    
    for (const auto& className : allClasses)
    {
        NLogger::Info("  - {}", className);
        
        const NClassReflection* reflection = 
            NObjectReflection::GetInstance().GetClassReflection(className);
        
        if (reflection)
        {
            NLogger::Info("    基类: {}", reflection->BaseClassName);
            NLogger::Info("    属性数量: {}", reflection->Properties.size());
            NLogger::Info("    函数数量: {}", reflection->Functions.size());
        }
    }
    
    // 测试对象创建
    NLogger::Info("\n🏭 对象创建测试:");
    
    NSharedPtr<NExampleClass> example = NObject::Create<NExampleClass>();
    if (example)
    {
        NLogger::Info("✅ NExampleClass 创建成功");
        
        // 测试属性
        example->ExampleIntProperty = 42;
        example->ExampleFloatProperty = 3.14f;
        example->ExampleStringProperty = "Hello NCLASS!";
        
        // 测试函数调用
        example->ExampleFunction();
        
        int sum = example->GetSum(10, 20);
        NLogger::Info("Sum result: {}", sum);
        
        float area = example->CalculateCircleArea(5.0f);
        NLogger::Info("Circle area result: {:.2f}", area);
    }
    else
    {
        NLogger::Error("❌ NExampleClass 创建失败");
    }
    
    // 测试动态创建
    NLogger::Info("\n🎯 动态创建测试:");
    NObject* dynamicObj = NObjectReflection::GetInstance().CreateInstance("NExampleClass");
    if (dynamicObj)
    {
        NLogger::Info("✅ 动态创建 NExampleClass 成功");
        NExampleClass* examplePtr = static_cast<NExampleClass*>(dynamicObj);
        examplePtr->ExampleIntProperty = 999;
        examplePtr->ExampleFunction();
        delete dynamicObj;
    }
    else
    {
        NLogger::Error("❌ 动态创建失败");
    }
    
    NLogger::Info("\n🎉 NClass 系统测试完成!");
    
    NLib::Shutdown();
    return 0;
}