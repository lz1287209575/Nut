#include "NTestObject.h"
#include "NGarbageCollector.h"
#include "NLogger.h"
#include <chrono>
#include <sstream>

namespace Nut {

// === NTestObject 实现 ===

NTestObject::NTestObject(int32_t Value) 
    : NObject()
    , TestValue(Value)
{
    NLogger::Debug("NTestObject created with value: " + std::to_string(Value) + 
                    ", ID: " + std::to_string(GetObjectId()));
}

NTestObject::~NTestObject() {
    NLogger::Debug("NTestObject destroyed with value: " + std::to_string(TestValue) + 
                    ", ID: " + std::to_string(GetObjectId()));
}

void NTestObject::SetReference(NSharedPtr<NTestObject> Other) {
    RefObject = Other;
    if (Other.IsValid()) {
        NLogger::Debug("NTestObject " + std::to_string(GetObjectId()) + 
                        " now references " + std::to_string(Other->GetObjectId()));
    }
}

void NTestObject::AddChild(NSharedPtr<NTestObject> Child) {
    if (Child.IsValid()) {
        Children.push_back(Child);
        NLogger::Debug("NTestObject " + std::to_string(GetObjectId()) + 
                        " added child " + std::to_string(Child->GetObjectId()));
    }
}

void NTestObject::CollectReferences(NVector<NObject*>& OutReferences) const {
    // 收集单个引用
    if (RefObject.IsValid()) {
        OutReferences.push_back(RefObject.Get());
    }
    
    // 收集所有子对象
    for (const auto& Child : Children) {
        if (Child.IsValid()) {
            OutReferences.push_back(Child.Get());
        }
    }
    
    NLogger::Debug("NTestObject " + std::to_string(GetObjectId()) + 
                    " collected " + std::to_string(OutReferences.size()) + " references");
}

// === GCTester 实现 ===

void GCTester::TestBasicRefCounting() {
    NLogger::Info("=== Starting Basic Reference Counting Test ===");
    
    auto& GC = NGarbageCollector::GetInstance();
    PrintGCStats();
    
    {
        // 创建对象
        auto Obj1 = NObject::Create<NTestObject>(100);
        auto Obj2 = NObject::Create<NTestObject>(200);
        
        NLogger::Info("Created 2 objects");
        PrintGCStats();
        
        // 创建引用
        Obj1->SetReference(Obj2);
        
        // 执行GC（应该不回收任何对象，因为都有引用）
        uint32_t Collected = GC.Collect();
        NLogger::Info("GC collected " + std::to_string(Collected) + " objects");
        PrintGCStats();
        
    } // 对象离开作用域，智能指针自动释放
    
    NLogger::Info("Objects went out of scope");
    PrintGCStats();
    
    // 再次执行GC
    uint32_t Collected = GC.Collect();
    NLogger::Info("Final GC collected " + std::to_string(Collected) + " objects");
    PrintGCStats();
    
    NLogger::Info("=== Basic Reference Counting Test Completed ===\n");
}

void GCTester::TestCircularReferences() {
    NLogger::Info("=== Starting Circular References Test ===");
    
    auto& GC = NGarbageCollector::GetInstance();
    PrintGCStats();
    
    CreateCircularReferences(5);
    
    NLogger::Info("Created circular references");
    PrintGCStats();
    
    // 执行GC来清理循环引用
    uint32_t Collected = GC.Collect();
    NLogger::Info("GC collected " + std::to_string(Collected) + " objects");
    PrintGCStats();
    
    NLogger::Info("=== Circular References Test Completed ===\n");
}

void GCTester::TestComplexObjectGraph() {
    NLogger::Info("=== Starting Complex Object Graph Test ===");
    
    auto& GC = NGarbageCollector::GetInstance();
    PrintGCStats();
    
    CreateComplexGraph(3, 4);
    
    NLogger::Info("Created complex object graph");
    PrintGCStats();
    
    // 执行GC
    uint32_t Collected = GC.Collect();
    NLogger::Info("GC collected " + std::to_string(Collected) + " objects");
    PrintGCStats();
    
    NLogger::Info("=== Complex Object Graph Test Completed ===\n");
}

void GCTester::TestGCPerformance() {
    NLogger::Info("=== Starting GC Performance Test ===");
    
    auto& GC = NGarbageCollector::GetInstance();
    
    // 创建大量对象
    const int32_t ObjectCount = 1000;
    NVector<NSharedPtr<NTestObject>> Objects;
    Objects.reserve(ObjectCount);
    
    auto StartTime = std::chrono::high_resolution_clock::now();
    
    // 创建对象
    for (int32_t i = 0; i < ObjectCount; ++i) {
        auto Obj = NObject::Create<NTestObject>(i);
        Objects.push_back(Obj);
        
        // 创建一些随机引用
        if (i > 0 && i % 10 == 0) {
            int32_t RefIndex = i - (i % 10);
            if (RefIndex < Objects.size()) {
                Obj->SetReference(Objects[RefIndex]);
            }
        }
    }
    
    auto CreateTime = std::chrono::high_resolution_clock::now();
    auto CreateDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        CreateTime - StartTime).count();
    
    NLogger::Info("Created " + std::to_string(ObjectCount) + " objects in " + 
                   std::to_string(CreateDuration) + "ms");
    PrintGCStats();
    
    // 清除一半对象的引用
    Objects.erase(Objects.begin() + ObjectCount/2, Objects.end());
    
    auto ClearTime = std::chrono::high_resolution_clock::now();
    
    // 执行GC
    uint32_t Collected = GC.Collect();
    
    auto GCTime = std::chrono::high_resolution_clock::now();
    auto GCDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        GCTime - ClearTime).count();
    
    NLogger::Info("GC collected " + std::to_string(Collected) + 
                   " objects in " + std::to_string(GCDuration) + "ms");
    PrintGCStats();
    
    Objects.clear(); // 清除剩余引用
    
    // 最终GC
    uint32_t FinalCollected = GC.Collect();
    auto FinalTime = std::chrono::high_resolution_clock::now();
    auto FinalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        FinalTime - GCTime).count();
    
    NLogger::Info("Final GC collected " + std::to_string(FinalCollected) + 
                   " objects in " + std::to_string(FinalDuration) + "ms");
    PrintGCStats();
    
    NLogger::Info("=== GC Performance Test Completed ===\n");
}

void GCTester::RunAllTests() {
    NLogger::Info("🧪 Starting LibNut GC Test Suite");
    
    // 初始化GC
    auto& GC = NGarbageCollector::GetInstance();
    if (!GC.IsInitialized()) {
        GC.Initialize(NGarbageCollector::EGCMode::Manual, 5000, false);
        NLogger::Info("GC initialized for testing");
    }
    
    try {
        TestBasicRefCounting();
        TestCircularReferences();
        TestComplexObjectGraph();
        TestGCPerformance();
        
        // 显示最终统计
        auto Stats = GC.GetStats();
        NLogger::Info("=== Final GC Statistics ===");
        NLogger::Info("Total Collections: " + std::to_string(Stats.TotalCollections));
        NLogger::Info("Objects Collected: " + std::to_string(Stats.ObjectsCollected));
        NLogger::Info("Objects Alive: " + std::to_string(Stats.ObjectsAlive));
        NLogger::Info("Total Collection Time: " + std::to_string(Stats.TotalCollectionTime) + "ms");
        
        NLogger::Info("🎉 All GC tests completed successfully!");
        
    } catch (const std::exception& e) {
        NLogger::Error("GC test failed: " + std::string(e.what()));
    }
}

void GCTester::PrintGCStats() {
    auto& GC = NGarbageCollector::GetInstance();
    auto Stats = GC.GetStats();
    
    std::stringstream ss;
    ss << "GC Stats - Objects Alive: " << Stats.ObjectsAlive 
       << ", Total Collections: " << Stats.TotalCollections
       << ", Objects Collected: " << Stats.ObjectsCollected;
    
    NLogger::Info(ss.str());
}

void GCTester::CreateCircularReferences(int32_t Count) {
    NVector<NSharedPtr<NTestObject>> Objects;
    Objects.reserve(Count);
    
    // 创建对象
    for (int32_t i = 0; i < Count; ++i) {
        auto Obj = NObject::Create<NTestObject>(i);
        Objects.push_back(Obj);
    }
    
    // 创建循环引用：每个对象引用下一个对象，最后一个引用第一个
    for (int32_t i = 0; i < Count; ++i) {
        int32_t NextIndex = (i + 1) % Count;
        Objects[i]->SetReference(Objects[NextIndex]);
    }
    
    // 注意：这里没有将Objects存储到外部，
    // 所以当这个函数结束时，所有NSharedPtr都会被销毁
    // 但对象间仍然存在循环引用，只能通过GC回收
}

void GCTester::CreateComplexGraph(int32_t Depth, int32_t Width) {
    if (Depth <= 0) {
        return;
    }
    
    NVector<NSharedPtr<NTestObject>> CurrentLevel;
    NVector<NSharedPtr<NTestObject>> NextLevel;
    
    // 创建根对象
    auto Root = NObject::Create<NTestObject>(0);
    CurrentLevel.push_back(Root);
    
    // 逐层创建对象图
    for (int32_t Level = 1; Level < Depth; ++Level) {
        NextLevel.clear();
        
        for (auto& Parent : CurrentLevel) {
            // 为每个父对象创建Width个子对象
            for (int32_t i = 0; i < Width; ++i) {
                int32_t Value = Level * Width + i;
                auto Child = NObject::Create<NTestObject>(Value);
                Parent->AddChild(Child);
                NextLevel.push_back(Child);
                
                // 创建一些交叉引用
                if (NextLevel.size() > 1 && i % 2 == 0) {
                    NextLevel[NextLevel.size() - 2]->SetReference(Child);
                }
            }
        }
        
        CurrentLevel = NextLevel;
    }
    
    // 创建一些向上的引用（增加复杂性）
    if (CurrentLevel.size() > 0 && Root) {
        CurrentLevel[0]->SetReference(Root);
    }
}

} // namespace Nut
