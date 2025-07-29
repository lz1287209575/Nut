#pragma once

#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace Nut
{

// 前向声明
class NObject;

/**
 * @brief 属性反射信息
 */
struct NPropertyReflection
{
    std::string Name;
    std::string Type;
    size_t Offset;
    std::function<void *(NObject *)> Getter;
    std::function<void(NObject *, const void *)> Setter;
};

/**
 * @brief 函数反射信息
 */
struct NFunctionReflection
{
    std::string Name;
    std::string ReturnType;
    std::vector<std::string> ParameterTypes;
    std::function<void(NObject *, void *, void **)> Invoker;
};

/**
 * @brief 类反射信息
 */
struct NClassReflection
{
    std::string ClassName;
    std::string BaseClassName;
    const std::type_info *TypeInfo;
    std::function<NObject *()> Factory;
    std::vector<NPropertyReflection> Properties;
    std::vector<NFunctionReflection> Functions;
};

/**
 * @brief 反射系统管理器
 * 提供运行时类型信息
 */
class NObjectReflection
{
  public:
    /**
     * @brief 获取反射系统单例
     */
    static NObjectReflection &GetInstance();

    /**
     * @brief 注册类反射信息
     */
    void RegisterClass(const std::string &className, const NClassReflection &reflection);

    /**
     * @brief 根据类名获取反射信息
     */
    const NClassReflection *GetClassReflection(const std::string &className) const;

    /**
     * @brief 根据类型信息获取反射信息
     */
    const NClassReflection *GetClassReflection(const std::type_info &typeInfo) const;

    /**
     * @brief 获取所有注册的类
     */
    std::vector<std::string> GetAllClassNames() const;

    /**
     * @brief 通过类名创建对象实例
     */
    NObject *CreateInstance(const std::string &className) const;

    /**
     * @brief 检查类是否继承自指定基类
     */
    bool IsChildOf(const std::string &className, const std::string &baseClassName) const;

  private:
    std::unordered_map<std::string, NClassReflection> ClassReflections;
    std::unordered_map<const std::type_info *, std::string> TypeInfoToClassName;
};

/**
 * @brief 自动注册类反射信息的辅助类
 */
template <typename T> class NClassRegistrar
{
  public:
    NClassRegistrar(const std::string &className, const std::string &baseClassName)
    {
        NClassReflection reflection;
        reflection.ClassName = className;
        reflection.BaseClassName = baseClassName;
        reflection.TypeInfo = &typeid(T);
        reflection.Factory = []() -> NObject * { return new T(); };

        NObjectReflection::GetInstance().RegisterClass(className, reflection);
    }
};

/**
 * @brief 宏：注册NCLASS到反射系统
 */
#define REGISTER_NCLASS_REFLECTION(ClassName)                                                                          \
    static NClassRegistrar<ClassName> ClassName##_Registrar(#ClassName, "NObject");

/**
 * @brief 宏：在NCLASS中生成静态类型信息
 */
#define NCLASS_REFLECTION_BODY(ClassName)                                                                              \
  public:                                                                                                              \
    static const char *GetStaticTypeName()                                                                             \
    {                                                                                                                  \
        return #ClassName;                                                                                             \
    }                                                                                                                  \
    virtual const NClassReflection *GetClassReflection() const override                                                \
    {                                                                                                                  \
        return NObjectReflection::GetInstance().GetClassReflection(#ClassName);                                        \
    }                                                                                                                  \
                                                                                                                       \
  private:

} // namespace Nut
