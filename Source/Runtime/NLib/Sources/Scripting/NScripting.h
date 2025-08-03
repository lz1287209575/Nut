#pragma once

/**
 * @file NScripting.h
 * @brief 脚本系统总头文件
 * 
 * 集成整个脚本系统，提供统一的包含接口
 */

#include "Scripting/NScriptMeta.h"
#include "Scripting/NScriptEngine.h"
#include "Scripting/NLuaEngine.h"
#include "Scripting/NLuaClassEngine.h"
#include "Scripting/NPythonEngine.h"
#include "Scripting/NTypeScriptEngine.h"
#include "Scripting/NCSharpEngine.h"
#include "Scripting/NBPEngine.h"
#include "Scripting/NScriptBindingGenerator.h"
#include "Scripting/NScriptExamples.h"

namespace NLib
{

/**
 * @brief 脚本系统概览
 * 
 * NLib脚本系统提供了完整的脚本语言集成解决方案：
 * 
 * 1. **元数据系统 (NScriptMeta.h)**
 *    - 基于NCLASS反射的脚本绑定元数据
 *    - 支持ScriptCreatable, ScriptReadWrite, ScriptCallable等标记
 *    - 自动元数据注册和查询
 * 
 * 2. **统一脚本引擎接口 (NScriptEngine.h)**
 *    - 支持多种脚本语言 (Lua, Python, JavaScript)
 *    - 统一的脚本值和上下文管理
 *    - 热重载和调试支持
 * 
 * 3. **Lua引擎实现 (NLuaEngine.h)**
 *    - 基于Lua 5.4的高性能实现
 *    - 自动C++对象绑定
 *    - 内存限制和执行超时
 * 
 * 4. **自动绑定生成器 (NScriptBindingGenerator.h)**
 *    - 基于元数据自动生成绑定代码
 *    - 支持多种脚本语言的代码生成
 *    - 集成到NutHeaderTools构建工具
 * 
 * 5. **使用示例 (NScriptExamples.h)**
 *    - 完整的游戏对象脚本绑定示例
 *    - 事件系统、配置管理、热重载等
 */

/**
 * @brief 快速开始指南
 * 
 * 1. **定义脚本可访问的类**
 * ```cpp
 * NCLASS(ScriptCreatable, Category="Gameplay")
 * class CPlayer : public CObject
 * {
 *     GENERATED_BODY()
 * 
 * public:
 *     NPROPERTY(ScriptReadWrite, Description="Player health")
 *     float Health = 100.0f;
 *     
 *     NFUNCTION(ScriptCallable, Description="Take damage")
 *     void TakeDamage(float Amount);
 * };
 * ```
 * 
 * 2. **初始化脚本系统**
 * ```cpp
 * // 创建并注册Lua引擎
 * auto LuaEngine = MakeShared<NLuaEngine>();
 * NScriptEngineManager::Get().RegisterEngine(EScriptLanguage::Lua, LuaEngine);
 * 
 * // 自动绑定所有脚本可访问的类
 * LuaEngine->AutoBindClasses();
 * ```
 * 
 * 3. **执行脚本代码**
 * ```cpp
 * auto Context = LuaEngine->GetMainContext();
 * CString LuaCode = R"(
 *     local player = CPlayer.New()
 *     player.Health = 75
 *     player:TakeDamage(25)
 *     print("Player health: " .. player.Health)
 * )";
 * 
 * auto Result = Context->Execute(LuaCode);
 * if (!Result.IsSuccess()) {
 *     // 处理错误
 * }
 * ```
 */

/**
 * @brief 支持的脚本元数据标记
 * 
 * **类标记 (NCLASS):**
 * - ScriptCreatable: 脚本可创建实例
 * - ScriptName="Name": 脚本中的名称
 * - Category="Category": 分类
 * - Description="Desc": 描述
 * - Languages=Lua|Python: 支持的脚本语言
 * - Singleton: 单例模式
 * - Abstract: 抽象类
 * 
 * **属性标记 (NPROPERTY):**
 * - ScriptReadable: 脚本可读
 * - ScriptWritable: 脚本可写  
 * - ScriptReadWrite: 脚本可读写
 * - ReadOnly: 只读属性
 * - Transient: 临时属性
 * - Min=Value, Max=Value: 数值范围
 * - DefaultValue="Value": 默认值
 * - Validator="FunctionName": 验证器
 * 
 * **函数标记 (NFUNCTION):**
 * - ScriptCallable: 脚本可调用
 * - Pure: 纯函数（无副作用）
 * - Async: 异步函数
 * - Static: 静态函数
 * - ParamNames="name1,name2": 参数名称
 * - ParamDescriptions="desc1,desc2": 参数描述
 * - ParamDefaults="val1,val2": 参数默认值
 * - ReturnDescription="Desc": 返回值描述
 */

/**
 * @brief 脚本系统特性
 * 
 * **性能优化:**
 * - 内存池管理
 * - 执行超时控制
 * - 智能垃圾回收
 * - 编译缓存
 * 
 * **开发体验:**
 * - 自动代码生成
 * - 热重载支持
 * - 实时调试
 * - 错误追踪
 * 
 * **扩展性:**
 * - 插件式脚本引擎
 * - 自定义绑定生成
 * - 多语言支持
 * - 模块化架构
 */

/**
 * @brief 脚本系统初始化辅助函数
 */
class LIBNLIB_API NScriptingSystem
{
public:
    /**
     * @brief 初始化脚本系统
     * @param Languages 要初始化的脚本语言
     * @return 是否成功初始化
     */
    static bool Initialize(EScriptLanguage Languages = EScriptLanguage::All);

    /**
     * @brief 关闭脚本系统
     */
    static void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    static bool IsInitialized();

    /**
     * @brief 自动绑定所有脚本可访问的类
     */
    static bool AutoBindAllClasses();

    /**
     * @brief 执行初始化脚本
     * @param ScriptPath 脚本文件路径
     */
    static bool ExecuteInitScript(const CString& ScriptPath);

    /**
     * @brief 启用热重载
     * @param WatchDirectory 监视目录
     */
    static bool EnableHotReload(const CString& WatchDirectory);

    /**
     * @brief 获取系统统计信息
     */
    static CHashMap<CString, double> GetSystemStatistics();

private:
    static bool bInitialized;
    static EScriptLanguage InitializedLanguages;
};

/**
 * @brief 便利宏定义
 */
#define INIT_SCRIPTING_SYSTEM(Languages) NLib::NScriptingSystem::Initialize(Languages)
#define SHUTDOWN_SCRIPTING_SYSTEM() NLib::NScriptingSystem::Shutdown()
#define AUTO_BIND_SCRIPT_CLASSES() NLib::NScriptingSystem::AutoBindAllClasses()
#define ENABLE_SCRIPT_HOT_RELOAD(Dir) NLib::NScriptingSystem::EnableHotReload(Dir)

#define GET_LUA_ENGINE() NLib::NScriptEngineManager::Get().GetEngine(NLib::EScriptLanguage::Lua)
#define GET_PYTHON_ENGINE() NLib::NScriptEngineManager::Get().GetEngine(NLib::EScriptLanguage::Python)
#define GET_JS_ENGINE() NLib::NScriptEngineManager::Get().GetEngine(NLib::EScriptLanguage::JavaScript)

#define EXECUTE_LUA(Code) GET_LUA_ENGINE()->GetMainContext()->Execute(Code)
#define EXECUTE_PYTHON(Code) GET_PYTHON_ENGINE()->GetMainContext()->Execute(Code)
#define EXECUTE_JS(Code) GET_JS_ENGINE()->GetMainContext()->Execute(Code)

} // namespace NLib