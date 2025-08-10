#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Memory/Memory.h"
#include "Reflection/ReflectionRegistry.h"
#include "ScriptEngine.h"

namespace NLib
{
/**
 * @brief 脚本绑定信息结构（由NutHeaderTools生成）
 * 这个结构体包含从meta标签解析的所有绑定信息
 */
struct SScriptBindingInfo
{
	// 类级别标志
	bool bScriptCreatable = false; // meta=ScriptCreatable
	bool bScriptVisible = false;   // meta=ScriptVisible

	// 属性级别标志
	bool bScriptReadable = false; // meta=ScriptReadable
	bool bScriptWritable = false; // meta=ScriptWritable

	// 函数级别标志
	bool bScriptCallable = false; // meta=ScriptCallable
	bool bScriptStatic = false;   // meta=ScriptStatic
	bool bScriptEvent = false;    // meta=ScriptEvent

	// 通用属性
	CString ScriptName;        // meta=(ScriptName="CustomName")
	CString ScriptCategory;    // meta=(ScriptCategory="GamePlay")
	CString ScriptDescription; // meta=(ScriptDescription="Description")

	// 支持的语言（位标志）
	uint32_t SupportedLanguages = 0; // 由ScriptLanguages解析而来

	/**
	 * @brief 检查是否支持指定语言
	 */
	bool SupportsLanguage(EScriptLanguage Language) const
	{
		if (SupportedLanguages == 0) // 0表示支持所有语言
			return true;
		return (SupportedLanguages & (1 << static_cast<uint32_t>(Language))) != 0;
	}

	/**
	 * @brief 检查是否需要绑定到脚本
	 */
	bool ShouldBind() const
	{
		return bScriptCreatable || bScriptVisible || bScriptReadable || bScriptWritable || bScriptCallable;
	}

	/**
	 * @brief 相等比较操作符
	 */
	bool operator==(const SScriptBindingInfo& Other) const
	{
		return bScriptCreatable == Other.bScriptCreatable &&
		       bScriptVisible == Other.bScriptVisible &&
		       bScriptReadable == Other.bScriptReadable &&
		       bScriptWritable == Other.bScriptWritable &&
		       bScriptCallable == Other.bScriptCallable &&
		       bScriptStatic == Other.bScriptStatic &&
		       bScriptEvent == Other.bScriptEvent &&
		       ScriptName == Other.ScriptName &&
		       ScriptCategory == Other.ScriptCategory &&
		       ScriptDescription == Other.ScriptDescription &&
		       SupportedLanguages == Other.SupportedLanguages;
	}
};

} // namespace NLib

/**
 * @brief std::hash specialization for SScriptBindingInfo
 */
namespace std
{
template <>
struct hash<NLib::SScriptBindingInfo>
{
	size_t operator()(const NLib::SScriptBindingInfo& Info) const noexcept
	{
		size_t h1 = hash<bool>{}(Info.bScriptCreatable);
		size_t h2 = hash<bool>{}(Info.bScriptVisible);
		size_t h3 = hash<bool>{}(Info.bScriptReadable);
		size_t h4 = hash<bool>{}(Info.bScriptWritable);
		size_t h5 = hash<bool>{}(Info.bScriptCallable);
		size_t h6 = hash<NLib::CString>{}(Info.ScriptName);
		size_t h7 = hash<uint32_t>{}(Info.SupportedLanguages);

		// Combine hashes using XOR and bit shifting
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6);
	}
};
} // namespace std

namespace NLib
{

/**
 * @brief 脚本绑定加载器
 * 负责加载NutHeaderTools生成的脚本绑定信息和代码
 */
class CScriptBindingLoader
{
public:
	/**
	 * @brief 获取单例实例
	 */
	static CScriptBindingLoader& GetInstance();

	// 禁用拷贝构造和赋值
	CScriptBindingLoader(const CScriptBindingLoader&) = delete;
	CScriptBindingLoader& operator=(const CScriptBindingLoader&) = delete;

public:
	// === 绑定信息注册（由生成的代码调用） ===

	/**
	 * @brief 注册类的脚本绑定信息
	 * @param ClassName 类名
	 * @param BindingInfo 绑定信息
	 */
	void RegisterClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册函数的脚本绑定信息
	 * @param ClassName 类名
	 * @param FunctionName 函数名
	 * @param BindingInfo 绑定信息
	 */
	void RegisterFunctionBinding(const char* ClassName,
	                             const char* FunctionName,
	                             const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册属性的脚本绑定信息
	 * @param ClassName 类名
	 * @param PropertyName 属性名
	 * @param BindingInfo 绑定信息
	 */
	void RegisterPropertyBinding(const char* ClassName,
	                             const char* PropertyName,
	                             const SScriptBindingInfo& BindingInfo);

	/**
	 * @brief 注册枚举的脚本绑定信息
	 * @param EnumName 枚举名
	 * @param BindingInfo 绑定信息
	 */
	void RegisterEnumBinding(const char* EnumName, const SScriptBindingInfo& BindingInfo);

public:
	// === 绑定信息查询 ===

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
	 * @brief 获取枚举的绑定信息
	 */
	const SScriptBindingInfo* GetEnumBindingInfo(const char* EnumName) const;

public:
	// === 脚本绑定加载 ===

	/**
	 * @brief 加载指定语言的脚本绑定
	 * @param Language 脚本语言
	 * @param Context 脚本上下文
	 * @param BindingDirectory 绑定文件目录
	 * @return 是否加载成功
	 */
	bool LoadScriptBindings(EScriptLanguage Language,
	                        TSharedPtr<CScriptContext> Context,
	                        const CString& BindingDirectory);

	/**
	 * @brief 加载Lua绑定
	 */
	bool LoadLuaBindings(TSharedPtr<CScriptContext> Context, const CString& BindingDirectory);

	/**
	 * @brief 加载TypeScript类型定义
	 */
	bool LoadTypeScriptBindings(TSharedPtr<CScriptContext> Context, const CString& BindingDirectory);

	/**
	 * @brief 获取所有支持脚本绑定的类
	 */
	TArray<const SClassReflection*, CMemoryManager> GetScriptBindableClasses() const;

	/**
	 * @brief 获取支持指定语言的类
	 */
	TArray<const SClassReflection*, CMemoryManager> GetClassesForLanguage(EScriptLanguage Language) const;

public:
	// === 运行时脚本调用支持 ===

	/**
	 * @brief 创建脚本对象
	 * @param ClassName 类名
	 * @param Args 构造函数参数
	 * @return 创建的对象
	 */
	NObject* CreateScriptObject(const char* ClassName, const TArray<TSharedPtr<NScriptValue>, CMemoryManager>& Args = {});

	/**
	 * @brief 调用脚本函数
	 * @param Object 对象实例（静态函数传nullptr）
	 * @param ClassName 类名
	 * @param FunctionName 函数名
	 * @param Args 函数参数
	 * @return 调用结果
	 */
	TSharedPtr<NScriptValue> CallScriptFunction(NObject* Object,
	                                            const char* ClassName,
	                                            const char* FunctionName,
	                                            const TArray<TSharedPtr<NScriptValue>, CMemoryManager>& Args = {});

	/**
	 * @brief 获取脚本属性值
	 * @param Object 对象实例
	 * @param ClassName 类名
	 * @param PropertyName 属性名
	 * @return 属性值
	 */
	TSharedPtr<NScriptValue> GetScriptProperty(NObject* Object, const char* ClassName, const char* PropertyName);

	/**
	 * @brief 设置脚本属性值
	 * @param Object 对象实例
	 * @param ClassName 类名
	 * @param PropertyName 属性名
	 * @param Value 新值
	 * @return 是否设置成功
	 */
	bool SetScriptProperty(NObject* Object, const char* ClassName, const char* PropertyName, const TSharedPtr<NScriptValue>& Value);

public:
	// === 调试和统计 ===

	/**
	 * @brief 打印所有绑定信息
	 */
	void PrintBindingInfo() const;

	/**
	 * @brief 绑定统计信息结构体
	 */
	struct SBindingStats
	{
		size_t ClassCount = 0;
		size_t FunctionCount = 0;
		size_t PropertyCount = 0;
		size_t EnumCount = 0;
	};

	/**
	 * @brief 获取绑定统计信息
	 */
	SBindingStats GetBindingStats() const;

private:
	CScriptBindingLoader() = default;
	~CScriptBindingLoader() = default;

	// 内部辅助方法
	CString GenerateBindingKey(const char* ClassName, const char* MemberName) const;
	bool LoadBindingFile(const CString& FilePath, EScriptLanguage Language, TSharedPtr<CScriptContext> Context);

private:
	// 绑定信息存储
	THashMap<CString, SScriptBindingInfo, std::hash<CString>, std::equal_to<CString>, CMemoryManager> ClassBindings;
	THashMap<CString, SScriptBindingInfo, std::hash<CString>, std::equal_to<CString>, CMemoryManager> FunctionBindings; // "ClassName::FunctionName"
	THashMap<CString, SScriptBindingInfo, std::hash<CString>, std::equal_to<CString>, CMemoryManager> PropertyBindings; // "ClassName::PropertyName"
	THashMap<CString, SScriptBindingInfo, std::hash<CString>, std::equal_to<CString>, CMemoryManager> EnumBindings;

	mutable std::mutex BindingMutex; // 线程安全锁
};

// === 全局便利函数 ===

/**
 * @brief 获取脚本绑定加载器实例
 */
inline CScriptBindingLoader& GetScriptBindingLoader()
{
	return CScriptBindingLoader::GetInstance();
}

/**
 * @brief 注册类脚本绑定信息的便利函数
 */
inline void RegisterScriptClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo)
{
	GetScriptBindingLoader().RegisterClassBinding(ClassName, BindingInfo);
}

/**
 * @brief 创建脚本对象的便利函数
 */
inline NObject* CreateScriptObject(const char* ClassName)
{
	return GetScriptBindingLoader().CreateScriptObject(ClassName);
}

} // namespace NLib

// === 注册宏（由NutHeaderTools生成的代码使用） ===

/**
 * @brief 注册类脚本绑定信息的宏
 * 这个宏会被NutHeaderTools在.script.generate.cpp文件中使用
 */
#define REGISTER_SCRIPT_CLASS_BINDING(ClassName, BindingInfo)                                                          \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct ClassName##ScriptBindingRegistrar                                                                           \
	{                                                                                                                  \
		ClassName##ScriptBindingRegistrar()                                                                            \
		{                                                                                                              \
			NLib::GetScriptBindingLoader().RegisterClassBinding(#ClassName, BindingInfo);                              \
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
			NLib::GetScriptBindingLoader().RegisterFunctionBinding(#ClassName, #FunctionName, BindingInfo);            \
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
			NLib::GetScriptBindingLoader().RegisterPropertyBinding(#ClassName, #PropertyName, BindingInfo);            \
		}                                                                                                              \
	};                                                                                                                 \
	static ClassName##_##PropertyName##ScriptBindingRegistrar ClassName##_##PropertyName##_script_binding_registrar_;  \
	}

#define REGISTER_SCRIPT_ENUM_BINDING(EnumName, BindingInfo)                                                            \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct EnumName##ScriptBindingRegistrar                                                                            \
	{                                                                                                                  \
		EnumName##ScriptBindingRegistrar()                                                                             \
		{                                                                                                              \
			NLib::GetScriptBindingLoader().RegisterEnumBinding(#EnumName, BindingInfo);                                \
		}                                                                                                              \
	};                                                                                                                 \
	static EnumName##ScriptBindingRegistrar EnumName##_script_binding_registrar_;                                      \
	}

/*
NutHeaderTools代码生成示例：

输入的C++代码：
```cpp
NCLASS(meta=(ScriptCreatable, ScriptVisible, ScriptName="Player"))
class NGamePlayer : public NObject
{
    GENERATED_BODY()

public:
    NPROPERTY(BlueprintReadWrite, meta=(ScriptReadable, ScriptWritable, ScriptName="PlayerName"))
    CString Name;

    NPROPERTY(BlueprintReadOnly, meta=ScriptReadable)
    int32_t Health;

    NFUNCTION(BlueprintCallable, meta=(ScriptCallable, ScriptLanguages="Lua,TypeScript"))
    void TakeDamage(int32_t Amount);
};
```

生成的NGamePlayer.script.generate.cpp：
```cpp
#include "ScriptBindingLoader.h"

// 绑定信息定义
static const NLib::SScriptBindingInfo NGamePlayer_ClassBinding = {
    .bScriptCreatable = true,
    .bScriptVisible = true,
    .ScriptName = TEXT("Player"),
    .SupportedLanguages = 0 // 0表示支持所有语言
};

static const NLib::SScriptBindingInfo NGamePlayer_Name_PropertyBinding = {
    .bScriptReadable = true,
    .bScriptWritable = true,
    .ScriptName = TEXT("PlayerName"),
    .SupportedLanguages = 0
};

static const NLib::SScriptBindingInfo NGamePlayer_TakeDamage_FunctionBinding = {
    .bScriptCallable = true,
    .SupportedLanguages = (1 << static_cast<uint32_t>(NLib::EScriptLanguage::Lua)) |
                         (1 << static_cast<uint32_t>(NLib::EScriptLanguage::TypeScript))
};

// 自动注册
REGISTER_SCRIPT_CLASS_BINDING(NGamePlayer, NGamePlayer_ClassBinding);
REGISTER_SCRIPT_PROPERTY_BINDING(NGamePlayer, Name, NGamePlayer_Name_PropertyBinding);
REGISTER_SCRIPT_FUNCTION_BINDING(NGamePlayer, TakeDamage, NGamePlayer_TakeDamage_FunctionBinding);
```

生成的Lua绑定文件Player.lua：
```lua
-- Player class binding (auto-generated)
local Player = {}
Player.__index = Player

function Player.new()
    local instance = setmetatable({}, Player)
    instance._cppObject = NLib.CreateObject("NGamePlayer")
    return instance
end

function Player:GetPlayerName()
    return NLib.GetProperty(self._cppObject, "Name")
end

function Player:SetPlayerName(value)
    NLib.SetProperty(self._cppObject, "Name", value)
end

function Player:GetHealth()
    return NLib.GetProperty(self._cppObject, "Health")
end

function Player:TakeDamage(amount)
    return NLib.CallMethod(self._cppObject, "TakeDamage", amount)
end

-- Register globally
_G.Player = Player
```

生成的TypeScript类型定义Player.d.ts：
```typescript
// Player class types (auto-generated)
declare class Player {
    constructor();
    PlayerName: string;
    readonly Health: number;
    TakeDamage(Amount: number): void;
}

declare const _G: {
    Player: typeof Player;
};
```
*/