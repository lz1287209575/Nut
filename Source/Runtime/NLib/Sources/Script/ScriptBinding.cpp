#include "ScriptBinding.h"

#include "Logging/LogCategory.h"

namespace NLib
{
// 全局Meta绑定管理器实例
CMetaScriptBindingManager GMetaScriptBindingManager;

// === CMetaScriptBindingManager 实现 ===

bool CMetaScriptBindingManager::Initialize()
{
	NLOG_SCRIPT(Info, "Initializing Meta Script Binding Manager...");

	ScanAndBindReflectionClasses();

	NLOG_SCRIPT(Info, "Meta Script Binding Manager initialized with {} class binders", ClassBinders.Size());
	return true;
}

void CMetaScriptBindingManager::ScanAndBindReflectionClasses()
{
	auto& Registry = CReflectionRegistry::GetInstance();

	// TODO: 这里需要反射注册表提供GetAllClasses()方法
	// 临时实现：手动扫描已知类

	NLOG_SCRIPT(Debug, "Scanning reflection registry for script-bindable classes...");

	// 当反射注册表提供遍历接口后，实现如下逻辑：
	/*
	auto AllClasses = Registry.GetAllClasses();
	for (const auto* ClassReflection : AllClasses)
	{
	    auto Binder = MakeShared<CMetaReflectionClassBinder>(ClassReflection);
	    if (Binder->ShouldBind())
	    {
	        CString ClassName(ClassReflection->Name);
	        ClassBinders.Add(ClassName, Binder);

	        NLOG_SCRIPT(Debug, "Registered script binding for class: {} (ScriptName: {})",
	                   ClassName.GetData(), Binder->GetScriptName().GetData());
	    }
	}
	*/

	NLOG_SCRIPT(Debug, "Reflection class scanning completed");
}

void CMetaScriptBindingManager::ApplyAllBindingsToContext(TSharedPtr<CScriptContext> Context) const
{
	if (!Context)
	{
		NLOG_SCRIPT(Error, "Cannot apply bindings to null script context");
		return;
	}

	NLOG_SCRIPT(Debug, "Applying {} class bindings to script context", ClassBinders.Size());

	int32_t AppliedCount = 0;
	for (const auto& BinderPair : ClassBinders)
	{
		if (BinderPair.Value->ShouldBind())
		{
			BinderPair.Value->ApplyToContext(Context);
			AppliedCount++;
		}
	}

	NLOG_SCRIPT(Info, "Applied {} script bindings to context", AppliedCount);
}

CScriptValue CMetaScriptBindingManager::WrapObject(NObject* Object, TSharedPtr<CScriptContext> Context) const
{
	if (!Object || !Context)
	{
		return CScriptValue();
	}

	const auto* ClassReflection = Object->GetClassReflection();
	if (!ClassReflection)
	{
		NLOG_SCRIPT(Warning, "Object has no reflection information, cannot wrap for script");
		return CScriptValue();
	}

	CString ClassName(ClassReflection->Name);
	auto FoundBinder = ClassBinders.Find(ClassName);
	if (FoundBinder && FoundBinder->Value->IsVisible())
	{
		NLOG_SCRIPT(Debug, "Wrapping object of class {} for script", ClassName.GetData());
		return FoundBinder->Value->CreateObjectWrapper(Object, Context);
	}

	NLOG_SCRIPT(Debug, "Class {} is not script-visible, cannot wrap object", ClassName.GetData());
	return CScriptValue();
}

} // namespace NLib