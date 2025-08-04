#pragma once

/**
 * @file Script.h
 * @brief NLib脚本模块主头文件
 *
 * 提供完整的脚本功能，包括：
 * - 多语言脚本引擎支持（Lua, LuaForge, Python, TypeScript, C#, NBP）
 * - 基于NCLASS反射的自动绑定系统
 * - 脚本上下文管理和隔离
 * - 类型安全的脚本调用
 * - 高性能的脚本执行环境
 */

#include "LuaEngine.h"
#include "ReflectionScriptBinding.h"
#include "ScriptBinding.h"
#include "ScriptEngine.h"
#include "ScriptManager.h"

namespace NLib
{
/**
 * @brief 脚本模块命名空间
 */
namespace Script
{
// === 核心类型别名 ===
using Engine = CScriptEngine;
using Context = CScriptContext;
using Module = CScriptModule;
using Value = CScriptValue;
using Function = CScriptFunction;
using Manager = CScriptManager;

using Config = SScriptConfig;
using Result = SScriptExecutionResult;
using Language = EScriptLanguage;
using Flags = EScriptContextFlags;

// === 引擎类型别名 ===
using LuaEngine = CLuaScriptEngine;
using LuaContext = CLuaScriptContext;
using LuaModule = CLuaScriptModule;
using LuaValue = CLuaScriptValue;

// === 绑定系统别名 ===
using BindingManager = CMetaScriptBindingManager;
using TypeConverter = IScriptTypeConverter;
using ClassBinder = CMetaReflectionClassBinder;
using FunctionWrapper = CMetaReflectionFunctionWrapper;
using PropertyAccessor = CMetaReflectionPropertyAccessor;

// === 便利函数 ===

/**
 * @brief 获取脚本管理器实例
 */
inline Manager& GetManager()
{
	return CScriptManager::GetInstance();
}

/**
 * @brief 初始化脚本模块
 */
inline bool Initialize()
{
	auto& Manager = GetManager();
	if (!Manager.Initialize())
	{
		return false;
	}

	// 初始化自动绑定系统
	GMetaScriptBindingManager.Initialize();

	// 注册反射绑定生成器
	auto& BindingRegistry = CScriptBindingRegistry::GetInstance();
	BindingRegistry.RegisterGenerator(EScriptLanguage::Lua, MakeShared<CLuaBindingGenerator>());
	BindingRegistry.RegisterGenerator(EScriptLanguage::TypeScript, MakeShared<CTypeScriptBindingGenerator>());
	BindingRegistry.RegisterGenerator(EScriptLanguage::Python, MakeShared<CPythonBindingGenerator>());
	BindingRegistry.RegisterGenerator(EScriptLanguage::CSharp, MakeShared<CCSharpBindingGenerator>());

	return true;
}

/**
 * @brief 关闭脚本模块
 */
inline void Shutdown()
{
	GetManager().Shutdown();
}

/**
 * @brief 创建脚本上下文
 */
inline TSharedPtr<Context> CreateContext(Language ScriptLanguage, Flags ContextFlags = Flags::None)
{
	return GetManager().CreateContext(ScriptLanguage, ContextFlags);
}

/**
 * @brief 创建带配置的脚本上下文
 */
inline TSharedPtr<Context> CreateContext(const Config& ScriptConfig)
{
	return GetManager().CreateContext(ScriptConfig);
}

/**
 * @brief 销毁脚本上下文
 */
inline void DestroyContext(TSharedPtr<Context> ScriptContext)
{
	GetManager().DestroyContext(ScriptContext);
}

/**
 * @brief 执行脚本文件
 */
inline Result ExecuteFile(Language ScriptLanguage, const TString& FilePath, const Config& ScriptConfig = Config())
{
	return GetManager().ExecuteFile(ScriptLanguage, FilePath, ScriptConfig);
}

/**
 * @brief 执行脚本字符串
 */
inline Result ExecuteString(Language ScriptLanguage, const TString& Code, const Config& ScriptConfig = Config())
{
	return GetManager().ExecuteString(ScriptLanguage, Code, ScriptConfig);
}

/**
 * @brief 检查脚本语法
 */
inline Result CheckSyntax(Language ScriptLanguage, const TString& Code)
{
	return GetManager().CheckSyntax(ScriptLanguage, Code);
}

/**
 * @brief 编译脚本文件
 */
inline Result CompileFile(Language ScriptLanguage, const TString& FilePath, const TString& OutputPath = TString())
{
	return GetManager().CompileFile(ScriptLanguage, FilePath, OutputPath);
}

/**
 * @brief 注册全局函数
 */
inline void RegisterGlobalFunction(const TString& Name, TSharedPtr<Function> ScriptFunction)
{
	GetManager().RegisterGlobalFunction(Name, ScriptFunction);
}

/**
 * @brief 注册全局对象
 */
inline void RegisterGlobalObject(const TString& Name, const Value& Object)
{
	GetManager().RegisterGlobalObject(Name, Object);
}

/**
 * @brief 注册全局常量
 */
inline void RegisterGlobalConstant(const TString& Name, const Value& ConstantValue)
{
	GetManager().RegisterGlobalConstant(Name, ConstantValue);
}

/**
 * @brief 应用反射绑定到上下文
 */
inline void ApplyReflectionBindings(TSharedPtr<Context> ScriptContext)
{
	GMetaScriptBindingManager.ApplyAllBindingsToContext(ScriptContext);
}

/**
 * @brief 包装对象为脚本值
 */
inline Value WrapObject(NObject* Object, TSharedPtr<Context> ScriptContext)
{
	return GMetaScriptBindingManager.WrapObject(Object, ScriptContext);
}

/**
 * @brief 检查语言是否支持
 */
inline bool IsLanguageSupported(Language ScriptLanguage)
{
	return GetManager().IsLanguageSupported(ScriptLanguage);
}

/**
 * @brief 获取支持的语言列表
 */
inline TArray<Language, CMemoryManager> GetSupportedLanguages()
{
	return GetManager().GetSupportedLanguages();
}

/**
 * @brief 获取脚本统计信息
 */
inline SScriptStatistics GetStatistics()
{
	return GetManager().GetStatistics();
}

/**
 * @brief 强制垃圾回收
 */
inline void CollectGarbage()
{
	GetManager().CollectGarbage();
}

/**
 * @brief 获取总内存使用量
 */
inline uint64_t GetTotalMemoryUsage()
{
	return GetManager().GetTotalMemoryUsage();
}

// === Lua特有便利函数 ===

namespace Lua
{
/**
 * @brief 创建Lua引擎
 */
inline TSharedPtr<LuaEngine> CreateEngine()
{
	return MakeShared<CLuaScriptEngine>();
}

/**
 * @brief 检查Lua是否可用
 */
inline bool IsAvailable()
{
	return CLuaScriptEngine::IsLuaAvailable();
}

/**
 * @brief 获取Lua版本
 */
inline TString GetVersion()
{
	return CLuaScriptEngine::GetLuaVersionString();
}

/**
 * @brief 创建Lua上下文
 */
inline TSharedPtr<LuaContext> CreateLuaContext(Flags ContextFlags = Flags::EnableSandbox)
{
	Config LuaConfig(Language::Lua);
	LuaConfig.Flags = ContextFlags;
	auto Context = GetManager().CreateContext(LuaConfig);
	return std::static_pointer_cast<CLuaScriptContext>(Context);
}

/**
 * @brief 执行Lua脚本文件
 */
inline Result ExecuteLuaFile(const TString& FilePath)
{
	return ExecuteFile(Language::Lua, FilePath);
}

/**
 * @brief 执行Lua脚本字符串
 */
inline Result ExecuteLuaString(const TString& Code)
{
	return ExecuteString(Language::Lua, Code);
}

/**
 * @brief 检查Lua脚本语法
 */
inline Result CheckLuaSyntax(const TString& Code)
{
	return CheckSyntax(Language::Lua, Code);
}
} // namespace Lua

// === 高级脚本执行器 ===

/**
 * @brief 脚本执行器类
 * 提供更高级的脚本执行功能
 */
class CScriptExecutor
{
public:
	explicit CScriptExecutor(Language ScriptLanguage)
	    : Language(ScriptLanguage)
	{
		Context = CreateContext(ScriptLanguage, Flags::EnableSandbox | Flags::EnableTimeout);
		if (Context)
		{
			ApplyReflectionBindings(Context);
		}
	}

	~CScriptExecutor()
	{
		if (Context)
		{
			DestroyContext(Context);
		}
	}

	/**
	 * @brief 执行脚本并返回结果
	 */
	template <typename ReturnType>
	TResult<ReturnType> Execute(const TString& Code)
	{
		if (!Context)
		{
			return TResult<ReturnType>::CreateError(TEXT("Script context not available"));
		}

		auto Result = Context->ExecuteString(Code);
		if (!Result.IsSuccess())
		{
			return TResult<ReturnType>::CreateError(Result.ErrorMessage);
		}

		// 尝试转换返回值
		try
		{
			if constexpr (std::is_same_v<ReturnType, void>)
			{
				return TResult<ReturnType>::CreateSuccess();
			}
			else
			{
				ReturnType Value = Result.ReturnValue.As<ReturnType>();
				return TResult<ReturnType>::CreateSuccess(Value);
			}
		}
		catch (const std::exception& e)
		{
			return TResult<ReturnType>::CreateError(TString(TEXT("Type conversion failed: ")) + TString(e.what()));
		}
	}

	/**
	 * @brief 调用脚本函数
	 */
	template <typename ReturnType, typename... Args>
	TResult<ReturnType> CallFunction(const TString& FunctionName, Args&&... Arguments)
	{
		if (!Context)
		{
			return TResult<ReturnType>::CreateError(TEXT("Script context not available"));
		}

		auto Function = Context->GetGlobal(FunctionName);
		if (!Function.IsFunction())
		{
			return TResult<ReturnType>::CreateError(TString(TEXT("Function not found: ")) + FunctionName);
		}

		TArray<Value, CMemoryManager> Args;
		(Args.Add(CreateScriptValue(std::forward<Args>(Arguments))), ...);

		auto Result = Function.CallFunction(Args);
		if (!Result.IsSuccess())
		{
			return TResult<ReturnType>::CreateError(Result.ErrorMessage);
		}

		try
		{
			if constexpr (std::is_same_v<ReturnType, void>)
			{
				return TResult<ReturnType>::CreateSuccess();
			}
			else
			{
				ReturnType Value = Result.ReturnValue.As<ReturnType>();
				return TResult<ReturnType>::CreateSuccess(Value);
			}
		}
		catch (const std::exception& e)
		{
			return TResult<ReturnType>::CreateError(TString(TEXT("Type conversion failed: ")) + TString(e.what()));
		}
	}

	/**
	 * @brief 设置全局变量
	 */
	template <typename T>
	void SetGlobal(const TString& Name, const T& Value)
	{
		if (Context)
		{
			Context->SetGlobal(Name, CreateScriptValue(Value));
		}
	}

	/**
	 * @brief 获取全局变量
	 */
	template <typename T>
	TResult<T> GetGlobal(const TString& Name)
	{
		if (!Context)
		{
			return TResult<T>::CreateError(TEXT("Script context not available"));
		}

		auto Value = Context->GetGlobal(Name);
		try
		{
			T Result = Value.As<T>();
			return TResult<T>::CreateSuccess(Result);
		}
		catch (const std::exception& e)
		{
			return TResult<T>::CreateError(TString(TEXT("Type conversion failed: ")) + TString(e.what()));
		}
	}

	/**
	 * @brief 获取上下文
	 */
	TSharedPtr<Context> GetContext() const
	{
		return Context;
	}

private:
	template <typename T>
	Value CreateScriptValue(const T& CppValue)
	{
		// 需要实现类型转换
		// 这里简化处理
		return Value();
	}

private:
	Language Language;
	TSharedPtr<Context> Context;
};

// === 便利类型别名 ===
using Executor = CScriptExecutor;
using LuaExecutor = CScriptExecutor;
} // namespace Script

// === 全局便利函数 ===

/**
 * @brief 初始化脚本系统
 */
inline bool InitializeScriptSystem()
{
	return Script::Initialize();
}

/**
 * @brief 关闭脚本系统
 */
inline void ShutdownScriptSystem()
{
	Script::Shutdown();
}

/**
 * @brief 执行Lua脚本文件
 */
inline SScriptExecutionResult ExecuteLuaFile(const TString& FilePath)
{
	return Script::Lua::ExecuteLuaFile(FilePath);
}

/**
 * @brief 执行Lua脚本字符串
 */
inline SScriptExecutionResult ExecuteLuaString(const TString& Code)
{
	return Script::Lua::ExecuteLuaString(Code);
}

/**
 * @brief 创建脚本执行器
 */
inline TSharedPtr<Script::Executor> CreateScriptExecutor(EScriptLanguage Language)
{
	return MakeShared<Script::CScriptExecutor>(Language);
}

/**
 * @brief 创建Lua执行器
 */
inline TSharedPtr<Script::LuaExecutor> CreateLuaExecutor()
{
	return MakeShared<Script::CScriptExecutor>(EScriptLanguage::Lua);
}

} // namespace NLib

// === 脚本绑定快速设置宏 ===

/**
 * @brief 快速注册脚本可创建的类
 * 使用方式：在类声明前加上这个宏
 */
#define SCRIPT_CLASS(ClassName)                                                                                        \
	NCLASS(meta = (ScriptCreatable = true, ScriptVisible = true))                                                      \
	class ClassName

/**
 * @brief 快速注册脚本可见的类
 */
#define SCRIPT_VISIBLE_CLASS(ClassName)                                                                                \
	NCLASS(meta = (ScriptVisible = true))                                                                              \
	class ClassName

/**
 * @brief 快速注册脚本属性
 */
#define SCRIPT_PROPERTY(Type, PropertyName)                                                                            \
	NPROPERTY(BlueprintReadWrite, meta = (ScriptProperty = true))                                                      \
	Type PropertyName

/**
 * @brief 快速注册脚本只读属性
 */
#define SCRIPT_READONLY_PROPERTY(Type, PropertyName)                                                                   \
	NPROPERTY(BlueprintReadOnly, meta = (ScriptProperty = true, ScriptReadOnly = true))                                \
	Type PropertyName

/**
 * @brief 快速注册脚本函数
 */
#define SCRIPT_FUNCTION(ReturnType, FunctionName, ...)                                                                 \
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable = true))                                                       \
	ReturnType FunctionName(__VA_ARGS__)

/**
 * @brief 快速注册脚本静态函数
 */
#define SCRIPT_STATIC_FUNCTION(ReturnType, FunctionName, ...)                                                          \
	NFUNCTION(BlueprintCallable, meta = (ScriptCallable = true, ScriptStatic = true))                                  \
	static ReturnType FunctionName(__VA_ARGS__)

/**
 * @brief 快速注册脚本事件
 */
#define SCRIPT_EVENT(ReturnType, EventName, ...)                                                                       \
	NFUNCTION(BlueprintImplementableEvent, meta = (ScriptEvent = true, ScriptOverridable = true))                      \
	ReturnType EventName(__VA_ARGS__)