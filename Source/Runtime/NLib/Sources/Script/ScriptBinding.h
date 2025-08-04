#pragma once

#include "Containers/THashMap.h"
#include "Core/TString.h"
#include "Memory/Memory.h"
#include "Reflection/ReflectionRegistry.h"
#include "Reflection/ReflectionStructures.h"
#include "ScriptEngine.h"

#include <any>
#include <functional>

namespace NLib
{
/**
 * @brief 脚本绑定元数据标志
 * 这些标志通过NCLASS、NPROPERTY、NFUNCTION的meta参数设置
 */
enum class EScriptBindingFlags : uint32_t
{
	None = 0,
	ScriptCreatable = 1 << 0,   // 可在脚本中创建实例 - NCLASS(meta=(ScriptCreatable=true))
	ScriptVisible = 1 << 1,     // 脚本中可见 - NCLASS(meta=(ScriptVisible=true))
	ScriptReadOnly = 1 << 2,    // 脚本只读 - NPROPERTY(meta=(ScriptReadOnly=true))
	ScriptCallable = 1 << 3,    // 脚本可调用 - NFUNCTION(meta=(ScriptCallable=true))
	ScriptEvent = 1 << 4,       // 脚本事件 - NFUNCTION(meta=(ScriptEvent=true))
	ScriptOverridable = 1 << 5, // 脚本可重写 - NFUNCTION(meta=(ScriptOverridable=true))
	ScriptStatic = 1 << 6,      // 脚本静态函数 - NFUNCTION(meta=(ScriptStatic=true))
	ScriptOperator = 1 << 7,    // 脚本操作符 - NFUNCTION(meta=(ScriptOperator=true))
	ScriptProperty = 1 << 8,    // 脚本属性 - NPROPERTY(meta=(ScriptProperty=true))
	ScriptHidden = 1 << 9       // 脚本中隐藏 - meta=(ScriptHidden=true)
};

NLIB_DEFINE_ENUM_FLAG_OPERATORS(EScriptBindingFlags)

/**
 * @brief 脚本绑定元数据解析器
 * 从反射信息的meta数据中提取脚本绑定信息
 */
class CScriptMetadataParser
{
public:
	/**
	 * @brief 解析类的脚本绑定标志
	 */
	static EScriptBindingFlags ParseClassFlags(const SClassReflection* ClassReflection)
	{
		if (!ClassReflection)
			return EScriptBindingFlags::None;

		EScriptBindingFlags Flags = EScriptBindingFlags::None;

		// 从反射信息中解析meta标签
		// 注意：这里需要反射系统支持meta数据存储
		// 临时实现：基于类标志推断

		if (ClassReflection->HasFlag(EClassFlags::BlueprintType))
		{
			Flags |= EScriptBindingFlags::ScriptVisible;
		}

		if (ClassReflection->HasFlag(EClassFlags::Blueprintable))
		{
			Flags |= EScriptBindingFlags::ScriptCreatable;
		}

		// TODO: 实际实现需要解析meta字符串
		// 例如：解析 "ScriptCreatable=true,ScriptVisible=true" 这样的字符串

		return Flags;
	}

	/**
	 * @brief 解析属性的脚本绑定标志
	 */
	static EScriptBindingFlags ParsePropertyFlags(const SPropertyReflection* PropertyReflection)
	{
		if (!PropertyReflection)
			return EScriptBindingFlags::None;

		EScriptBindingFlags Flags = EScriptBindingFlags::None;

		if (PropertyReflection->HasFlag(EPropertyFlags::BlueprintReadWrite))
		{
			Flags |= EScriptBindingFlags::ScriptProperty;
		}
		else if (PropertyReflection->HasFlag(EPropertyFlags::BlueprintReadOnly))
		{
			Flags |= EScriptBindingFlags::ScriptProperty | EScriptBindingFlags::ScriptReadOnly;
		}

		// TODO: 解析meta数据
		return Flags;
	}

	/**
	 * @brief 解析函数的脚本绑定标志
	 */
	static EScriptBindingFlags ParseFunctionFlags(const SFunctionReflection* FunctionReflection)
	{
		if (!FunctionReflection)
			return EScriptBindingFlags::None;

		EScriptBindingFlags Flags = EScriptBindingFlags::None;

		if (FunctionReflection->HasFlag(EFunctionFlags::BlueprintCallable))
		{
			Flags |= EScriptBindingFlags::ScriptCallable;
		}

		if (FunctionReflection->HasFlag(EFunctionFlags::BlueprintImplementableEvent))
		{
			Flags |= EScriptBindingFlags::ScriptEvent | EScriptBindingFlags::ScriptOverridable;
		}

		if (FunctionReflection->HasFlag(EFunctionFlags::Static))
		{
			Flags |= EScriptBindingFlags::ScriptStatic;
		}

		// TODO: 解析meta数据
		return Flags;
	}

	/**
	 * @brief 检查是否应该绑定到脚本
	 */
	static bool ShouldBindToScript(EScriptBindingFlags Flags)
	{
		return (Flags & EScriptBindingFlags::ScriptVisible) != EScriptBindingFlags::None ||
		       (Flags & EScriptBindingFlags::ScriptCreatable) != EScriptBindingFlags::None ||
		       (Flags & EScriptBindingFlags::ScriptProperty) != EScriptBindingFlags::None ||
		       (Flags & EScriptBindingFlags::ScriptCallable) != EScriptBindingFlags::None;
	}

	/**
	 * @brief 获取脚本中的名称（可能与C++名称不同）
	 */
	static TString GetScriptName(const char* CppName, EScriptBindingFlags Flags)
	{
		// TODO: 从meta数据中解析ScriptName
		// 例如：NFUNCTION(meta=(ScriptName="MyFunction"))
		return TString(CppName);
	}

	/**
	 * @brief 获取脚本分类
	 */
	static TString GetScriptCategory(const char* Category, EScriptBindingFlags Flags)
	{
		// TODO: 从meta数据中解析ScriptCategory
		return Category ? TString(Category) : TString();
	}
};

/**
 * @brief 基于meta标签的类型转换器
 */
template <typename T>
class TMetaScriptTypeConverter : public IScriptTypeConverter
{
public:
	CScriptValue ToScriptValue(const std::any& CppValue) const override
	{
		try
		{
			const T& Value = std::any_cast<const T&>(CppValue);
			return ConvertToScript(Value);
		}
		catch (const std::bad_any_cast&)
		{
			return CScriptValue();
		}
	}

	std::any FromScriptValue(const CScriptValue& ScriptValue) const override
	{
		T Value;
		if (ConvertFromScript(ScriptValue, Value))
		{
			return std::make_any<T>(Value);
		}
		return std::any();
	}

	bool CanConvert(const CScriptValue& ScriptValue) const override
	{
		return IsValidScriptValue(ScriptValue);
	}

	const std::type_info& GetTypeInfo() const override
	{
		return typeid(T);
	}

	TString GetScriptTypeName() const override
	{
		// 可以从反射信息中获取meta指定的脚本类型名
		return GetScriptTypeNameForType<T>();
	}

protected:
	virtual CScriptValue ConvertToScript(const T& Value) const = 0;
	virtual bool ConvertFromScript(const CScriptValue& ScriptValue, T& Value) const = 0;
	virtual bool IsValidScriptValue(const CScriptValue& ScriptValue) const = 0;

	template <typename U>
	TString GetScriptTypeNameForType() const
	{
		if constexpr (std::is_same_v<U, bool>)
			return TEXT("boolean");
		else if constexpr (std::is_integral_v<U>)
			return TEXT("number");
		else if constexpr (std::is_floating_point_v<U>)
			return TEXT("number");
		else if constexpr (std::is_same_v<U, TString>)
			return TEXT("string");
		else
			return TEXT("object");
	}
};

/**
 * @brief 基于meta标签的反射函数包装器
 */
class CMetaReflectionFunctionWrapper : public CScriptFunction
{
	GENERATED_BODY()

public:
	CMetaReflectionFunctionWrapper(const SFunctionReflection* InFunctionReflection)
	    : FunctionReflection(InFunctionReflection),
	      BindingFlags(CScriptMetadataParser::ParseFunctionFlags(InFunctionReflection))
	{
		ScriptName = CScriptMetadataParser::GetScriptName(FunctionReflection ? FunctionReflection->Name : "unknown",
		                                                  BindingFlags);
	}

	SScriptExecutionResult Call(const TArray<CScriptValue, CMemoryManager>& Args) override
	{
		if (!FunctionReflection)
		{
			return SScriptExecutionResult(EScriptResult::FunctionNotFound, TEXT("Function reflection is null"));
		}

		// 检查脚本调用权限
		if ((BindingFlags & EScriptBindingFlags::ScriptCallable) == EScriptBindingFlags::None)
		{
			return SScriptExecutionResult(EScriptResult::SecurityError,
			                              TEXT("Function is not marked as ScriptCallable"));
		}

		// 处理静态函数调用
		if ((BindingFlags & EScriptBindingFlags::ScriptStatic) != EScriptBindingFlags::None)
		{
			TargetObject = nullptr;
		}

		return CallReflectionFunction(Args);
	}

	TString GetSignature() const override
	{
		if (!FunctionReflection)
			return TEXT("invalid_function()");

		return GenerateScriptSignature();
	}

	TString GetDocumentation() const override
	{
		if (!FunctionReflection || !FunctionReflection->ToolTip)
			return TString();

		return TString(FunctionReflection->ToolTip);
	}

	/**
	 * @brief 获取脚本中的函数名
	 */
	TString GetScriptName() const
	{
		return ScriptName;
	}

	/**
	 * @brief 设置目标对象
	 */
	void SetTargetObject(NObject* Object)
	{
		TargetObject = Object;
	}

	/**
	 * @brief 检查是否为静态函数
	 */
	bool IsStatic() const
	{
		return (BindingFlags & EScriptBindingFlags::ScriptStatic) != EScriptBindingFlags::None;
	}

	/**
	 * @brief 检查是否为可重写事件
	 */
	bool IsOverridable() const
	{
		return (BindingFlags & EScriptBindingFlags::ScriptOverridable) != EScriptBindingFlags::None;
	}

private:
	SScriptExecutionResult CallReflectionFunction(const TArray<CScriptValue, CMemoryManager>& Args)
	{
		// 实现反射函数调用逻辑
		// 这里需要类型转换、参数验证等
		try
		{
			// 转换参数
			std::vector<std::any> CppArgs;
			for (int32_t i = 0; i < Args.Size() && i < static_cast<int32_t>(FunctionReflection->Parameters.size()); ++i)
			{
				// 参数类型转换逻辑
				// TODO: 实现具体的转换
			}

			// 调用函数
			std::any Result = FunctionReflection->Invoker(TargetObject, CppArgs);

			// 转换返回值
			SScriptExecutionResult ExecutionResult(EScriptResult::Success);
			// TODO: 转换返回值
			return ExecutionResult;
		}
		catch (const std::exception& e)
		{
			return SScriptExecutionResult(EScriptResult::RuntimeError,
			                              TString(TEXT("Function execution failed: ")) + TString(e.what()));
		}
	}

	TString GenerateScriptSignature() const
	{
		TString Signature = TString(FunctionReflection->ReturnTypeName) + TEXT(" ") + ScriptName + TEXT("(");

		for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
		{
			if (i > 0)
				Signature += TEXT(", ");
			const auto& Param = FunctionReflection->Parameters[i];
			Signature += TString(Param.TypeName) + TEXT(" ") + TString(Param.Name);
		}

		Signature += TEXT(")");

		if (IsStatic())
		{
			Signature = TEXT("static ") + Signature;
		}

		return Signature;
	}

private:
	const SFunctionReflection* FunctionReflection = nullptr;
	EScriptBindingFlags BindingFlags = EScriptBindingFlags::None;
	TString ScriptName;
	NObject* TargetObject = nullptr;
};

/**
 * @brief 基于meta标签的属性访问器
 */
class CMetaReflectionPropertyAccessor
{
public:
	CMetaReflectionPropertyAccessor(const SPropertyReflection* InPropertyReflection)
	    : PropertyReflection(InPropertyReflection),
	      BindingFlags(CScriptMetadataParser::ParsePropertyFlags(InPropertyReflection))
	{
		ScriptName = CScriptMetadataParser::GetScriptName(PropertyReflection ? PropertyReflection->Name : "unknown",
		                                                  BindingFlags);
	}

	/**
	 * @brief 获取属性值
	 */
	CScriptValue GetValue(NObject* Object) const
	{
		if (!PropertyReflection || !Object)
			return CScriptValue();

		// 检查脚本访问权限
		if ((BindingFlags & EScriptBindingFlags::ScriptProperty) == EScriptBindingFlags::None)
		{
			NLOG_SCRIPT(Warning, "Property {} is not marked as ScriptProperty", PropertyReflection->Name);
			return CScriptValue();
		}

		try
		{
			std::any Value = PropertyReflection->Getter(Object);
			// TODO: 类型转换
			return CScriptValue();
		}
		catch (const std::exception& e)
		{
			NLOG_SCRIPT(Error, "Failed to get property {}: {}", PropertyReflection->Name, e.what());
			return CScriptValue();
		}
	}

	/**
	 * @brief 设置属性值
	 */
	bool SetValue(NObject* Object, const CScriptValue& Value) const
	{
		if (!PropertyReflection || !Object)
			return false;

		// 检查是否为只读
		if ((BindingFlags & EScriptBindingFlags::ScriptReadOnly) != EScriptBindingFlags::None)
		{
			NLOG_SCRIPT(Warning, "Attempting to set read-only script property: {}", PropertyReflection->Name);
			return false;
		}

		try
		{
			// TODO: 类型转换和设置
			return true;
		}
		catch (const std::exception& e)
		{
			NLOG_SCRIPT(Error, "Failed to set property {}: {}", PropertyReflection->Name, e.what());
			return false;
		}
	}

	/**
	 * @brief 获取脚本中的属性名
	 */
	TString GetScriptName() const
	{
		return ScriptName;
	}

	/**
	 * @brief 检查是否为只读属性
	 */
	bool IsReadOnly() const
	{
		return (BindingFlags & EScriptBindingFlags::ScriptReadOnly) != EScriptBindingFlags::None;
	}

private:
	const SPropertyReflection* PropertyReflection = nullptr;
	EScriptBindingFlags BindingFlags = EScriptBindingFlags::None;
	TString ScriptName;
};

/**
 * @brief 基于meta标签的类绑定器
 */
class CMetaReflectionClassBinder
{
public:
	CMetaReflectionClassBinder(const SClassReflection* InClassReflection)
	    : ClassReflection(InClassReflection),
	      BindingFlags(CScriptMetadataParser::ParseClassFlags(InClassReflection))
	{
		if (ClassReflection)
		{
			ScriptName = CScriptMetadataParser::GetScriptName(ClassReflection->Name, BindingFlags);
			GenerateBindings();
		}
	}

	/**
	 * @brief 检查类是否应该绑定到脚本
	 */
	bool ShouldBind() const
	{
		return CScriptMetadataParser::ShouldBindToScript(BindingFlags);
	}

	/**
	 * @brief 检查类是否可在脚本中创建
	 */
	bool IsCreatable() const
	{
		return (BindingFlags & EScriptBindingFlags::ScriptCreatable) != EScriptBindingFlags::None;
	}

	/**
	 * @brief 检查类是否在脚本中可见
	 */
	bool IsVisible() const
	{
		return (BindingFlags & EScriptBindingFlags::ScriptVisible) != EScriptBindingFlags::None;
	}

	/**
	 * @brief 应用绑定到脚本上下文
	 */
	void ApplyToContext(TSharedPtr<CScriptContext> Context) const
	{
		if (!Context || !ShouldBind())
			return;

		// 注册构造函数（如果可创建）
		if (IsCreatable() && ClassReflection->Constructor)
		{
			RegisterConstructor(Context);
		}

		// 注册静态方法
		for (const auto& MethodPair : StaticMethods)
		{
			Context->RegisterGlobalFunction(MethodPair.Key, MethodPair.Value);
		}

		// 类的实例方法和属性需要在创建对象时绑定
	}

	/**
	 * @brief 获取脚本中的类名
	 */
	TString GetScriptName() const
	{
		return ScriptName;
	}

	/**
	 * @brief 创建对象的脚本包装
	 */
	CScriptValue CreateObjectWrapper(NObject* Object, TSharedPtr<CScriptContext> Context) const
	{
		if (!Object || !Context || !IsVisible())
			return CScriptValue();

		// 创建脚本对象并绑定属性和方法
		auto ScriptObject = Context->GetEngine()->CreateObject();

		// 绑定实例方法
		for (const auto& MethodPair : InstanceMethods)
		{
			auto Method = MethodPair.Value;
			Method->SetTargetObject(Object);
			// 绑定到脚本对象
			// ScriptObject.SetObjectProperty(MethodPair.Key, ...);
		}

		// 绑定属性访问器
		for (const auto& PropertyPair : Properties)
		{
			// 创建属性的getter/setter
			// ScriptObject.SetObjectProperty(PropertyPair.Key, ...);
		}

		return ScriptObject;
	}

private:
	void GenerateBindings()
	{
		if (!ClassReflection)
			return;

		// 生成属性绑定
		for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
		{
			const auto& PropInfo = ClassReflection->Properties[i];
			auto Accessor = MakeShared<CMetaReflectionPropertyAccessor>(&PropInfo);

			if (CScriptMetadataParser::ShouldBindToScript(CScriptMetadataParser::ParsePropertyFlags(&PropInfo)))
			{
				Properties.Add(Accessor->GetScriptName(), Accessor);
			}
		}

		// 生成方法绑定
		for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
		{
			const auto& FuncInfo = ClassReflection->Functions[i];
			auto Wrapper = MakeShared<CMetaReflectionFunctionWrapper>(&FuncInfo);

			if (CScriptMetadataParser::ShouldBindToScript(CScriptMetadataParser::ParseFunctionFlags(&FuncInfo)))
			{
				if (Wrapper->IsStatic())
				{
					// 静态方法注册为全局函数
					TString GlobalName = ScriptName + TEXT("_") + Wrapper->GetScriptName();
					StaticMethods.Add(GlobalName, Wrapper);
				}
				else
				{
					// 实例方法
					InstanceMethods.Add(Wrapper->GetScriptName(), Wrapper);
				}
			}
		}
	}

	void RegisterConstructor(TSharedPtr<CScriptContext> Context) const
	{
		auto ConstructorWrapper = MakeShared<CScriptFunctionWrapper>(
		    SScriptFunctionSignature(ScriptName),
		    [this](const TArray<CScriptValue, CMemoryManager>& Args) -> SScriptExecutionResult {
			    if (ClassReflection && ClassReflection->Constructor)
			    {
				    NObject* NewObject = ClassReflection->Constructor();
				    if (NewObject)
				    {
					    SScriptExecutionResult Result(EScriptResult::Success);
					    // TODO: 包装为脚本对象
					    return Result;
				    }
			    }
			    return SScriptExecutionResult(EScriptResult::RuntimeError, TEXT("Failed to create object"));
		    });

		Context->RegisterGlobalFunction(ScriptName, ConstructorWrapper);
	}

private:
	const SClassReflection* ClassReflection = nullptr;
	EScriptBindingFlags BindingFlags = EScriptBindingFlags::None;
	TString ScriptName;

	THashMap<TString, TSharedPtr<CMetaReflectionPropertyAccessor>, CMemoryManager> Properties;
	THashMap<TString, TSharedPtr<CMetaReflectionFunctionWrapper>, CMemoryManager> InstanceMethods;
	THashMap<TString, TSharedPtr<CMetaReflectionFunctionWrapper>, CMemoryManager> StaticMethods;
};

/**
 * @brief Meta标签驱动的自动绑定管理器
 */
class CMetaScriptBindingManager
{
public:
	/**
	 * @brief 初始化绑定管理器
	 */
	bool Initialize()
	{
		ScanAndBindReflectionClasses();
		return true;
	}

	/**
	 * @brief 扫描反射注册表中标记的类
	 */
	void ScanAndBindReflectionClasses()
	{
		auto& Registry = CReflectionRegistry::GetInstance();

		// TODO: 需要反射注册表提供遍历接口
		// 扫描所有注册的类，检查meta标签
		// 只绑定标记了ScriptCreatable、ScriptVisible等的类
	}

	/**
	 * @brief 应用所有符合meta标签要求的绑定
	 */
	void ApplyAllBindingsToContext(TSharedPtr<CScriptContext> Context) const
	{
		if (!Context)
			return;

		for (const auto& BinderPair : ClassBinders)
		{
			if (BinderPair.Value->ShouldBind())
			{
				BinderPair.Value->ApplyToContext(Context);
			}
		}
	}

	/**
	 * @brief 包装对象为脚本值（仅限标记的类）
	 */
	CScriptValue WrapObject(NObject* Object, TSharedPtr<CScriptContext> Context) const
	{
		if (!Object || !Context)
			return CScriptValue();

		const auto* ClassReflection = Object->GetClassReflection();
		if (!ClassReflection)
			return CScriptValue();

		TString ClassName(ClassReflection->Name);
		auto FoundBinder = ClassBinders.Find(ClassName);
		if (FoundBinder && FoundBinder->Value->IsVisible())
		{
			return FoundBinder->Value->CreateObjectWrapper(Object, Context);
		}

		return CScriptValue();
	}

private:
	THashMap<TString, TSharedPtr<CMetaReflectionClassBinder>, CMemoryManager> ClassBinders;
};

/**
 * @brief 全局Meta绑定管理器
 */
extern CMetaScriptBindingManager GMetaScriptBindingManager;

} // namespace NLib

// === Meta标签辅助宏 ===

/**
 * @brief 标记类可在脚本中创建和访问
 * 使用方式：NCLASS(meta=(ScriptCreatable=true, ScriptVisible=true))
 */
#define SCRIPT_CLASS_META() ScriptCreatable = true, ScriptVisible = true

/**
 * @brief 标记类只在脚本中可见，不可创建
 * 使用方式：NCLASS(meta=(ScriptVisible=true))
 */
#define SCRIPT_VISIBLE_META() ScriptVisible = true

/**
 * @brief 标记属性可在脚本中访问
 * 使用方式：NPROPERTY(BlueprintReadWrite, meta=(ScriptProperty=true))
 */
#define SCRIPT_PROPERTY_META() ScriptProperty = true

/**
 * @brief 标记属性在脚本中只读
 * 使用方式：NPROPERTY(BlueprintReadOnly, meta=(ScriptProperty=true, ScriptReadOnly=true))
 */
#define SCRIPT_READONLY_META() ScriptProperty = true, ScriptReadOnly = true

/**
 * @brief 标记函数可在脚本中调用
 * 使用方式：NFUNCTION(BlueprintCallable, meta=(ScriptCallable=true))
 */
#define SCRIPT_FUNCTION_META() ScriptCallable = true

/**
 * @brief 标记静态函数
 * 使用方式：NFUNCTION(BlueprintCallable, meta=(ScriptCallable=true, ScriptStatic=true))
 */
#define SCRIPT_STATIC_META() ScriptCallable = true, ScriptStatic = true

/**
 * @brief 标记可重写的脚本事件
 * 使用方式：NFUNCTION(BlueprintImplementableEvent, meta=(ScriptEvent=true, ScriptOverridable=true))
 */
#define SCRIPT_EVENT_META() ScriptEvent = true, ScriptOverridable = true