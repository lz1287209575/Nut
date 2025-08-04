#include "ScriptSystemExample.h"

#include "IO/Path.h"
#include "Logging/LogCategory.h"

namespace NLib::Examples
{
// === CGamePlayer 实现 ===

CGamePlayer::CGamePlayer()
    : PlayerName(TEXT("DefaultPlayer")),
      Health(100),
      Level(1)
{
	NLOG_SCRIPT(Debug, "Created GamePlayer: {}", PlayerName.GetData());
}

void CGamePlayer::ReceiveDamage(int32_t Amount)
{
	if (Amount <= 0)
		return;

	Health -= Amount;
	if (Health < 0)
		Health = 0;

	NLOG_SCRIPT(Info, "Player '{}' took {} damage, health: {}", PlayerName.GetData(), Amount, Health);

	CheckDeath();
}

void CGamePlayer::Heal(int32_t Amount)
{
	if (Amount <= 0)
		return;

	Health += Amount;
	if (Health > 100)
		Health = 100;

	NLOG_SCRIPT(Info, "Player '{}' healed {} points, health: {}", PlayerName.GetData(), Amount, Health);
}

TString CGamePlayer::GetPlayerInfo() const
{
	return TString::Format(TEXT("Player: {} (Level: {}, Health: {})"), PlayerName.GetData(), Level, Health);
}

CGamePlayer* CGamePlayer::CreatePlayer(const TString& Name, int32_t InitialLevel)
{
	auto Player = NewNObject<CGamePlayer>();
	if (Player)
	{
		Player->PlayerName = Name;
		Player->Level = InitialLevel;
		NLOG_SCRIPT(Info, "Created player '{}' at level {}", Name.GetData(), InitialLevel);
	}
	return Player.Get();
}

void CGamePlayer::OnPlayerDeath()
{
	NLOG_SCRIPT(Info, "Player '{}' has died", PlayerName.GetData());
	// 默认实现，可在脚本中重写
}

void CGamePlayer::CheckDeath()
{
	if (Health <= 0)
	{
		OnPlayerDeath();
	}
}

// === CGameItem 实现 ===

void CGameItem::UseItem()
{
	if (CanUse())
	{
		ItemCount--;
		NLOG_SCRIPT(Info, "Used item '{}', remaining: {}", ItemName.GetData(), ItemCount);
	}
	else
	{
		NLOG_SCRIPT(Warning, "Cannot use item '{}'", ItemName.GetData());
	}
}

bool CGameItem::CanUse() const
{
	return ItemCount > 0;
}

// === CScriptSystemExample 实现 ===

TSharedPtr<CLuaScriptEngine> CScriptSystemExample::LuaEngine = nullptr;
TSharedPtr<CLuaScriptContext> CScriptSystemExample::LuaContext = nullptr;

bool CScriptSystemExample::InitializeScriptSystem(const TString& BindingDirectory)
{
	NLOG_SCRIPT(Info, "Initializing Script System Example...");

	// 1. 创建并初始化Lua引擎
	LuaEngine = MakeShared<CLuaScriptEngine>();
	if (!LuaEngine->Initialize())
	{
		NLOG_SCRIPT(Error, "Failed to initialize Lua engine");
		return false;
	}

	// 2. 创建脚本上下文
	SScriptConfig Config;
	Config.bEnableSandbox = true;               // 启用沙箱模式
	Config.TimeoutMilliseconds = 5000;          // 5秒超时
	Config.MemoryLimitBytes = 64 * 1024 * 1024; // 64MB内存限制

	LuaContext = LuaEngine->CreateContext(Config);
	if (!LuaContext)
	{
		NLOG_SCRIPT(Error, "Failed to create Lua context");
		return false;
	}

	// 3. 加载脚本绑定
	auto& BindingLoader = CScriptBindingLoader::GetInstance();
	if (!BindingLoader.LoadLuaBindings(LuaContext, BindingDirectory))
	{
		NLOG_SCRIPT(Warning, "Failed to load Lua bindings from: {}", BindingDirectory.GetData());
		// 继续执行，可能绑定文件还没生成
	}

	// 4. 打印绑定统计信息
	BindingLoader.PrintBindingInfo();

	NLOG_SCRIPT(Info, "Script System initialized successfully");
	return true;
}

void CScriptSystemExample::RunLuaExample()
{
	if (!LuaContext)
	{
		NLOG_SCRIPT(Error, "Lua context not initialized");
		return;
	}

	NLOG_SCRIPT(Info, "Running Lua Script Example...");

	// 示例1: 基础脚本执行
	TString BasicScript = TEXT(R"(
            print("Hello from Lua!")
            local result = 10 + 20
            print("Calculation result:", result)
            return result
        )");

	auto Result = LuaContext->ExecuteString(BasicScript);
	if (Result.Result == EScriptResult::Success)
	{
		NLOG_SCRIPT(Info, "Basic script executed successfully, result: {}", Result.ReturnValue.ToInt32());
	}
	else
	{
		NLOG_SCRIPT(Error, "Basic script failed: {}", Result.ErrorMessage.GetData());
	}

	// 示例2: 使用NLib功能
	TString NLibScript = TEXT(R"(
            -- 测试NLib API（如果已加载）
            if NLib then
                print("NLib is available!")
                -- TODO: 调用NLib函数
            else
                print("NLib bindings not loaded")
            end
        )");

	Result = LuaContext->ExecuteString(NLibScript);
	if (Result.Result != EScriptResult::Success)
	{
		NLOG_SCRIPT(Error, "NLib script failed: {}", Result.ErrorMessage.GetData());
	}
}

void CScriptSystemExample::DemonstrateScriptObjectInteraction()
{
	if (!LuaContext)
	{
		NLOG_SCRIPT(Error, "Lua context not initialized");
		return;
	}

	NLOG_SCRIPT(Info, "Demonstrating Script-Object Interaction...");

	// 创建C++对象
	auto Player = NewNObject<CGamePlayer>();
	Player->PlayerName = TEXT("CppPlayer");
	Player->Level = 5;

	// 通过脚本绑定加载器注册对象到脚本
	// 注意：这需要实际的绑定系统支持

	TString InteractionScript = TEXT(R"(
            -- 如果Player类已绑定，可以这样使用：
            -- local player = GetCppObject("CppPlayer")  -- 获取C++创建的对象
            -- if player then
            --     print("Player name:", player.Name)
            --     print("Player level:", player.Level)
            --     player:TakeDamage(25)
            --     print("Player health after damage:", player.HP)
            -- end
            
            print("Script-Object interaction demo completed")
        )");

	auto Result = LuaContext->ExecuteString(InteractionScript);
	if (Result.Result != EScriptResult::Success)
	{
		NLOG_SCRIPT(Error, "Interaction script failed: {}", Result.ErrorMessage.GetData());
	}

	// 显示C++对象的最终状态
	NLOG_SCRIPT(Info, "Final player state: {}", Player->GetPlayerInfo().GetData());
}

void CScriptSystemExample::DemonstrateScriptObjectCreation()
{
	if (!LuaContext)
	{
		NLOG_SCRIPT(Error, "Lua context not initialized");
		return;
	}

	NLOG_SCRIPT(Info, "Demonstrating Script Object Creation...");

	TString CreationScript = TEXT(R"(
            -- 如果Player类已绑定，可以这样创建对象：
            -- local player1 = Player.new()
            -- player1.Name = "LuaPlayer1"
            -- player1.Level = 3
            -- print("Created player:", player1:GetPlayerInfo())
            
            -- 使用静态方法创建
            -- local player2 = Player.CreatePlayer("LuaPlayer2", 10)
            -- player2:TakeDamage(50)
            -- print("Created player via static method:", player2:GetPlayerInfo())
            
            -- 创建道具
            -- local sword = GameItem.new()
            -- sword.ItemName = "Magic Sword"
            -- sword.ItemCount = 1
            -- sword:UseItem()
            
            print("Object creation demo completed")
        )");

	auto Result = LuaContext->ExecuteString(CreationScript);
	if (Result.Result != EScriptResult::Success)
	{
		NLOG_SCRIPT(Error, "Creation script failed: {}", Result.ErrorMessage.GetData());
	}
}

void CScriptSystemExample::CleanupScriptSystem()
{
	NLOG_SCRIPT(Info, "Cleaning up Script System...");

	if (LuaContext)
	{
		LuaContext->Shutdown();
		LuaContext = nullptr;
	}

	if (LuaEngine)
	{
		LuaEngine->Shutdown();
		LuaEngine = nullptr;
	}

	NLOG_SCRIPT(Info, "Script System cleaned up");
}

} // namespace NLib::Examples

/*
========================================================================================
运行示例的完整代码：

```cpp
#include "ScriptSystemExample.h"

int main()
{
    using namespace NLib::Examples;

    // 1. 初始化脚本系统
    if (!CScriptSystemExample::InitializeScriptSystem("Generated/ScriptBindings"))
    {
        return -1;
    }

    // 2. 运行基础Lua示例
    CScriptSystemExample::RunLuaExample();

    // 3. 演示脚本与对象交互
    CScriptSystemExample::DemonstrateScriptObjectInteraction();

    // 4. 演示脚本创建对象
    CScriptSystemExample::DemonstrateScriptObjectCreation();

    // 5. 清理系统
    CScriptSystemExample::CleanupScriptSystem();

    return 0;
}
```

注意事项：
1. 在实际使用前，需要先运行NutHeaderTools生成脚本绑定代码
2. 确保生成的绑定文件放在正确的目录中
3. 如果没有生成绑定文件，脚本中的C++对象调用将不可用
4. 可以通过修改示例类的meta标签来定制脚本绑定行为

生成绑定的典型命令行：
```bash
NutHeaderTools.exe --project-root=. --header-dirs=Source/Runtime/NLib/Sources --output-dir=Generated/ScriptBindings
--languages=Lua,TypeScript
```
========================================================================================
*/