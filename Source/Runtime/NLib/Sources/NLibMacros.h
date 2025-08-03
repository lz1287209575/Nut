#pragma once

// ===============================
// 命名规范宏定义
// ===============================

/**
 * @brief 命名规范说明：
 * 
 * N*** - 继承自NObject的类（托管对象，支持GC和反射）
 * C*** - 普通C++类（非托管，不继承NObject）
 * I*** - 接口类（纯虚函数类）
 * E*** - 枚举类型
 * TType*** - 模板类
 * 
 * 模板参数命名：
 * TType - 类型参数
 * TArgs - 参数包
 * TFunction - 函数类型
 * TKey, TValue - 键值对
 * TIndex - 索引类型
 * TSize - 大小类型
 * TAllocator - 分配器类型
 */

// ===============================
// 反射系统宏定义
// ===============================

/**
 * @brief NCLASS宏 - 标记需要反射支持的NObject子类
 * 使用方式：NCLASS(meta = (Category="Core", BlueprintType=true))
 */
#define NCLASS(...)

/**
 * @brief GENERATED_BODY宏 - 在NObject子类中插入生成的反射代码
 * 必须与NCLASS宏配合使用
 */
#define GENERATED_BODY() \
private: \
    friend class CObjectReflection; \
public: \
    using Super = CObject; \
    virtual const std::type_info& GetTypeInfo() const override { return typeid(*this); } \
    virtual const char* GetTypeName() const override { return GetStaticTypeName(); } \
    virtual const NClassReflection* GetClassReflection() const override; \
    static const char* GetStaticTypeName();

/**
 * @brief NPROPERTY宏 - 标记需要反射支持的属性
 * 使用方式：NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
 */
#define NPROPERTY(...)

/**
 * @brief NFUNCTION宏 - 标记需要反射支持的函数
 * 使用方式：NFUNCTION(BlueprintCallable, Category="Utility")
 */
#define NFUNCTION(...)

// ===============================
// 旧系统兼容宏（逐步迁移）
// ===============================

/**
 * @brief DECLARE_NOBJECT_CLASS - 旧系统，逐步迁移到NCLASS+GENERATED_BODY
 * @deprecated 请使用 NCLASS() + GENERATED_BODY() 替代
 */
#define NCLASS()
class ClassName : public SuperClass
{
    GENERATED_BODY() \
public: \
    using Super = SuperClass; \
    virtual const std::type_info& GetTypeInfo() const override { return typeid(ClassName); } \
    virtual const char* GetTypeName() const override { return #ClassName; } \
private:

/**
 * @brief DECLARE_NOBJECT_ABSTRACT_CLASS - 旧系统抽象类声明
 * @deprecated 请使用 NCLASS() + GENERATED_BODY() 替代
 */
#define DECLARE_NOBJECT_ABSTRACT_CLASS(ClassName, SuperClass) \
public: \
    using Super = SuperClass; \
    virtual const std::type_info& GetTypeInfo() const override { return typeid(ClassName); } \
    virtual const char* GetTypeName() const override { return #ClassName; } \
private:

// ===============================
// 模板类命名规范
// ===============================

/**
 * @brief 模板类命名规范示例：
 * 
 * template<typename TType>
 * class TArray { ... };
 * 
 * template<typename TKey, typename TValue>
 * class THashMap { ... };
 * 
 * template<typename TFunction>
 * class TDelegate { ... };
 * 
 * template<typename... TArgs>
 * class TVariant { ... };
 */

// ===============================
// 接口类命名规范
// ===============================

/**
 * @brief 接口类命名规范示例：
 * 
 * class IAllocator {
public:
    virtual ~IAllocator() = default;
    virtual void* Allocate(size_t Size) = 0;
    virtual void Deallocate(void* Ptr) = 0;
};
 * class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual bool Serialize(CArchive& Archive) = 0;
    virtual bool Deserialize(CArchive& Archive) = 0;
};
 * class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void HandleEvent(const NEvent& Event) = 0;
};
 */

// ===============================
// 枚举类命名规范
// ===============================

/**
 * @brief 枚举类命名规范示例：
 * 
 * enum class ESocketType { ... };
 * enum class EAsyncTaskState { ... };
 * enum class ELogLevel { ... };
 */

// ===============================
// 普通类命名规范
// ===============================

/**
 * @brief 普通类命名规范示例：
 * 
 * class CConfigParser { ... };
 * class CFileWatcher { ... };
 * class CPerformanceCounter { ... };
 */

// ===============================
// 生成文件包含提醒
// ===============================

/**
 * @brief 提醒包含生成的头文件
 * 对于使用NCLASS的类，需要在.cpp文件中包含对应的.generate.h文件
 */
#define GENERATED_INCLUDE_REMINDER() /* 请在.cpp文件中添加: #include "ClassName.generate.h" */

// ===============================
// 迁移辅助宏
// ===============================

/**
 * @brief 迁移辅助宏 - 标记需要迁移的类
 * 用于标识当前使用旧系统但需要迁移到新系统的类
 */
#define MIGRATE_TO_NCLASS(ClassName) \
    static_assert(false, "Class " #ClassName " needs migration from DECLARE_NOBJECT_CLASS to NCLASS+GENERATED_BODY")

// ===============================
// 编译时检查
// ===============================

/**
 * @brief 检查类命名规范
 * 在开发阶段帮助确保命名规范的一致性
 */
#ifdef NLIB_ENABLE_NAMING_CHECKS
    #define CHECK_NCLASS_NAMING(ClassName) \
        static_assert(ClassName[0] == 'N' && ClassName[1] == ClassName[1], \
                     "NObject-derived classes should start with 'N'")
#else
    #define CHECK_NCLASS_NAMING(ClassName)
#endif