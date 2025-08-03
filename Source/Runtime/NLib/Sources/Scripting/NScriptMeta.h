#pragma once

/**
 * @file NScriptMeta.h
 * @brief 脚本集成元数据系统
 * 
 * 扩展NCLASS反射系统，支持脚本绑定的元数据标记
 * 支持多种脚本语言：Lua, Python, JavaScript等
 */

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"

namespace NLib
{

/**
 * @brief 脚本访问权限类型
 */
enum class EScriptAccess : uint32_t
{
    None = 0,
    Read = 1 << 0,          // 脚本可读
    Write = 1 << 1,         // 脚本可写
    ReadWrite = Read | Write, // 脚本可读写
    Call = 1 << 2,          // 脚本可调用（函数）
    Create = 1 << 3,        // 脚本可创建实例（类）
    All = Read | Write | Call | Create
};

ENUM_CLASS_FLAGS(EScriptAccess)

/**
 * @brief 支持的脚本语言类型
 */
enum class EScriptLanguage : uint32_t
{
    None = 0,
    Lua = 1 << 0,           // 标准Lua脚本
    LuaClass = 1 << 1,      // 魔改Lua (支持类系统)
    Python = 1 << 2,        // Python脚本
    TypeScript = 1 << 3,    // TypeScript (编译到JavaScript)
    JavaScript = 1 << 4,    // JavaScript/V8
    CSharp = 1 << 5,        // C# (Mono/CoreCLR)
    NBP = 1 << 6,           // NLib Blueprint Script (自定义脚本语言)
    All = Lua | LuaClass | Python | TypeScript | JavaScript | CSharp | NBP
};

ENUM_CLASS_FLAGS(EScriptLanguage)

/**
 * @brief 脚本元数据信息
 */
struct LIBNLIB_API NScriptMetaInfo
{
    EScriptAccess Access;           // 访问权限
    EScriptLanguage Languages;      // 支持的脚本语言
    CString ScriptName;             // 脚本中的名称（可选，默认使用C++名称）
    CString Description;            // 描述信息
    CString Category;               // 分类
    bool bDeprecated;               // 是否已废弃
    CString DeprecationMessage;     // 废弃消息
    
    NScriptMetaInfo();
    NScriptMetaInfo(EScriptAccess InAccess, EScriptLanguage InLanguages = EScriptLanguage::All);
    
    bool HasAccess(EScriptAccess CheckAccess) const;
    bool SupportsLanguage(EScriptLanguage Language) const;
    bool IsScriptAccessible() const;
};

/**
 * @brief 脚本属性元数据
 */
struct LIBNLIB_API NScriptPropertyMeta : public NScriptMetaInfo
{
    bool bReadOnly;                 // 是否只读
    bool bTransient;                // 是否临时（不序列化到脚本）
    CString DefaultValue;           // 默认值（字符串形式）
    CString Validator;              // 验证器函数名
    float MinValue;                 // 最小值（数值类型）
    float MaxValue;                 // 最大值（数值类型）
    
    NScriptPropertyMeta();
    NScriptPropertyMeta(EScriptAccess InAccess, bool bInReadOnly = false);
};

/**
 * @brief 脚本函数元数据
 */
struct LIBNLIB_API NScriptFunctionMeta : public NScriptMetaInfo
{
    bool bPure;                     // 是否纯函数（无副作用）
    bool bAsync;                    // 是否异步函数
    bool bStatic;                   // 是否静态函数
    CString ReturnValueDescription; // 返回值描述
    CArray<CString> ParameterNames; // 参数名称
    CArray<CString> ParameterDescriptions; // 参数描述
    CArray<CString> ParameterDefaultValues; // 参数默认值
    
    NScriptFunctionMeta();
    NScriptFunctionMeta(EScriptAccess InAccess, bool bInPure = false);
};

/**
 * @brief 脚本类元数据
 */
struct LIBNLIB_API NScriptClassMeta : public NScriptMetaInfo
{
    bool bAbstract;                 // 是否抽象类
    bool bSingleton;                // 是否单例
    CString BaseClassName;          // 基类名称
    CArray<CString> InterfaceNames; // 实现的接口
    CString FactoryFunction;        // 工厂函数名
    
    NScriptClassMeta();
    NScriptClassMeta(EScriptAccess InAccess, bool bInAbstract = false);
};

/**
 * @brief 脚本元数据注册表
 */
class LIBNLIB_API NScriptMetaRegistry
{
public:
    static NScriptMetaRegistry& Get();
    
    // 类元数据注册
    void RegisterClass(const CString& ClassName, const NScriptClassMeta& Meta);
    void RegisterProperty(const CString& ClassName, const CString& PropertyName, const NScriptPropertyMeta& Meta);
    void RegisterFunction(const CString& ClassName, const CString& FunctionName, const NScriptFunctionMeta& Meta);
    
    // 全局函数注册
    void RegisterGlobalFunction(const CString& FunctionName, const NScriptFunctionMeta& Meta);
    
    // 查询接口
    const NScriptClassMeta* GetClassMeta(const CString& ClassName) const;
    const NScriptPropertyMeta* GetPropertyMeta(const CString& ClassName, const CString& PropertyName) const;
    const NScriptFunctionMeta* GetFunctionMeta(const CString& ClassName, const CString& FunctionName) const;
    const NScriptFunctionMeta* GetGlobalFunctionMeta(const CString& FunctionName) const;
    
    // 枚举接口
    CArray<CString> GetScriptAccessibleClasses(EScriptLanguage Language = EScriptLanguage::All) const;
    CArray<CString> GetScriptAccessibleProperties(const CString& ClassName, EScriptLanguage Language = EScriptLanguage::All) const;
    CArray<CString> GetScriptAccessibleFunctions(const CString& ClassName, EScriptLanguage Language = EScriptLanguage::All) const;
    CArray<CString> GetGlobalScriptFunctions(EScriptLanguage Language = EScriptLanguage::All) const;
    
    // 验证接口
    bool IsClassScriptAccessible(const CString& ClassName, EScriptLanguage Language) const;
    bool IsPropertyScriptAccessible(const CString& ClassName, const CString& PropertyName, EScriptAccess Access, EScriptLanguage Language) const;
    bool IsFunctionScriptCallable(const CString& ClassName, const CString& FunctionName, EScriptLanguage Language) const;
    
    // 清理
    void Clear();

private:
    NScriptMetaRegistry() = default;
    
    CHashMap<CString, NScriptClassMeta> ClassMetas;
    CHashMap<CString, CHashMap<CString, NScriptPropertyMeta>> PropertyMetas; // ClassName -> PropertyName -> Meta
    CHashMap<CString, CHashMap<CString, NScriptFunctionMeta>> FunctionMetas; // ClassName -> FunctionName -> Meta
    CHashMap<CString, NScriptFunctionMeta> GlobalFunctionMetas;
    
    mutable NMutex RegistryMutex;
};

/**
 * @brief 脚本元数据自动注册器
 */
template<typename TClass>
class CScriptMetaRegistrar
{
public:
    CScriptMetaRegistrar(const CString& ClassName, const NScriptClassMeta& ClassMeta)
    {
        NScriptMetaRegistry::Get().RegisterClass(ClassName, ClassMeta);
    }
    
    CScriptMetaRegistrar& Property(const CString& PropertyName, const NScriptPropertyMeta& Meta)
    {
        NScriptMetaRegistry::Get().RegisterProperty(TClass::GetStaticTypeName(), PropertyName, Meta);
        return *this;
    }
    
    CScriptMetaRegistrar& Function(const CString& FunctionName, const NScriptFunctionMeta& Meta)
    {
        NScriptMetaRegistry::Get().RegisterFunction(TClass::GetStaticTypeName(), FunctionName, Meta);
        return *this;
    }
};

} // namespace NLib

/**
 * @brief 扩展的NCLASS宏 - 支持脚本元数据
 * 
 * 使用示例：
 * NCLASS(ScriptCreatable, ScriptName="Player", Category="Gameplay")
 * class CPlayer : public CObject
 * {
 * public:
 *     NPROPERTY(ScriptReadWrite, Description="Player health")
 *     float Health = 100.0f;
 *     
 *     NFUNCTION(ScriptCallable, Description="Take damage")
 *     void TakeDamage(float Amount);
 * };
 */

// 重新定义NCLASS宏以支持脚本元数据解析
#undef NCLASS
#undef NPROPERTY
#undef NFUNCTION

/**
 * @brief NCLASS宏 - 支持脚本集成元数据
 * 
 * 支持的元数据标记：
 * - ScriptCreatable: 脚本可创建实例
 * - ScriptName="Name": 脚本中的名称
 * - Category="Category": 分类
 * - Description="Desc": 描述
 * - Languages=Lua|Python: 支持的脚本语言
 * - Deprecated="Message": 标记为废弃
 */
#define NCLASS(...) \
    /* 这个宏会被NutHeaderTools解析并生成对应的脚本元数据注册代码 */

/**
 * @brief NPROPERTY宏 - 支持脚本属性元数据
 * 
 * 支持的元数据标记：
 * - ScriptReadable: 脚本可读
 * - ScriptWritable: 脚本可写
 * - ScriptReadWrite: 脚本可读写
 * - ScriptName="Name": 脚本中的名称
 * - Description="Desc": 描述
 * - ReadOnly: 只读属性
 * - Transient: 临时属性（不序列化到脚本）
 * - Min=Value: 最小值
 * - Max=Value: 最大值
 * - DefaultValue="Value": 默认值
 * - Validator="FunctionName": 验证器函数
 */
#define NPROPERTY(...) \
    /* 这个宏会被NutHeaderTools解析并生成对应的脚本元数据注册代码 */

/**
 * @brief NFUNCTION宏 - 支持脚本函数元数据
 * 
 * 支持的元数据标记：
 * - ScriptCallable: 脚本可调用
 * - ScriptName="Name": 脚本中的名称
 * - Description="Desc": 描述
 * - Pure: 纯函数（无副作用）
 * - Async: 异步函数
 * - Static: 静态函数
 * - ReturnDescription="Desc": 返回值描述
 * - ParamNames="name1,name2": 参数名称
 * - ParamDescriptions="desc1,desc2": 参数描述
 * - ParamDefaults="val1,val2": 参数默认值
 */
#define NFUNCTION(...) \
    /* 这个宏会被NutHeaderTools解析并生成对应的脚本元数据注册代码 */

/**
 * @brief 脚本元数据注册宏
 * 
 * 这个宏会被NutHeaderTools生成在.generate.h文件中
 */
#define REGISTER_SCRIPT_META(ClassName) \
    static NLib::CScriptMetaRegistrar<ClassName> ClassName##_ScriptMeta_Registrar(#ClassName, /* 生成的元数据 */);

/**
 * @brief 便利宏定义
 */
#define SCRIPT_CREATABLE        ScriptCreatable
#define SCRIPT_READABLE         ScriptReadable  
#define SCRIPT_WRITABLE         ScriptWritable
#define SCRIPT_READWRITE        ScriptReadWrite
#define SCRIPT_CALLABLE         ScriptCallable