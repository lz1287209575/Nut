#pragma once

/**
 * @file NSharedFromThisExample.h
 * @brief NSharedFromThis使用示例
 * 
 * 这个文件展示了如何正确使用NSharedFromThis
 */

#include "Memory/NSmartPointers.h"
#include "Core/CObject.h"

namespace NLib
{

/**
 * @brief NSharedFromThis基本使用示例
 */
class LIBNLIB_API NExampleClass : public CSharedFromThis<NExampleClass>
{
public:
    NExampleClass() = default;
    virtual ~NExampleClass() = default;

    /**
     * @brief 创建子任务的示例方法
     * 
     * 这个方法展示了如何在对象内部安全地获取指向自身的shared_ptr
     */
    void StartAsyncTask()
    {
        // 安全地获取指向自己的shared_ptr
        auto self = SharedFromThis();
        
        // 可以将self传递给异步任务、回调函数等
        // 确保对象在异步操作完成前不会被销毁
        PostAsyncTask([self, this]() {
            this->OnTaskCompleted();
        });
    }

    /**
     * @brief 尝试获取自引用的安全版本
     */
    void TrySafeOperation()
    {
        // 使用Try版本，不会抛出异常
        auto self = TrySharedFromThis();
        if (self.IsValid())
        {
            // 安全操作
            DoSomethingWithSelf(self);
        }
        else
        {
            // 对象未被shared_ptr管理
            CLogger::Get().LogWarning("Object not managed by shared_ptr");
        }
    }

    /**
     * @brief 获取弱引用的示例
     */
    TWeakPtr<NExampleClass> GetWeakReference()
    {
        return WeakFromThis();
    }

    /**
     * @brief 使用SharedRef的示例
     */
    void UseSharedRef()
    {
        try
        {
            auto selfRef = SharedRefFromThis();
            // selfRef保证非空，无需检查
            DoSomethingWithSelfRef(selfRef);
        }
        catch (const std::exception& e)
        {
            CLogger::Get().LogError("Failed to get SharedRef: {}", e.what());
        }
    }

private:
    void OnTaskCompleted()
    {
        CLogger::Get().LogInfo("Async task completed");
    }

    void DoSomethingWithSelf(TSharedPtr<NExampleClass> Self)
    {
        CLogger::Get().LogInfo("Doing something with self, ref count: {}", 
                              Self.GetSharedReferenceCount());
    }

    void DoSomethingWithSelfRef(TSharedRef<NExampleClass> Self)
    {
        CLogger::Get().LogInfo("Doing something with self ref, ref count: {}", 
                              Self.GetSharedReferenceCount());
    }

    void PostAsyncTask(const NFunction<void()>& Task)
    {
        // 这里应该将任务投递到线程池或事件循环
        // 为了示例，我们直接调用
        Task();
    }
};

/**
 * @brief 继承自NObject的NSharedFromThis示例
 */
class LIBNLIB_API NExampleNObject : public CObject, public CSharedFromThis<NExampleNObject>
{
    NCLASS()
class NExampleNObject : public NObject
{
    GENERATED_BODY()

public:
    NExampleNObject() = default;
    virtual ~NExampleNObject() = default;

    /**
     * @brief 展示NObject和NSharedFromThis的协同工作
     */
    void RegisterForCallback()
    {
        auto self = SharedFromThis();
        
        // 将自己注册到某个管理器中
        // 使用shared_ptr确保对象生命周期
        SomeManager::RegisterCallback(self, [](TSharedPtr<NExampleNObject> obj) {
            obj->OnCallback();
        });
    }

    /**
     * @brief 在集合中存储自引用
     */
    void AddToCollection()
    {
        auto self = SharedFromThis();
        
        // 将自己添加到某个全局集合中
        // 这是安全的，因为使用了shared_ptr
        GlobalObjectCollection::Add(self);
    }

private:
    void OnCallback()
    {
        CLogger::Get().LogInfo("Callback received for CObject: {}", GetObjectId());
    }

    // 模拟的全局管理器
    struct SomeManager
    {
        static void RegisterCallback(TSharedPtr<NExampleNObject> obj, 
                                   NFunction<void(TSharedPtr<NExampleNObject>)> callback)
        {
            // 实际实现会存储这些回调
            callback(obj);
        }
    };

    struct GlobalObjectCollection
    {
        static void Add(TSharedPtr<NExampleNObject> obj)
        {
            // 实际实现会将对象添加到集合中
            CLogger::Get().LogInfo("Added object {} to collection", obj->GetObjectId());
        }
    };
};

/**
 * @brief 使用示例函数
 */
namespace SharedFromThisExamples
{
    void BasicUsageExample()
    {
        // 正确的使用方式：通过MakeShared创建对象
        auto obj = MakeShared<NExampleClass>();
        obj->StartAsyncTask();
        obj->TrySafeOperation();
        
        // 获取弱引用
        auto weakRef = obj->GetWeakReference();
        
        // 重置原始指针
        obj.Reset();
        
        // 检查弱引用是否仍然有效
        if (auto locked = weakRef.Lock())
        {
            CLogger::Get().LogInfo("Object still alive");
        }
        else
        {
            CLogger::Get().LogInfo("Object has been destroyed");
        }
    }

    void NObjectIntegrationExample()
    {
        // 使用NewNObjectModern创建NObject派生类
        auto nobj = NewNObjectModern<NExampleNObject>();
        nobj->RegisterForCallback();
        nobj->AddToCollection();
    }

    void ErrorHandlingExample()
    {
        // 错误示例：直接使用new创建对象，然后调用SharedFromThis
        // 这会导致运行时错误
        /*
        auto* rawPtr = new NExampleClass();
        try
        {
            auto shared = rawPtr->SharedFromThis(); // 这会失败！
        }
        catch (const std::exception& e)
        {
            CLogger::Get().LogError("Expected error: {}", e.what());
        }
        delete rawPtr;
        */

        // 正确示例：使用Try版本避免异常
        auto* rawPtr = new NExampleClass();
        auto shared = rawPtr->TrySharedFromThis();
        if (!shared.IsValid())
        {
            CLogger::Get().LogInfo("Object not managed by shared_ptr (expected)");
        }
        delete rawPtr;
    }
}

} // namespace NLib