#pragma once

#include "NObject.h"

namespace Nut {

// 前向声明
class NTestObject;

/**
 * @brief 测试对象 - 用于验证GC功能
 * 
 * 演示了如何正确继承NObject并实现引用收集
 */
class NTestObject : public NObject {
    DECLARE_NOBJECT_CLASS(NTestObject, NObject)

public:
    NTestObject(int32_t Value = 0);
    virtual ~NTestObject();

    // 获取/设置值
    int32_t GetValue() const { return TestValue; }
    void SetValue(int32_t Value) { TestValue = Value; }

    // 引用管理
    void SetReference(NSharedPtr<NTestObject> Other);
    NSharedPtr<NTestObject> GetReference() const { return RefObject; }

    // 添加子对象
    void AddChild(NSharedPtr<NTestObject> Child);
    const NVector<NSharedPtr<NTestObject>>& GetChildren() const { return Children; }

    // 重写引用收集方法
    virtual void CollectReferences(NVector<NObject*>& OutReferences) const override;

private:
    int32_t TestValue;
    NSharedPtr<NTestObject> RefObject;                 // 单个引用
    NVector<NSharedPtr<NTestObject>> Children;       // 子对象列表，使用tcmalloc分配器
};

/**
 * @brief GC测试工具类
 * 
 * 提供各种测试场景来验证GC功能
 */
class GCTester {
public:
    /**
     * @brief 基础引用计数测试
     */
    static void TestBasicRefCounting();

    /**
     * @brief 循环引用测试
     */
    static void TestCircularReferences();

    /**
     * @brief 复杂对象图测试
     */
    static void TestComplexObjectGraph();

    /**
     * @brief GC性能测试
     */
    static void TestGCPerformance();

    /**
     * @brief 运行所有测试
     */
    static void RunAllTests();

private:
    static void PrintGCStats();
    static void CreateCircularReferences(int32_t Count);
    static void CreateComplexGraph(int32_t Depth, int32_t Width);
};

} // namespace Nut
