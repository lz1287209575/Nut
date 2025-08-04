#pragma once

#include <cstddef>
#include <typeinfo>

namespace NLib
{
// 前向声明
struct SClassReflection;
struct SPropertyReflection;
struct SFunctionReflection;
class CReflectionRegistry;
class NObject;

/**
 * @brief 反射属性标志
 */
enum class EPropertyFlags : uint32_t
{
	None = 0,
	EditAnywhere = 1 << 0,        // 可在编辑器中编辑
	EditDefaultsOnly = 1 << 1,    // 只能在类默认值中编辑
	EditInstanceOnly = 1 << 2,    // 只能在实例中编辑
	VisibleAnywhere = 1 << 3,     // 在编辑器中可见
	VisibleDefaultsOnly = 1 << 4, // 只在类默认值中可见
	VisibleInstanceOnly = 1 << 5, // 只在实例中可见
	BlueprintReadOnly = 1 << 6,   // 蓝图只读
	BlueprintReadWrite = 1 << 7,  // 蓝图可读写
	SaveGame = 1 << 8,            // 保存到存档
	Transient = 1 << 9,           // 临时变量，不序列化
	Config = 1 << 10,             // 配置变量
	Replicated = 1 << 11,         // 网络复制
	ReplicatedUsing = 1 << 12,    // 使用自定义函数复制
	NotReplicated = 1 << 13,      // 不复制
	Interp = 1 << 14,             // 可插值
	NonTransactional = 1 << 15    // 不参与事务
};

/**
 * @brief 反射函数标志
 */
enum class EFunctionFlags : uint32_t
{
	None = 0,
	BlueprintCallable = 1 << 0,           // 蓝图可调用
	BlueprintImplementableEvent = 1 << 1, // 蓝图实现事件
	BlueprintNativeEvent = 1 << 2,        // 蓝图原生事件
	CallInEditor = 1 << 3,                // 编辑器中可调用
	Exec = 1 << 4,                        // 控制台命令
	Server = 1 << 5,                      // 服务器RPC
	Client = 1 << 6,                      // 客户端RPC
	NetMulticast = 1 << 7,                // 多播RPC
	Reliable = 1 << 8,                    // 可靠传输
	Unreliable = 1 << 9,                  // 不可靠传输
	WithValidation = 1 << 10,             // 带验证
	Pure = 1 << 11,                       // 纯函数（无副作用）
	Const = 1 << 12,                      // 常量函数
	Static = 1 << 13,                     // 静态函数
	Final = 1 << 14,                      // 不可重写
	Override = 1 << 15                    // 重写父类函数
};

/**
 * @brief 反射类标志
 */
enum class EClassFlags : uint32_t
{
	None = 0,
	Abstract = 1 << 0,                    // 抽象类
	BlueprintType = 1 << 1,               // 蓝图类型
	Blueprintable = 1 << 2,               // 可创建蓝图
	NotBlueprintable = 1 << 3,            // 不可创建蓝图
	BlueprintSpawnableComponent = 1 << 4, // 蓝图可生成组件
	Config = 1 << 5,                      // 配置类
	DefaultConfig = 1 << 6,               // 默认配置
	GlobalUserConfig = 1 << 7,            // 全局用户配置
	ProjectUserConfig = 1 << 8,           // 项目用户配置
	Transient = 1 << 9,                   // 临时类
	MatchedSerializers = 1 << 10,         // 匹配的序列化器
	Native = 1 << 11,                     // 原生类
	NotPlaceable = 1 << 12,               // 不可放置
	Placeable = 1 << 13,                  // 可放置
	Deprecated = 1 << 14,                 // 已废弃
	Experimental = 1 << 15                // 实验性
};

} // namespace NLib

// === 核心反射宏 ===

/**
 * @brief NCLASS 宏 - 标记类需要反射支持
 *
 * 语法：NCLASS(meta = (key1=value1, key2=value2, ...))
 *
 * 常用元数据：
 * - BlueprintType: 可用于蓝图
 * - Blueprintable: 可创建蓝图子类
 * - Abstract: 抽象类
 * - Category: 分类
 * - DisplayName: 显示名称
 * - ToolTip: 工具提示
 *
 * 脚本绑定元数据：
 * - ScriptName: 脚本中的类名 (ScriptName="Player")
 * - ScriptModule: 所属脚本模块 (ScriptModule="GameLogic")
 * - ScriptLanguages: 支持的脚本语言 (ScriptLanguages="Lua,TypeScript,Python")
 * - ScriptCategory: 脚本分类 (ScriptCategory="Gameplay")
 * - ScriptCreatable: 可在脚本中创建实例 (ScriptCreatable=true)
 * - ScriptVisible: 在脚本中可见 (ScriptVisible=true)
 *
 * 示例：
 * NCLASS(meta = (BlueprintType = true, ScriptCreatable = true, ScriptName = "GamePlayer"))
 * class NGamePlayer : public NObject
 */
#define NCLASS(...)

/**
 * @brief NPROPERTY 宏 - 标记属性需要反射支持
 *
 * 常用标志：
 * - EditAnywhere: 可在任何地方编辑
 * - EditDefaultsOnly: 只能在类默认值中编辑
 * - VisibleAnywhere: 在任何地方可见
 * - BlueprintReadOnly: 蓝图只读
 * - BlueprintReadWrite: 蓝图可读写
 * - Category: 分类
 * - DisplayName: 显示名称
 * - ToolTip: 工具提示
 *
 * 脚本绑定元数据：
 * - ScriptName: 脚本中的属性名 (meta=(ScriptName="PlayerName"))
 * - ScriptReadable: 脚本可读 (meta=(ScriptReadable=true))
 * - ScriptWritable: 脚本可写 (meta=(ScriptWritable=true))
 * - ScriptLanguages: 支持的脚本语言 (meta=(ScriptLanguages="Lua,TypeScript"))
 *
 * 示例：
 * NPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ScriptReadable=true, ScriptWritable=true,
 * ScriptName="PlayerHealth")) int32_t Health;
 */
#define NPROPERTY(...)

/**
 * @brief NFUNCTION 宏 - 标记函数需要反射支持
 *
 * 常用标志：
 * - BlueprintCallable: 蓝图可调用
 * - BlueprintImplementableEvent: 蓝图实现事件
 * - CallInEditor: 编辑器中可调用
 * - Category: 分类
 * - DisplayName: 显示名称
 * - ToolTip: 工具提示
 *
 * 脚本绑定元数据：
 * - ScriptName: 脚本中的函数名 (meta=(ScriptName="TakeDamage"))
 * - ScriptCallable: 脚本可调用 (meta=(ScriptCallable=true))
 * - ScriptStatic: 静态函数 (meta=(ScriptStatic=true))
 * - ScriptEvent: 脚本事件 (meta=(ScriptEvent=true))
 * - ScriptLanguages: 支持的脚本语言 (meta=(ScriptLanguages="Lua,Python"))
 *
 * 示例：
 * NFUNCTION(BlueprintCallable, meta=(ScriptCallable=true, ScriptName="DealDamage"))
 * void TakeDamage(int32_t Amount);
 */
#define NFUNCTION(...)

/**
 * @brief NMETHOD 宏 - 标记方法需要反射支持（NFUNCTION的别名）
 */
#define NMETHOD(...) NFUNCTION(__VA_ARGS__)

/**
 * @brief NENUM 宏 - 标记枚举需要反射支持
 *
 * 示例：
 * NENUM(BlueprintType)
 * enum class EPlayerState
 * {
 *     Idle,
 *     Moving,
 *     Fighting
 * };
 */
#define NENUM(...)

/**
 * @brief NSTRUCT 宏 - 标记结构体需要反射支持
 *
 * 示例：
 * NSTRUCT(BlueprintType)
 * struct SPlayerData
 * {
 *     GENERATED_BODY()
 *
 *     NPROPERTY(EditAnywhere)
 *     CString PlayerName;
 * };
 */
#define NSTRUCT(...)

/**
 * @brief GENERATED_BODY 宏 - 插入生成的反射代码
 *
 * 这个宏会被NutHeaderTools替换为实际的生成代码，包括：
 * - 类型信息函数
 * - 反射注册代码
 * - 构造函数辅助代码
 *
 * 注意：GENERATED_BODY() 必须放在类的public部分
 */
#define GENERATED_BODY()                                                                                               \
private:                                                                                                               \
	friend class NLib::CReflectionRegistry;                                                                            \
	static bool bReflectionRegistered;                                                                                 \
	static void RegisterReflection();                                                                                  \
                                                                                                                       \
public:                                                                                                                \
	using Super = NObject;                                                                                             \
	virtual const std::type_info& GetTypeInfo() const override                                                         \
	{                                                                                                                  \
		return typeid(*this);                                                                                          \
	}                                                                                                                  \
	virtual const char* GetTypeName() const override                                                                   \
	{                                                                                                                  \
		return GetStaticTypeName();                                                                                    \
	}                                                                                                                  \
	virtual const NLib::SClassReflection* GetClassReflection() const override;                                         \
	static const char* GetStaticTypeName();                                                                            \
	static const NLib::SClassReflection* GetStaticClassReflection();                                                   \
	static NLib::NObject* CreateDefaultObject();                                                                       \
                                                                                                                       \
private:

/**
 * @brief GENERATED_NSTRUCT_BODY 宏 - 用于结构体的生成代码
 */
#define GENERATED_NSTRUCT_BODY()                                                                                       \
public:                                                                                                                \
	virtual const std::type_info& GetTypeInfo() const                                                                  \
	{                                                                                                                  \
		return typeid(*this);                                                                                          \
	}                                                                                                                  \
	virtual const char* GetTypeName() const                                                                            \
	{                                                                                                                  \
		return GetStaticTypeName();                                                                                    \
	}                                                                                                                  \
	static const char* GetStaticTypeName();                                                                            \
	static const NLib::SClassReflection* GetStaticClassReflection();                                                   \
                                                                                                                       \
private:

// === 反射注册宏 ===

/**
 * @brief 注册类反射信息到反射系统
 * 这个宏由NutHeaderTools在.generate.h文件中使用
 */
#define REGISTER_NCLASS_REFLECTION(ClassName)                                                                          \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct ClassName##ReflectionRegistrar                                                                              \
	{                                                                                                                  \
		ClassName##ReflectionRegistrar()                                                                               \
		{                                                                                                              \
			ClassName::RegisterReflection();                                                                           \
		}                                                                                                              \
	};                                                                                                                 \
	static ClassName##ReflectionRegistrar ClassName##_registrar_;                                                      \
	}

/**
 * @brief 实现类反射信息
 * 这个宏用在.cpp文件中实现反射功能
 */
#define IMPLEMENT_NCLASS_REFLECTION(ClassName, BaseClassName)                                                          \
	bool ClassName::bReflectionRegistered = false;                                                                     \
	const char* ClassName::GetStaticTypeName()                                                                         \
	{                                                                                                                  \
		return #ClassName;                                                                                             \
	}                                                                                                                  \
	const NLib::SClassReflection* ClassName::GetClassReflection() const                                                \
	{                                                                                                                  \
		return GetStaticClassReflection();                                                                             \
	}                                                                                                                  \
	const NLib::SClassReflection* ClassName::GetStaticClassReflection()                                                \
	{                                                                                                                  \
		static NLib::SClassReflection ClassReflection = {#ClassName,                                                   \
		                                                 #BaseClassName,                                               \
		                                                 sizeof(ClassName),                                            \
		                                                 &typeid(ClassName),                                           \
		                                                 nullptr, /* Properties */                                     \
		                                                 0,       /* PropertyCount */                                  \
		                                                 nullptr, /* Functions */                                      \
		                                                 0,       /* FunctionCount */                                  \
		                                                 NLib::EClassFlags::None};                                     \
		return &ClassReflection;                                                                                       \
	}                                                                                                                  \
	NLib::NObject* ClassName::CreateDefaultObject()                                                                    \
	{                                                                                                                  \
		return NewNObject<ClassName>().Get();                                                                          \
	}                                                                                                                  \
	void ClassName::RegisterReflection()                                                                               \
	{                                                                                                                  \
		if (!bReflectionRegistered)                                                                                    \
		{                                                                                                              \
			auto& Registry = NLib::CReflectionRegistry::GetInstance();                                                 \
			Registry.RegisterClass(GetStaticClassReflection());                                                        \
			bReflectionRegistered = true;                                                                              \
		}                                                                                                              \
	}

// === 便捷宏 ===

/**
 * @brief 声明类需要反射但不在当前文件实现
 */
#define DECLARE_NCLASS(ClassName, BaseClassName)                                                                       \
public:                                                                                                                \
	using Super = BaseClassName;                                                                                       \
	static const char* GetStaticTypeName()                                                                             \
	{                                                                                                                  \
		return #ClassName;                                                                                             \
	}                                                                                                                  \
	virtual const char* GetTypeName() const override                                                                   \
	{                                                                                                                  \
		return GetStaticTypeName();                                                                                    \
	}                                                                                                                  \
	virtual const NLib::SClassReflection* GetClassReflection() const override;                                         \
	static const NLib::SClassReflection* GetStaticClassReflection();                                                   \
                                                                                                                       \
private:

/**
 * @brief 检查对象是否为指定类型
 */
#define IS_A(Object, ClassName) ((Object) && (Object)->GetTypeInfo() == typeid(ClassName))

/**
 * @brief 安全的类型转换
 */
#define CAST(Object, ClassName) (IS_A(Object, ClassName) ? static_cast<ClassName*>(Object) : nullptr)

/**
 * @brief 检查类是否为指定类的子类
 */
#define IS_CHILD_OF(ChildClass, ParentClass) std::is_base_of_v<ParentClass, ChildClass>

// === 调试和开发宏 ===

/**
 * @brief 在开发模式下输出反射信息
 */
#ifdef NLIB_DEBUG_REFLECTION
#define NREFLECTION_LOG(Message) NLOG_DEBUG(Debug, "Reflection: {}", Message)
#else
#define NREFLECTION_LOG(Message)
#endif

/**
 * @brief 标记函数为已废弃
 */
#define DEPRECATED(Version, Message) [[deprecated("Deprecated in " Version ": " Message)]]

/**
 * @brief 标记实验性功能
 */
#define EXPERIMENTAL(Message) /* Experimental feature: Message */

/**
 * @brief 元数据提取宏（用于编译时处理）
 */
#define META(...) __VA_ARGS__

// === 代码生成辅助 ===

/**
 * @brief 包含生成的头文件的提醒宏
 * 实际使用中应该直接 #include "ClassName.generate.h"
 */
#define INCLUDE_GENERATED_FILE(ClassName) /* Remember to #include #ClassName ".generate.h" at the end of this file */

/**
 * @brief 检查生成的代码版本
 */
#define GENERATED_CODE_VERSION 1

/**
 * @brief 强制重新生成代码的宏
 */
#define FORCE_REGENERATE_CODE() static_assert(false, "请运行 NutHeaderTools 重新生成反射代码")

#ifndef GENERATED_CODE_VERSION_CHECK
#define GENERATED_CODE_VERSION_CHECK(Version)                                                                          \
	static_assert(GENERATED_CODE_VERSION >= Version, "生成的代码版本过旧，请运行 NutHeaderTools 重新生成")
#endif