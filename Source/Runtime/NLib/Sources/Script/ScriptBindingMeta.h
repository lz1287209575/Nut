#pragma once

/**
 * @file ScriptBindingMeta.h
 * @brief NutHeaderTools脚本绑定元数据常量定义
 *
 * 此文件定义用于NutHeaderTools处理的元标签常量，
 * 通过编译时代码生成实现脚本绑定，而非运行时注册。
 */

namespace NLib
{
/**
 * @brief 脚本绑定元标签常量
 * 这些常量用于NCLASS、NPROPERTY、NFUNCTION的meta参数
 */
namespace ScriptMeta
{
// === 类级别标签 ===

/**
 * @brief 标记类可在脚本中创建实例
 * 用法: NCLASS(meta=ScriptCreatable)
 */
constexpr const char* ScriptCreatable = "ScriptCreatable";

/**
 * @brief 标记类在脚本中可见（可访问静态成员）
 * 用法: NCLASS(meta=ScriptVisible)
 */
constexpr const char* ScriptVisible = "ScriptVisible";

// === 属性级别标签 ===

/**
 * @brief 标记属性可在脚本中读取
 * 用法: NPROPERTY(meta=ScriptReadable)
 */
constexpr const char* ScriptReadable = "ScriptReadable";

/**
 * @brief 标记属性可在脚本中写入
 * 用法: NPROPERTY(meta=ScriptWritable)
 */
constexpr const char* ScriptWritable = "ScriptWritable";

// === 函数级别标签 ===

/**
 * @brief 标记函数可在脚本中调用
 * 用法: NFUNCTION(meta=ScriptCallable)
 */
constexpr const char* ScriptCallable = "ScriptCallable";

/**
 * @brief 标记函数为静态函数
 * 用法: NFUNCTION(meta=ScriptStatic)
 */
constexpr const char* ScriptStatic = "ScriptStatic";

/**
 * @brief 标记函数为事件（可在脚本中重写）
 * 用法: NFUNCTION(meta=ScriptEvent)
 */
constexpr const char* ScriptEvent = "ScriptEvent";

// === 通用标签 ===

/**
 * @brief 自定义脚本中的名称
 * 用法: NCLASS(meta=(ScriptVisible, ScriptName="PlayerCharacter"))
 */
constexpr const char* ScriptName = "ScriptName";

/**
 * @brief 指定支持的脚本语言
 * 用法: NFUNCTION(meta=(ScriptCallable, ScriptLanguages="Lua,TypeScript,Python"))
 */
constexpr const char* ScriptLanguages = "ScriptLanguages";

/**
 * @brief 脚本分类（用于组织绑定代码）
 * 用法: NCLASS(meta=(ScriptVisible, ScriptCategory="GamePlay"))
 */
constexpr const char* ScriptCategory = "ScriptCategory";

/**
 * @brief 脚本描述（用于生成文档）
 * 用法: NFUNCTION(meta=(ScriptCallable, ScriptDescription="Deals damage to the player"))
 */
constexpr const char* ScriptDescription = "ScriptDescription";
} // namespace ScriptMeta

/**
 * @brief 脚本语言枚举（用于NutHeaderTools代码生成）
 */
enum class EScriptLanguageTarget : uint8_t
{
	Lua = 1 << 0,
	LuaForge = 1 << 1,
	TypeScript = 1 << 2,
	Python = 1 << 3,
	CSharp = 1 << 4,
	NBP = 1 << 5,

	// 组合标志
	All = Lua | LuaForge | TypeScript | Python | CSharp | NBP,
	Scripting = Lua | LuaForge | TypeScript | Python | CSharp,
	Visual = NBP
};

} // namespace NLib

// === 便利宏定义 ===

/**
 * @brief 便利宏用于常见的脚本绑定模式
 * 这些宏只是元标签的简写，实际处理仍由NutHeaderTools完成
 */

/**
 * @brief 快速声明脚本可创建的类
 * 展开为: NCLASS(meta=(ScriptCreatable, ScriptVisible))
 */
#define SCRIPT_CLASS() meta = (ScriptCreatable, ScriptVisible)

/**
 * @brief 快速声明脚本可见的类
 * 展开为: NCLASS(meta=ScriptVisible)
 */
#define SCRIPT_VISIBLE_CLASS() meta = ScriptVisible

/**
 * @brief 快速声明脚本属性（读写）
 * 展开为: NPROPERTY(meta=(ScriptReadable, ScriptWritable))
 */
#define SCRIPT_PROPERTY() meta = (ScriptReadable, ScriptWritable)

/**
 * @brief 快速声明脚本只读属性
 * 展开为: NPROPERTY(meta=ScriptReadable)
 */
#define SCRIPT_READONLY_PROPERTY() meta = ScriptReadable

/**
 * @brief 快速声明脚本函数
 * 展开为: NFUNCTION(meta=ScriptCallable)
 */
#define SCRIPT_FUNCTION() meta = ScriptCallable

/**
 * @brief 快速声明脚本静态函数
 * 展开为: NFUNCTION(meta=(ScriptCallable, ScriptStatic))
 */
#define SCRIPT_STATIC_FUNCTION() meta = (ScriptCallable, ScriptStatic)

/**
 * @brief 快速声明脚本事件
 * 展开为: NFUNCTION(meta=(ScriptCallable, ScriptEvent))
 */
#define SCRIPT_EVENT() meta = (ScriptCallable, ScriptEvent)

/*
使用示例：

```cpp
// 方式1: 直接使用元标签
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

    NFUNCTION(BlueprintCallable, meta=(ScriptCallable, ScriptStatic))
    static NGamePlayer* CreatePlayer(const CString& PlayerName);
};

// 方式2: 使用便利宏
NCLASS(SCRIPT_CLASS())
class NInventoryItem : public NObject
{
    GENERATED_BODY()

public:
    NPROPERTY(BlueprintReadWrite, SCRIPT_PROPERTY())
    CString ItemName;

    NPROPERTY(BlueprintReadOnly, SCRIPT_READONLY_PROPERTY())
    int32_t ItemCount;

    NFUNCTION(BlueprintCallable, SCRIPT_FUNCTION())
    void UseItem();
};
```

NutHeaderTools将处理这些元标签并生成：
1. NGamePlayer.script.generate.h - 绑定信息结构体
2. NGamePlayer.script.generate.cpp - 绑定注册代码
3. Lua/GamePlayer.lua - Lua绑定文件
4. TypeScript/GamePlayer.d.ts - TypeScript类型定义
5. Python/game_player.py - Python绑定文件
6. CSharp/GamePlayer.cs - C#绑定类

运行时系统只需要加载这些预生成的绑定文件即可。
*/