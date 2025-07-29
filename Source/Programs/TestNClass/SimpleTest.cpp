#include "NExampleClass.h"
#include "NObjectReflection.h"
#include "NLogger.h"
#include <iostream>

using namespace Nut;

int main()
{
    std::cout << "🚀 NCLASS 系统简单测试开始" << std::endl;
    
    // 测试反射系统
    std::cout << "📊 反射系统测试:" << std::endl;
    
    // 获取所有注册的类
    auto allClasses = NObjectReflection::GetInstance().GetAllClassNames();
    std::cout << "已注册的类数量: " << allClasses.size() << std::endl;
    
    for (const auto& className : allClasses)
    {
        std::cout << "  - " << className << std::endl;
        
        const NClassReflection* reflection = 
            NObjectReflection::GetInstance().GetClassReflection(className);
        
        if (reflection)
        {
            std::cout << "    基类: " << reflection->BaseClassName << std::endl;
            std::cout << "    属性数量: " << reflection->Properties.size() << std::endl;
            std::cout << "    函数数量: " << reflection->Functions.size() << std::endl;
        }
    }
    
    // 测试对象创建
    std::cout << "\n🏭 对象创建测试:" << std::endl;
    
    try {
        NExampleClass* example = new NExampleClass();
        if (example)
        {
            std::cout << "✅ NExampleClass 创建成功" << std::endl;
            
            // 测试属性
            example->ExampleIntProperty = 42;
            example->ExampleFloatProperty = 3.14f;
            example->ExampleStringProperty = "Hello NCLASS!";
            
            std::cout << "设置属性: Int=" << example->ExampleIntProperty 
                      << ", Float=" << example->ExampleFloatProperty 
                      << ", String=" << example->ExampleStringProperty << std::endl;
            
            // 测试类型信息
            std::cout << "类型名称: " << example->GetTypeName() << std::endl;
            
            delete example;
        }
        else
        {
            std::cout << "❌ NExampleClass 创建失败" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ 异常: " << e.what() << std::endl;
    }
    
    // 测试动态创建
    std::cout << "\n🎯 动态创建测试:" << std::endl;
    try {
        NObject* dynamicObj = NObjectReflection::GetInstance().CreateInstance("NExampleClass");
        if (dynamicObj)
        {
            std::cout << "✅ 动态创建 NExampleClass 成功" << std::endl;
            NExampleClass* examplePtr = static_cast<NExampleClass*>(dynamicObj);
            examplePtr->ExampleIntProperty = 999;
            std::cout << "动态创建对象的属性: " << examplePtr->ExampleIntProperty << std::endl;
            delete dynamicObj;
        }
        else
        {
            std::cout << "❌ 动态创建失败" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ 动态创建异常: " << e.what() << std::endl;
    }
    
    std::cout << "\n🎉 NCLASS 系统测试完成!" << std::endl;
    
    return 0;
}