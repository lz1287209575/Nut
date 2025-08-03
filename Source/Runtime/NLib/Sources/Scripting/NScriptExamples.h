#pragma once

/**
 * @file NScriptExamples.h
 * @brief 脚本集成系统使用示例
 * 
 * 展示如何使用NCLASS反射系统进行脚本绑定
 */

#include "Core/CObject.h"
#include "Scripting/NScriptMeta.h"
#include "Scripting/NScriptEngine.h"
#include "Memory/NSmartPointers.h"

namespace NLib
{

/**
 * @brief 基础游戏对象示例
 */
NCLASS(ScriptCreatable, ScriptName="GameObject", Category="Core", Description="Base game object")
class LIBNLIB_API NGameObject : public CObject
{
    GENERATED_BODY()

public:
    NGameObject();
    virtual ~NGameObject();

    // 位置属性 - 脚本可读写
    NPROPERTY(ScriptReadWrite, Description="Object position", DefaultValue="0,0,0")
    NVector3 Position;

    // 名称属性 - 脚本可读写，有验证器
    NPROPERTY(ScriptReadWrite, Description="Object name", Validator="ValidateName")
    CString Name;

    // 生命值 - 脚本只读，有范围限制
    NPROPERTY(ScriptReadable, Description="Object health", Min=0, Max=100, ReadOnly)
    float Health = 100.0f;

    // 是否激活 - 脚本可读写
    NPROPERTY(ScriptReadWrite, Description="Whether object is active")
    bool bIsActive = true;

    // 标签数组 - 脚本可读写
    NPROPERTY(ScriptReadWrite, Description="Object tags")
    CArray<CString> Tags;

    // 移动函数 - 脚本可调用
    NFUNCTION(ScriptCallable, Description="Move object by offset", 
              ParamNames="Offset", ParamDescriptions="Movement offset")
    void Move(const NVector3& Offset);

    // 设置生命值 - 脚本可调用，有参数验证
    NFUNCTION(ScriptCallable, Description="Set object health", 
              ParamNames="NewHealth", ParamDescriptions="New health value", 
              ParamDefaults="100")
    void SetHealth(float NewHealth);

    // 获取距离 - 纯函数，脚本可调用
    NFUNCTION(ScriptCallable, Pure, Description="Get distance to another object",
              ParamNames="Other", ParamDescriptions="Target object",
              ReturnDescription="Distance in units")
    float GetDistanceTo(NGameObject* Other) const;

    // 异步保存 - 异步函数
    NFUNCTION(ScriptCallable, Async, Description="Save object data asynchronously")
    void SaveAsync();

    // 静态工厂函数
    NFUNCTION(ScriptCallable, Static, Description="Create a new game object",
              ParamNames="Name", ParamDescriptions="Object name",
              ReturnDescription="New game object instance")
    static TSharedPtr<NGameObject> Create(const CString& Name);

protected:
    // 名称验证器
    bool ValidateName(const CString& NewName) const;

    // 内部函数 - 不暴露给脚本
    void InternalUpdate();
};

/**
 * @brief 玩家类示例 - 继承自NGameObject
 */
NCLASS(ScriptCreatable, ScriptName="Player", Category="Gameplay", 
       Description="Player character", BaseClass="GameObject")
class LIBNLIB_API CPlayer : public NGameObject
{
    GENERATED_BODY()

public:
    CPlayer();
    virtual ~CPlayer();

    // 玩家等级 - 脚本只读
    NPROPERTY(ScriptReadable, Description="Player level", Min=1, Max=100)
    int32_t Level = 1;

    // 经验值 - 脚本可读写
    NPROPERTY(ScriptReadWrite, Description="Player experience points")
    int32_t Experience = 0;

    // 装备列表 - 脚本只读
    NPROPERTY(ScriptReadable, Description="Player equipment")
    CArray<TSharedPtr<NGameObject>> Equipment;

    // 升级函数
    NFUNCTION(ScriptCallable, Description="Level up the player")
    void LevelUp();

    // 添加经验
    NFUNCTION(ScriptCallable, Description="Add experience points",
              ParamNames="Amount", ParamDescriptions="Experience amount to add")
    void AddExperience(int32_t Amount);

    // 装备物品
    NFUNCTION(ScriptCallable, Description="Equip an item",
              ParamNames="Item", ParamDescriptions="Item to equip")
    bool EquipItem(TSharedPtr<NGameObject> Item);

    // 获取玩家统计信息 - 返回脚本表
    NFUNCTION(ScriptCallable, Description="Get player statistics",
              ReturnDescription="Statistics table")
    CHashMap<CString, float> GetStatistics() const;
};

/**
 * @brief 游戏管理器 - 单例模式
 */
NCLASS(ScriptCreatable, ScriptName="GameManager", Category="Core", 
       Description="Game manager singleton", Singleton)
class LIBNLIB_API NGameManager : public CObject
{
    GENERATED_BODY()

public:
    static NGameManager& Get();

    // 游戏时间 - 脚本只读
    NPROPERTY(ScriptReadable, Description="Current game time in seconds")
    double GameTime = 0.0;

    // 玩家数量 - 脚本只读
    NPROPERTY(ScriptReadable, Description="Number of active players")
    int32_t PlayerCount = 0;

    // 游戏状态 - 脚本可读写
    NPROPERTY(ScriptReadWrite, Description="Current game state")
    CString GameState = "Menu";

    // 创建玩家
    NFUNCTION(ScriptCallable, Description="Create a new player",
              ParamNames="PlayerName", ParamDescriptions="Name of the new player",
              ReturnDescription="Created player instance")
    TSharedPtr<CPlayer> CreatePlayer(const CString& PlayerName);

    // 查找玩家
    NFUNCTION(ScriptCallable, Description="Find player by name",
              ParamNames="PlayerName", ParamDescriptions="Name to search for",
              ReturnDescription="Found player or null")
    TSharedPtr<CPlayer> FindPlayer(const CString& PlayerName);

    // 获取所有玩家
    NFUNCTION(ScriptCallable, Description="Get all active players",
              ReturnDescription="Array of all players")
    CArray<TSharedPtr<CPlayer>> GetAllPlayers();

    // 开始游戏
    NFUNCTION(ScriptCallable, Description="Start the game")
    void StartGame();

    // 结束游戏
    NFUNCTION(ScriptCallable, Description="End the game")
    void EndGame();

    // 保存游戏状态
    NFUNCTION(ScriptCallable, Async, Description="Save game state asynchronously")
    void SaveGameState();

private:
    NGameManager() = default;
    CArray<TSharedPtr<CPlayer>> Players;
    mutable NMutex PlayersMutex;
};

/**
 * @brief 数学工具类 - 全局函数示例
 */
class LIBNLIB_API NMathUtils
{
public:
    // 全局数学函数 - 脚本可调用
    NFUNCTION(ScriptCallable, Static, Pure, Description="Calculate distance between two points",
              ParamNames="Point1,Point2", ParamDescriptions="First point,Second point",
              ReturnDescription="Distance between points")
    static float Distance(const NVector3& Point1, const NVector3& Point2);

    NFUNCTION(ScriptCallable, Static, Pure, Description="Linear interpolation",
              ParamNames="A,B,TType", ParamDescriptions="Start value,End value,Interpolation factor",
              ParamDefaults="0,1,0.5", ReturnDescription="Interpolated value")
    static float Lerp(float A, float B, float TType);

    NFUNCTION(ScriptCallable, Static, Pure, Description="Clamp value to range",
              ParamNames="Value,Min,Max", ParamDescriptions="Value to clamp,Minimum value,Maximum value",
              ReturnDescription="Clamped value")
    static float Clamp(float Value, float Min, float Max);
};

/**
 * @brief 事件系统示例
 */
NCLASS(ScriptCreatable, ScriptName="EventDispatcher", Category="Events",
       Description="Event dispatcher for script communication")
class LIBNLIB_API NScriptEventDispatcher : public CObject
{
    GENERATED_BODY()

public:
    // 绑定事件监听器
    NFUNCTION(ScriptCallable, Description="Bind event listener",
              ParamNames="EventName,Callback", ParamDescriptions="Event name,Callback function")
    void BindEvent(const CString& EventName, const CScriptValue& Callback);

    // 解绑事件监听器
    NFUNCTION(ScriptCallable, Description="Unbind event listener",
              ParamNames="EventName,Callback", ParamDescriptions="Event name,Callback function")
    void UnbindEvent(const CString& EventName, const CScriptValue& Callback);

    // 触发事件
    NFUNCTION(ScriptCallable, Description="Trigger event",
              ParamNames="EventName,Data", ParamDescriptions="Event name,Event data")
    void TriggerEvent(const CString& EventName, const CScriptValue& Data = CScriptValue());

    // 延迟触发事件
    NFUNCTION(ScriptCallable, Description="Trigger event after delay",
              ParamNames="EventName,Delay,Data", ParamDescriptions="Event name,Delay in seconds,Event data")
    void TriggerEventDelayed(const CString& EventName, float Delay, const CScriptValue& Data = CScriptValue());

private:
    CHashMap<CString, CArray<CScriptValue>> EventListeners;
    mutable NMutex EventMutex;
};

/**
 * @brief 脚本配置示例
 */
NCLASS(ScriptCreatable, ScriptName="Config", Category="Core",
       Description="Configuration management", Singleton)
class LIBNLIB_API NScriptConfig : public CObject
{
    GENERATED_BODY()

public:
    static NScriptConfig& Get();

    // 获取配置值
    NFUNCTION(ScriptCallable, Description="Get configuration value",
              ParamNames="Key,DefaultValue", ParamDescriptions="Configuration key,Default value if not found",
              ReturnDescription="Configuration value")
    CScriptValue GetValue(const CString& Key, const CScriptValue& DefaultValue = CScriptValue()) const;

    // 设置配置值
    NFUNCTION(ScriptCallable, Description="Set configuration value",
              ParamNames="Key,Value", ParamDescriptions="Configuration key,Value to set")
    void SetValue(const CString& Key, const CScriptValue& Value);

    // 保存配置
    NFUNCTION(ScriptCallable, Description="Save configuration to file")
    bool SaveConfig();

    // 加载配置
    NFUNCTION(ScriptCallable, Description="Load configuration from file")
    bool LoadConfig();

    // 重置配置
    NFUNCTION(ScriptCallable, Description="Reset configuration to defaults")
    void ResetToDefaults();

private:
    NScriptConfig() = default;
    CHashMap<CString, CScriptValue> ConfigValues;
    mutable NMutex ConfigMutex;
};

/**
 * @brief 脚本使用示例
 */
namespace ScriptExamples
{
    /**
     * @brief 基本脚本绑定示例
     */
    void BasicBindingExample()
    {
        // 获取Lua引擎
        auto& Manager = NScriptEngineManager::Get();
        auto LuaEngine = Manager.GetEngine(EScriptLanguage::Lua);
        auto Context = LuaEngine->GetMainContext();

        // 自动绑定所有脚本可访问的类
        LuaEngine->AutoBindClasses();

        // 执行Lua脚本
        CString LuaCode = R"(
            -- 创建游戏对象
            local gameObj = GameObject.Create("TestObject")
            gameObj:SetPosition(Vector3.New(10, 20, 30))
            gameObj:SetHealth(75)
            
            -- 创建玩家
            local player = GameManager.Get():CreatePlayer("Player1")
            player:AddExperience(1000)
            player:LevelUp()
            
            -- 输出玩家信息
            print("Player level: " .. player.Level)
            print("Player health: " .. player.Health)
            
            -- 计算距离
            local distance = MathUtils.Distance(gameObj.Position, player.Position)
            print("Distance: " .. distance)
        )";

        auto Result = Context->Execute(LuaCode);
        if (!Result.IsSuccess())
        {
            CLogger::Get().LogError("Lua execution failed: {}", Result.ErrorMessage);
        }
    }

    /**
     * @brief 事件系统示例
     */
    void EventSystemExample()
    {
        auto& Manager = NScriptEngineManager::Get();
        auto Context = Manager.GetMainContext(EScriptLanguage::Lua);

        CString LuaCode = R"(
            -- 创建事件分发器
            local eventDispatcher = EventDispatcher.New()
            
            -- 绑定事件监听器
            eventDispatcher:BindEvent("PlayerDamaged", function(data)
                print("Player took " .. data.damage .. " damage!")
                if data.health <= 0 then
                    print("Player died!")
                end
            end)
            
            -- 触发事件
            eventDispatcher:TriggerEvent("PlayerDamaged", {
                damage = 25,
                health = 75
            })
            
            -- 延迟触发事件
            eventDispatcher:TriggerEventDelayed("PlayerRespawn", 3.0, {
                position = Vector3.New(0, 0, 0)
            })
        )";

        Context->Execute(LuaCode);
    }

    /**
     * @brief 配置管理示例
     */
    void ConfigManagementExample()
    {
        auto& Manager = NScriptEngineManager::Get();
        auto Context = Manager.GetMainContext(EScriptLanguage::Lua);

        CString LuaCode = R"(
            -- 获取配置管理器
            local config = Config.Get()
            
            -- 设置配置值
            config:SetValue("graphics.resolution", "1920x1080")
            config:SetValue("audio.volume", 0.8)
            config:SetValue("gameplay.difficulty", "Normal")
            
            -- 获取配置值
            local resolution = config:GetValue("graphics.resolution", "800x600")
            local volume = config:GetValue("audio.volume", 1.0)
            
            print("Resolution: " .. resolution)
            print("Volume: " .. volume)
            
            -- 保存配置
            config:SaveConfig()
        )";

        Context->Execute(LuaCode);
    }

    /**
     * @brief 热重载示例
     */
    void HotReloadExample()
    {
        auto& Manager = NScriptEngineManager::Get();
        auto LuaEngine = Manager.GetEngine(EScriptLanguage::Lua);

        // 启用热重载
        LuaEngine->EnableHotReload("Scripts/");

        // 脚本文件被修改时会自动重新加载
        // 可以在运行时修改脚本逻辑而无需重启程序
    }

    /**
     * @brief 性能监控示例
     */
    void PerformanceMonitoringExample()
    {
        auto& Manager = NScriptEngineManager::Get();
        auto Stats = Manager.GetAllStatistics();

        for (const auto& [Language, EngineStats] : Stats)
        {
            CLogger::Get().LogInfo("Language: {}", (int32_t)Language);
            for (const auto& [StatName, StatValue] : EngineStats)
            {
                CLogger::Get().LogInfo("  {}: {}", StatName, StatValue);
            }
        }
    }
}

} // namespace NLib