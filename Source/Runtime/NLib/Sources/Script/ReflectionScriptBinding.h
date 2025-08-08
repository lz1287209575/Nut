#pragma once

#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Reflection/ReflectionRegistry.h"
#include "Reflection/ReflectionStructures.h"
#include "ScriptEngine.h"

namespace NLib
{
/**
 * @brief 脚本绑定信息结构
 * 由NutHeaderTools生成，包含从meta标签解析的绑定信息
 */
struct SScriptBindingInfo
{
	bool bScriptCreatable = false;                              // meta=(ScriptCreatable)
	bool bScriptVisible = false;                                // meta=(ScriptVisible)
	bool bScriptReadable = false;                               // meta=(ScriptReadable)
	bool bScriptWritable = false;                               // meta=(ScriptWritable)
	bool bScriptCallable = false;                               // meta=(ScriptCallable)
	bool bScriptStatic = false;                                 // meta=(ScriptStatic)
	bool bScriptEvent = false;                                  // meta=(ScriptEvent)
	CString ScriptName;                                         // meta=(ScriptName="CustomName")
	CString ScriptCategory;                                     // meta=(ScriptCategory="MyCategory")
	TArray<EScriptLanguage, CMemoryManager> SupportedLanguages; // meta=(Languages="Lua,TypeScript,Python")

	/**
	 * @brief 检查是否支持指定语言
	 */
	bool SupportsLanguage(EScriptLanguage Language) const
	{
		return SupportedLanguages.IsEmpty() || SupportedLanguages.Contains(Language);
	}

	/**
	 * @brief 检查是否应该绑定到脚本
	 */
	bool ShouldBind() const
	{
		return bScriptCreatable || bScriptVisible || bScriptReadable || bScriptWritable || bScriptCallable;
	}
};

/**
 * @brief 脚本绑定生成器基类
 * 每种脚本语言实现自己的绑定代码生成器
 */
class IScriptBindingGenerator
{
public:
	virtual ~IScriptBindingGenerator() = default;

	/**
	 * @brief 获取支持的脚本语言
	 */
	virtual EScriptLanguage GetLanguage() const = 0;

	/**
	 * @brief 生成类绑定代码
	 */
	virtual CString GenerateClassBinding(const SClassReflection* ClassReflection,
	                                     const SScriptBindingInfo& BindingInfo) = 0;

	/**
	 * @brief 生成函数绑定代码
	 */
	virtual CString GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
	                                        const SScriptBindingInfo& BindingInfo,
	                                        const CString& ClassName = CString()) = 0;

	/**
	 * @brief 生成属性绑定代码
	 */
	virtual CString GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
	                                        const SScriptBindingInfo& BindingInfo,
	                                        const CString& ClassName) = 0;

	/**
	 * @brief 生成枚举绑定代码
	 */
	virtual CString GenerateEnumBinding(const SEnumReflection* EnumReflection,
	                                    const SScriptBindingInfo& BindingInfo) = 0;

	/**
	 * @brief 生成完整的绑定文件
	 */
	virtual CString GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) = 0;
};

/**
 * @brief Lua绑定生成器
 */
class CLuaBindingGenerator : public IScriptBindingGenerator
{
public:
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Lua;
	}

	CString GenerateClassBinding(const SClassReflection* ClassReflection,
	                             const SScriptBindingInfo& BindingInfo) override;

	CString GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName = CString()) override;

	CString GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName) override;

	CString GenerateEnumBinding(const SEnumReflection* EnumReflection, const SScriptBindingInfo& BindingInfo) override;

	CString GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) override;

private:
	CString ConvertTypeToLua(const std::type_info& TypeInfo) const;
	CString GenerateLuaUserdata(const SClassReflection* ClassReflection) const;
	CString GenerateLuaMetatable(const SClassReflection* ClassReflection, const SScriptBindingInfo& BindingInfo) const;
};

/**
 * @brief TypeScript绑定生成器
 */
class CTypeScriptBindingGenerator : public IScriptBindingGenerator
{
public:
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::TypeScript;
	}

	CString GenerateClassBinding(const SClassReflection* ClassReflection,
	                             const SScriptBindingInfo& BindingInfo) override;

	CString GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName = CString()) override;

	CString GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName) override;

	CString GenerateEnumBinding(const SEnumReflection* EnumReflection, const SScriptBindingInfo& BindingInfo) override;

	CString GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) override;

private:
	CString ConvertTypeToTypeScript(const std::type_info& TypeInfo) const;
	CString GenerateTypeDefinition(const SClassReflection* ClassReflection,
	                               const SScriptBindingInfo& BindingInfo) const;
	CString GenerateInterfaceDefinition(const SClassReflection* ClassReflection) const;
};

/**
 * @brief Python绑定生成器
 */
class CPythonBindingGenerator : public IScriptBindingGenerator
{
public:
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::Python;
	}

	CString GenerateClassBinding(const SClassReflection* ClassReflection,
	                             const SScriptBindingInfo& BindingInfo) override;

	CString GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName = CString()) override;

	CString GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName) override;

	CString GenerateEnumBinding(const SEnumReflection* EnumReflection, const SScriptBindingInfo& BindingInfo) override;

	CString GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) override;

private:
	CString ConvertTypeToPython(const std::type_info& TypeInfo) const;
	CString GeneratePythonClass(const SClassReflection* ClassReflection, const SScriptBindingInfo& BindingInfo) const;
	CString GeneratePythonStub(const SClassReflection* ClassReflection) const;
	CString GenerateTypeStub(const SClassReflection* ClassReflection, const SScriptBindingInfo& BindingInfo) const;
	CString GeneratePyiFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) const;
};

/**
 * @brief C#绑定生成器
 */
class CCSharpBindingGenerator : public IScriptBindingGenerator
{
public:
	EScriptLanguage GetLanguage() const override
	{
		return EScriptLanguage::CSharp;
	}

	CString GenerateClassBinding(const SClassReflection* ClassReflection,
	                             const SScriptBindingInfo& BindingInfo) override;

	CString GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName = CString()) override;

	CString GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
	                                const SScriptBindingInfo& BindingInfo,
	                                const CString& ClassName) override;

	CString GenerateEnumBinding(const SEnumReflection* EnumReflection, const SScriptBindingInfo& BindingInfo) override;

	CString GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) override;

private:
	CString ConvertTypeToCSharp(const std::type_info& TypeInfo) const;
	CString GenerateCSharpClass(const SClassReflection* ClassReflection, const SScriptBindingInfo& BindingInfo) const;
	CString GeneratePInvokeDeclarations(const SClassReflection* ClassReflection) const;
};

/**
 * @brief 脚本绑定注册表
 * 管理所有生成的绑定信息
 */
class CScriptBindingRegistry
{
public:
	/**
	 * @brief 获取单例实例
	 */
	static CScriptBindingRegistry& GetInstance();

	/**
	 * @brief 注册绑定生成器
	 */
	void RegisterGenerator(EScriptLanguage Language, TSharedPtr<IScriptBindingGenerator> Generator);

	/**
	 * @brief 注册类的脚本绑定信息（由NutHeaderTools生成的代码调用）
	 */
	void RegisterClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册函数的脚本绑定信息
	 */
	void RegisterFunctionBinding(const char* ClassName,
	                             const char* FunctionName,
	                             const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册属性的脚本绑定信息
	 */
	void RegisterPropertyBinding(const char* ClassName,
	                             const char* PropertyName,
	                             const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册枚举的脚本绑定信息
	 */
	void RegisterEnumBinding(const char* EnumName, const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 获取类的绑定信息
	 */
	const SScriptBindingInfo* GetClassBindingInfo(const char* ClassName) const;

	/**
	 * @brief 获取函数的绑定信息
	 */
	const SScriptBindingInfo* GetFunctionBindingInfo(const char* ClassName, const char* FunctionName) const;

	/**
	 * @brief 获取属性的绑定信息
	 */
	const SScriptBindingInfo* GetPropertyBindingInfo(const char* ClassName, const char* PropertyName) const;

	/**
	 * @brief 生成指定语言的绑定代码
	 */
	CString GenerateBindingCode(EScriptLanguage Language) const;

	/**
	 * @brief 生成所有语言的绑定代码
	 */
	void GenerateAllBindings(const CString& OutputDirectory) const;

	/**
	 * @brief 获取所有支持脚本绑定的类
	 */
	TArray<const SClassReflection*, CMemoryManager> GetScriptBindableClasses() const;

private:
	CScriptBindingRegistry() = default;

	THashMap<EScriptLanguage, TSharedPtr<IScriptBindingGenerator>, CMemoryManager> Generators;
	THashMap<CString, SScriptBindingInfo, CMemoryManager> ClassBindings;
	THashMap<CString, SScriptBindingInfo, CMemoryManager> FunctionBindings; // "ClassName::FunctionName"
	THashMap<CString, SScriptBindingInfo, CMemoryManager> PropertyBindings; // "ClassName::PropertyName"
	THashMap<CString, SScriptBindingInfo, CMemoryManager> EnumBindings;
};

/**
 * @brief 脚本绑定注册辅助宏（由NutHeaderTools生成）
 */
#define REGISTER_SCRIPT_CLASS_BINDING(ClassName, BindingInfo)                                                          \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct ClassName##ScriptBindingRegistrar                                                                           \
	{                                                                                                                  \
		ClassName##ScriptBindingRegistrar()                                                                            \
		{                                                                                                              \
			NLib::CScriptBindingRegistry::GetInstance().RegisterClassBinding(#ClassName, BindingInfo);                 \
		}                                                                                                              \
	};                                                                                                                 \
	static ClassName##ScriptBindingRegistrar ClassName##_script_binding_registrar_;                                    \
	}

#define REGISTER_SCRIPT_FUNCTION_BINDING(ClassName, FunctionName, BindingInfo)                                         \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct ClassName##_##FunctionName##ScriptBindingRegistrar                                                          \
	{                                                                                                                  \
		ClassName##_##FunctionName##ScriptBindingRegistrar()                                                           \
		{                                                                                                              \
			NLib::CScriptBindingRegistry::GetInstance().RegisterFunctionBinding(                                       \
			    #ClassName, #FunctionName, BindingInfo);                                                               \
		}                                                                                                              \
	};                                                                                                                 \
	static ClassName##_##FunctionName##ScriptBindingRegistrar ClassName##_##FunctionName##_script_binding_registrar_;  \
	}

#define REGISTER_SCRIPT_PROPERTY_BINDING(ClassName, PropertyName, BindingInfo)                                         \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct ClassName##_##PropertyName##ScriptBindingRegistrar                                                          \
	{                                                                                                                  \
		ClassName##_##PropertyName##ScriptBindingRegistrar()                                                           \
		{                                                                                                              \
			NLib::CScriptBindingRegistry::GetInstance().RegisterPropertyBinding(                                       \
			    #ClassName, #PropertyName, BindingInfo);                                                               \
		}                                                                                                              \
	};                                                                                                                 \
	static ClassName##_##PropertyName##ScriptBindingRegistrar ClassName##_##PropertyName##_script_binding_registrar_;  \
	}

} // namespace NLib

/* NutHeaderTools处理示例：

输入的C++代码：
```cpp
NCLASS(meta=(ScriptCreatable, ScriptVisible))
class NGamePlayer : public NObject
{
    GENERATED_BODY()

public:
    NPROPERTY(BlueprintReadWrite, meta=(ScriptReadable, ScriptWritable, ScriptName="PlayerName"))
    CString Name;

    NPROPERTY(BlueprintReadOnly, meta=(ScriptReadable))
    int32_t Health;

    NFUNCTION(BlueprintCallable, meta=(ScriptCallable, Languages="Lua,TypeScript,Python"))
    void TakeDamage(int32_t Amount);

    NFUNCTION(BlueprintCallable, meta=(ScriptCallable, ScriptStatic))
    static NGamePlayer* CreatePlayer(const CString& PlayerName);
};
```

NutHeaderTools生成的绑定注册代码：
```cpp
// 在 NGamePlayer.script.generate.cpp 中：

SScriptBindingInfo NGamePlayer_ClassBinding = {
    .bScriptCreatable = true,
    .bScriptVisible = true,
    .SupportedLanguages = {} // 空表示支持所有语言
};

SScriptBindingInfo NGamePlayer_Name_PropertyBinding = {
    .bScriptReadable = true,
    .bScriptWritable = true,
    .ScriptName = TEXT("PlayerName"),
    .SupportedLanguages = {}
};

SScriptBindingInfo NGamePlayer_TakeDamage_FunctionBinding = {
    .bScriptCallable = true,
    .SupportedLanguages = {EScriptLanguage::Lua, EScriptLanguage::TypeScript, EScriptLanguage::Python}
};

REGISTER_SCRIPT_CLASS_BINDING(NGamePlayer, NGamePlayer_ClassBinding);
REGISTER_SCRIPT_PROPERTY_BINDING(NGamePlayer, Name, NGamePlayer_Name_PropertyBinding);
REGISTER_SCRIPT_FUNCTION_BINDING(NGamePlayer, TakeDamage, NGamePlayer_TakeDamage_FunctionBinding);
```

生成的TypeScript绑定：
```typescript
declare class GamePlayer {
    PlayerName: string;
    readonly Health: number;

    TakeDamage(Amount: number): void;
    static CreatePlayer(PlayerName: string): GamePlayer;
}
```

生成的Lua绑定：
```lua
-- GamePlayer类绑定
local GamePlayer = {}
GamePlayer.__index = GamePlayer

function GamePlayer.new()
    return setmetatable({}, GamePlayer)
end

function GamePlayer:TakeDamage(amount)
    -- 调用C++函数
end

-- 注册到Lua全局
_G.GamePlayer = GamePlayer
```

*/