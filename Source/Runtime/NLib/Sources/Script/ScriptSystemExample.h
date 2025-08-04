#pragma once

/**
 * @file ScriptSystemExample.h
 * @brief NLib脚本系统使用示例
 *
 * 此文件展示如何使用NLib的脚本绑定系统，包括：
 * 1. 如何在C++类中添加脚本绑定标记
 * 2. 如何初始化和使用脚本引擎
 * 3. 如何加载生成的脚本绑定
 * 4. 如何在脚本中调用C++代码
 */

#include "Core/Object.h"
#include "LuaEngine.h"
#include "Macros/ReflectionMacros.h"
#include "ScriptBindingLoader.h"

#include "ScriptSystemExample.generate.h"

namespace NLib::Examples
{
/**
 * @brief 游戏玩家类示例
 *
 * 展示如何使用NCLASS meta标签来启用脚本绑定
 */
NCLASS(meta = (ScriptCreatable, ScriptVisible, ScriptName = "Player", ScriptCategory = "Gameplay"))
class NGamePlayer : public NObject
{
	GENERATED_BODY()

public:
	NGamePlayer();
	virtual ~NGamePlayer() = default;

	// === 脚本可访问的属性 ===

	/**
	 * @brief 玩家名称（脚本可读写）
	 */
	NPROPERTY(BlueprintReadWrite, meta = (ScriptReadable, ScriptWritable, ScriptName = "Name"))
	TString PlayerName;

	/**
	 * @brief 玩家血量（脚本只读）
	 */
	NPROPERTY(BlueprintReadOnly, meta = (ScriptReadable, ScriptName = "HP"))
	int32_t Health = 100;

	/**
	 * @brief 玩家等级（脚本可读写，仅支持Lua和TypeScript）
	 */
	NPROPERTY(BlueprintReadWrite, meta = (ScriptReadable, ScriptWritable, ScriptLanguages = "Lua,TypeScript"))
	int32_t Level = 1;

	// === 脚本可调用的方法 ===

	/**
	 * @brief 受到伤害（脚本可调用）
	 */
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable, ScriptName = "TakeDamage"))
	void ReceiveDamage(int32_t Amount);

	/**
	 * @brief 治疗（脚本可调用，仅支持Lua）
	 */
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable, ScriptLanguages = "Lua"))
	void Heal(int32_t Amount);

	/**
	 * @brief 获取玩家信息（脚本可调用）
	 */
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable))
	TString GetPlayerInfo() const;

	/**
	 * @brief 创建玩家实例（静态函数，脚本可调用）
	 */
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable, ScriptStatic))
	static NGamePlayer* CreatePlayer(const TString& Name, int32_t InitialLevel = 1);

	// === 脚本事件 ===

	/**
	 * @brief 玩家死亡事件（可在脚本中重写）
	 */
	NFUNCTION(BlueprintImplementableEvent, meta = (ScriptEvent))
	virtual void OnPlayerDeath();

private:
	void CheckDeath();
};

/**
 * @brief 游戏道具类示例
 */
NCLASS(meta = (ScriptCreatable, ScriptVisible))
class NGameItem : public NObject
{
	GENERATED_BODY()

public:
	NGameItem() = default;
	virtual ~NGameItem() = default;

	NPROPERTY(BlueprintReadWrite, meta = (ScriptReadable, ScriptWritable))
	TString ItemName;

	NPROPERTY(BlueprintReadWrite, meta = (ScriptReadable, ScriptWritable))
	int32_t ItemCount = 1;

	NFUNCTION(BlueprintCallable, meta = (ScriptCallable))
	void UseItem();

	NFUNCTION(BlueprintCallable, meta = (ScriptCallable))
	bool CanUse() const;
};

/**
 * @brief 游戏状态枚举示例
 */
NENUM(meta = (ScriptVisible, ScriptName = "GameState"))
enum class EGameState : uint8_t
{
	Menu,
	Playing,
	Paused,
	GameOver
};

/**
 * @brief 脚本系统使用示例类
 * 展示如何初始化和使用脚本系统
 */
class CScriptSystemExample
{
public:
	/**
	 * @brief 初始化脚本系统示例
	 * @param BindingDirectory 脚本绑定文件目录
	 * @return 是否初始化成功
	 */
	static bool InitializeScriptSystem(const TString& BindingDirectory = TEXT("Generated/ScriptBindings"));

	/**
	 * @brief 运行Lua脚本示例
	 */
	static void RunLuaExample();

	/**
	 * @brief 演示脚本与C++对象的交互
	 */
	static void DemonstrateScriptObjectInteraction();

	/**
	 * @brief 演示如何在脚本中创建C++对象
	 */
	static void DemonstrateScriptObjectCreation();

	/**
	 * @brief 清理脚本系统
	 */
	static void CleanupScriptSystem();

private:
	static TSharedPtr<CLuaScriptEngine> LuaEngine;
	static TSharedPtr<CLuaScriptContext> LuaContext;
};

} // namespace NLib::Examples

/*
========================================================================================
使用流程说明：

1. 编写C++类并添加脚本绑定标记：
```cpp
NCLASS(meta=(ScriptCreatable, ScriptVisible, ScriptName="Player"))
class NGamePlayer : public NObject
{
    GENERATED_BODY()

public:
    NPROPERTY(BlueprintReadWrite, meta=(ScriptReadable, ScriptWritable))
    TString PlayerName;

    NFUNCTION(BlueprintCallable, meta=(ScriptCallable))
    void TakeDamage(int32_t Amount);
};
```

2. 运行NutHeaderTools生成绑定代码：
```bash
NutHeaderTools.exe --input=Source --output=Generated --mode=ScriptBinding
```

3. NutHeaderTools会生成以下文件：
- NGamePlayer.script.generate.cpp  (绑定注册代码)
- Generated/ScriptBindings/Lua/Player.lua  (Lua绑定)
- Generated/ScriptBindings/TypeScript/Player.d.ts  (TypeScript类型定义)

4. 在应用程序中初始化脚本系统：
```cpp
// 初始化脚本引擎
auto LuaEngine = MakeShared<CLuaScriptEngine>();
LuaEngine->Initialize();

// 创建脚本上下文
SScriptConfig Config;
Config.bEnableSandbox = true;
Config.TimeoutMilliseconds = 5000;
auto Context = LuaEngine->CreateContext(Config);

// 加载脚本绑定
CScriptBindingLoader::GetInstance().LoadLuaBindings(Context, "Generated/ScriptBindings");
```

5. 在Lua脚本中使用C++对象：
```lua
-- 创建玩家对象
local player = Player.new()
player.Name = "TestPlayer"
player.HP = 100

-- 调用C++方法
player:TakeDamage(25)
print("Player health:", player.HP)

-- 使用静态方法
local newPlayer = Player.CreatePlayer("NewPlayer", 5)
```

6. 在C++中执行脚本：
```cpp
// 执行Lua脚本
auto Result = Context->ExecuteString(R"(
    local player = Player.new()
    player.Name = "ScriptPlayer"
    player:TakeDamage(30)
    return player.HP
)");

if (Result.Result == EScriptResult::Success)
{
    int32_t Health = Result.ReturnValue.ToInt32();
    NLOG_INFO("Player health from script: {}", Health);
}
```

========================================================================================
*/