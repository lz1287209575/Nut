#pragma once

/**
 * @file NScriptEngine.h
 * @brief 统一的脚本引擎接口
 * 
 * 提供统一的脚本执行和绑定接口，支持多种脚本语言
 */

#include "Core/CObject.h"
#include "Scripting/NScriptMeta.h"
#include "Memory/NSmartPointers.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Delegates/CDelegate.h"

namespace NLib
{

// 前向声明
class CScriptValue;
class CScriptContext;
class IScriptEngine;

/**
 * @brief 脚本值类型
 */
enum class EScriptValueType : uint32_t
{
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Object,
    Array,
    Function
};

/**
 * @brief 脚本值 - 统一的值表示
 */
class LIBNLIB_API CScriptValue
{
public:
    CScriptValue();
    CScriptValue(bool Value);
    CScriptValue(int32_t Value);
    CScriptValue(float Value);
    CScriptValue(double Value);
    CScriptValue(const CString& Value);
    CScriptValue(const char* Value);
    CScriptValue(CObject* Object);
    CScriptValue(const CScriptValue& Other);
    CScriptValue(CScriptValue&& Other) noexcept;
    ~CScriptValue();

    CScriptValue& operator=(const CScriptValue& Other);
    CScriptValue& operator=(CScriptValue&& Other) noexcept;

    // 类型检查
    EScriptValueType GetType() const;
    bool IsNull() const;
    bool IsBool() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsObject() const;
    bool IsArray() const;
    bool IsFunction() const;

    // 值获取
    bool ToBool() const;
    int32_t ToInt() const;
    float ToFloat() const;
    double ToDouble() const;
    CString ToString() const;
    CObject* ToObject() const;

    // 数组操作
    void SetArrayElement(int32_t Index, const CScriptValue& Value);
    CScriptValue GetArrayElement(int32_t Index) const;
    int32_t GetArraySize() const;

    // 对象属性操作
    void SetProperty(const CString& Name, const CScriptValue& Value);
    CScriptValue GetProperty(const CString& Name) const;
    CArray<CString> GetPropertyNames() const;

    // 函数调用
    CScriptValue Call(const CArray<CScriptValue>& Args = {}) const;

private:
    void Clear();
    void Copy(const CScriptValue& Other);

    EScriptValueType Type;
    union
    {
        bool BoolValue;
        int32_t IntValue;
        double DoubleValue;
        void* PtrValue; // 用于字符串、对象、数组等
    };
};

/**
 * @brief 脚本执行结果
 */
struct LIBNLIB_API NScriptResult
{
    bool bSuccess;              // 是否成功
    CString ErrorMessage;       // 错误消息
    CScriptValue ReturnValue;   // 返回值
    int32_t ErrorLine;          // 错误行号
    int32_t ErrorColumn;        // 错误列号

    NScriptResult();
    NScriptResult(bool bInSuccess, const CString& InErrorMessage = "");
    NScriptResult(const CScriptValue& InReturnValue);

    bool IsSuccess() const { return bSuccess; }
    bool HasError() const { return !bSuccess; }
};

/**
 * @brief 脚本上下文接口
 */  
class LIBNLIB_API IScriptContext
{
public:
    virtual ~IScriptContext() = default;

    // 变量操作
    virtual void SetGlobal(const CString& Name, const CScriptValue& Value) = 0;
    virtual CScriptValue GetGlobal(const CString& Name) const = 0;
    virtual bool HasGlobal(const CString& Name) const = 0;

    // 对象绑定
    virtual void BindObject(const CString& Name, CObject* Object) = 0;
    virtual void UnbindObject(const CString& Name) = 0;

    // 函数绑定
    virtual void BindFunction(const CString& Name, const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Function) = 0;
    virtual void UnbindFunction(const CString& Name) = 0;

    // 模块管理
    virtual bool LoadModule(const CString& ModuleName, const CString& ModulePath) = 0;
    virtual bool UnloadModule(const CString& ModuleName) = 0;
    virtual CArray<CString> GetLoadedModules() const = 0;

    // 执行
    virtual NScriptResult Execute(const CString& Code) = 0;
    virtual NScriptResult ExecuteFile(const CString& FilePath) = 0;
    virtual NScriptResult CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args = {}) = 0;

    // 调试支持
    virtual void SetBreakpoint(const CString& FilePath, int32_t Line) = 0;
    virtual void RemoveBreakpoint(const CString& FilePath, int32_t Line) = 0;
    virtual void SetDebugMode(bool bEnabled) = 0;

    // 垃圾回收
    virtual void CollectGarbage() = 0;
    virtual size_t GetMemoryUsage() const = 0;
};

/**
 * @brief 脚本引擎接口
 */
class LIBNLIB_API IScriptEngine
{
public:
    virtual ~IScriptEngine() = default;

    // 引擎信息
    virtual EScriptLanguage GetLanguage() const = 0;
    virtual CString GetVersion() const = 0;
    virtual CString GetName() const = 0;

    // 生命周期
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;

    // 上下文管理
    virtual TSharedPtr<IScriptContext> CreateContext() = 0;
    virtual void DestroyContext(TSharedPtr<IScriptContext> Context) = 0;
    virtual TSharedPtr<IScriptContext> GetMainContext() = 0;

    // 类型系统集成
    virtual bool RegisterClass(const CString& ClassName) = 0;
    virtual bool UnregisterClass(const CString& ClassName) = 0;
    virtual bool IsClassRegistered(const CString& ClassName) const = 0;

    // 自动绑定（基于脚本元数据）
    virtual bool AutoBindClasses() = 0;
    virtual bool AutoBindClass(const CString& ClassName) = 0;

    // 脚本热重载
    virtual bool EnableHotReload(const CString& WatchDirectory) = 0;
    virtual void DisableHotReload() = 0;
    virtual bool IsHotReloadEnabled() const = 0;

    // 性能统计
    virtual void ResetStatistics() = 0;
    virtual CHashMap<CString, double> GetStatistics() const = 0;
};

/**
 * @brief 脚本引擎管理器
 */
class LIBNLIB_API NScriptEngineManager : public CObject
{
    NCLASS()
class NScriptEngineManager : public NObject
{
    GENERATED_BODY()

public:
    static NScriptEngineManager& Get();

    // 引擎注册
    void RegisterEngine(EScriptLanguage Language, TSharedPtr<IScriptEngine> Engine);
    void UnregisterEngine(EScriptLanguage Language);
    TSharedPtr<IScriptEngine> GetEngine(EScriptLanguage Language) const;

    // 全局上下文管理
    TSharedPtr<IScriptContext> CreateContext(EScriptLanguage Language);
    TSharedPtr<IScriptContext> GetMainContext(EScriptLanguage Language);

    // 多语言执行
    NScriptResult ExecuteInAllEngines(const CString& Code);
    NScriptResult ExecuteInLanguages(const CString& Code, EScriptLanguage Languages);

    // 全局函数和变量
    void SetGlobalVariable(const CString& Name, const CScriptValue& Value, EScriptLanguage Languages = EScriptLanguage::All);
    void BindGlobalFunction(const CString& Name, const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Function, EScriptLanguage Languages = EScriptLanguage::All);

    // 自动绑定所有已注册的脚本类
    bool AutoBindAllClasses(EScriptLanguage Languages = EScriptLanguage::All);

    // 统计信息
    CHashMap<EScriptLanguage, CHashMap<CString, double>> GetAllStatistics() const;

    // 生命周期
    virtual void Destroy() override;

private:
    NScriptEngineManager() = default;

    CHashMap<EScriptLanguage, TSharedPtr<IScriptEngine>> Engines;
    mutable NMutex EngineMutex;
};

/**
 * @brief 脚本绑定辅助宏
 */
#define BIND_SCRIPT_FUNCTION(FunctionName, Function) \
    Context->BindFunction(#FunctionName, [](const CArray<CScriptValue>& Args) -> CScriptValue { \
        return Function(Args); \
    })

#define BIND_SCRIPT_OBJECT(ObjectName, Object) \
    Context->BindObject(#ObjectName, Object)

/**
 * @brief 脚本值转换辅助函数
 */
namespace ScriptValueUtils
{
    // C++类型到脚本值
    template<typename TType>
    CScriptValue ToScriptValue(const TType& Value);

    // 脚本值到C++类型
    template<typename TType>
    TType FromScriptValue(const CScriptValue& Value);

    // 数组转换
    template<typename TType>
    CScriptValue ArrayToScriptValue(const CArray<TType>& Array);

    template<typename TType>
    CArray<TType> ArrayFromScriptValue(const CScriptValue& Value);

    // 映射转换
    template<typename TKey, typename TValue>
    CScriptValue MapToScriptValue(const CHashMap<TKey, TValue>& Map);

    template<typename TKey, typename TValue>
    CHashMap<TKey, TValue> MapFromScriptValue(const CScriptValue& Value);
}

/**
 * @brief 脚本回调接口
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnScriptError, const CString&, const CString&);      // FilePath, ErrorMessage
DECLARE_MULTICAST_DELEGATE_OneParam(FOnScriptFileChanged, const CString&);                  // FilePath
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnScriptFunctionCalled, const CString&, double);     // FunctionName, ExecutionTime

/**
 * @brief 全局脚本事件
 */
extern LIBNLIB_API FOnScriptError GOnScriptError;
extern LIBNLIB_API FOnScriptFileChanged GOnScriptFileChanged;  
extern LIBNLIB_API FOnScriptFunctionCalled GOnScriptFunctionCalled;

} // namespace NLib