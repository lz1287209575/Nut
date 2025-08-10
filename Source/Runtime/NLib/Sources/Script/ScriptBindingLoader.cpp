#include "ScriptBindingLoader.h"

#include "IO/FileSystem.h"
#include "IO/Path.h"
#include "Logging/LogCategory.h"

#include <mutex>

namespace NLib
{
// === CScriptBindingLoader 实现 ===

CScriptBindingLoader& CScriptBindingLoader::GetInstance()
{
	static CScriptBindingLoader Instance;
	return Instance;
}

void CScriptBindingLoader::RegisterClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName)
	{
		NLOG_SCRIPT(Error, "Cannot register class binding with null name");
		return;
	}

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString ClassNameStr(ClassName);
	ClassBindings.Insert(ClassNameStr, BindingInfo);

	NLOG_SCRIPT(Debug,
	            "Registered script binding for class: {} (ScriptName: {})",
	            ClassName,
	            BindingInfo.ScriptName.IsEmpty() ? ClassName : BindingInfo.ScriptName.GetData());
}

void CScriptBindingLoader::RegisterFunctionBinding(const char* ClassName,
                                                   const char* FunctionName,
                                                   const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName || !FunctionName)
	{
		NLOG_SCRIPT(Error, "Cannot register function binding with null names");
		return;
	}

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString Key = GenerateBindingKey(ClassName, FunctionName);
	FunctionBindings.Insert(Key, BindingInfo);

	NLOG_SCRIPT(Debug,
	            "Registered script binding for function: {}::{} (ScriptName: {})",
	            ClassName,
	            FunctionName,
	            BindingInfo.ScriptName.IsEmpty() ? FunctionName : BindingInfo.ScriptName.GetData());
}

void CScriptBindingLoader::RegisterPropertyBinding(const char* ClassName,
                                                   const char* PropertyName,
                                                   const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName || !PropertyName)
	{
		NLOG_SCRIPT(Error, "Cannot register property binding with null names");
		return;
	}

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString Key = GenerateBindingKey(ClassName, PropertyName);
	PropertyBindings.Insert(Key, BindingInfo);

	NLOG_SCRIPT(Debug,
	            "Registered script binding for property: {}::{} (ScriptName: {})",
	            ClassName,
	            PropertyName,
	            BindingInfo.ScriptName.IsEmpty() ? PropertyName : BindingInfo.ScriptName.GetData());
}

void CScriptBindingLoader::RegisterEnumBinding(const char* EnumName, const SScriptBindingInfo& BindingInfo)
{
	if (!EnumName)
	{
		NLOG_SCRIPT(Error, "Cannot register enum binding with null name");
		return;
	}

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString EnumNameStr(EnumName);
	EnumBindings.Insert(EnumNameStr, BindingInfo);

	NLOG_SCRIPT(Debug,
	            "Registered script binding for enum: {} (ScriptName: {})",
	            EnumName,
	            BindingInfo.ScriptName.IsEmpty() ? EnumName : BindingInfo.ScriptName.GetData());
}

const SScriptBindingInfo* CScriptBindingLoader::GetClassBindingInfo(const char* ClassName) const
{
	if (!ClassName)
		return nullptr;

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString ClassNameStr(ClassName);
	auto Found = ClassBindings.Find(ClassNameStr);
	return Found;
}

const SScriptBindingInfo* CScriptBindingLoader::GetFunctionBindingInfo(const char* ClassName,
                                                                       const char* FunctionName) const
{
	if (!ClassName || !FunctionName)
		return nullptr;

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString Key = GenerateBindingKey(ClassName, FunctionName);
	auto Found = FunctionBindings.Find(Key);
	return Found;
}

const SScriptBindingInfo* CScriptBindingLoader::GetPropertyBindingInfo(const char* ClassName,
                                                                       const char* PropertyName) const
{
	if (!ClassName || !PropertyName)
		return nullptr;

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString Key = GenerateBindingKey(ClassName, PropertyName);
	auto Found = PropertyBindings.Find(Key);
	return Found;
}

const SScriptBindingInfo* CScriptBindingLoader::GetEnumBindingInfo(const char* EnumName) const
{
	if (!EnumName)
		return nullptr;

	std::lock_guard<std::mutex> Lock(BindingMutex);

	CString EnumNameStr(EnumName);
	auto Found = EnumBindings.Find(EnumNameStr);
	return Found;
}

bool CScriptBindingLoader::LoadScriptBindings(EScriptLanguage Language,
                                              TSharedPtr<CScriptContext> Context,
                                              const CString& BindingDirectory)
{
	if (!Context)
	{
		NLOG_SCRIPT(Error, "Cannot load script bindings with null context");
		return false;
	}

	if (!NFileSystem::IsDirectory(NPath(BindingDirectory)))
	{
		NLOG_SCRIPT(Error, "Script binding directory does not exist: {}", BindingDirectory.GetData());
		return false;
	}

	switch (Language)
	{
	case EScriptLanguage::Lua:
		return LoadLuaBindings(Context, BindingDirectory);

	case EScriptLanguage::TypeScript:
		return LoadTypeScriptBindings(Context, BindingDirectory);

	default:
		NLOG_SCRIPT(Warning, "Script binding loading not implemented for language: {}", static_cast<int>(Language));
		return false;
	}
}

bool CScriptBindingLoader::LoadLuaBindings(TSharedPtr<CScriptContext> Context, const CString& BindingDirectory)
{
	NLOG_SCRIPT(Info, "Loading Lua script bindings from: {}", BindingDirectory.GetData());

	// 查找所有.lua绑定文件
	NPath LuaBindingDir = NPath::Combine(BindingDirectory, TEXT("Lua"));
	if (!NFileSystem::IsDirectory(LuaBindingDir))
	{
		NLOG_SCRIPT(Warning, "Lua binding directory not found: {}", LuaBindingDir.ToString().GetData());
		return false;
	}

	// 首先加载NLib核心API
	NPath NLibApiFile = NPath::Combine(LuaBindingDir, TEXT("NLibAPI.lua"));
	if (NFileSystem::IsFile(NLibApiFile))
	{
		if (!LoadBindingFile(NLibApiFile.ToString(), EScriptLanguage::Lua, Context))
		{
			NLOG_SCRIPT(Error, "Failed to load NLib API file: {}", NLibApiFile.ToString().GetData());
			return false;
		}
	}

	// 加载所有类绑定文件
	auto LuaFiles = NFileSystem::FindFiles(LuaBindingDir, TEXT("*.lua"));
	int32_t LoadedCount = 0;

	for (const auto& FilePath : LuaFiles)
	{
		// 跳过已经加载的NLibAPI.lua
		if (FilePath.GetFileName() == TEXT("NLibAPI.lua"))
			continue;

		if (LoadBindingFile(FilePath.ToString(), EScriptLanguage::Lua, Context))
		{
			LoadedCount++;
		}
		else
		{
			NLOG_SCRIPT(Warning, "Failed to load Lua binding file: {}", FilePath.GetData());
		}
	}

	NLOG_SCRIPT(Info, "Loaded {} Lua binding files", LoadedCount);
	return LoadedCount > 0;
}

bool CScriptBindingLoader::LoadTypeScriptBindings(TSharedPtr<CScriptContext> Context, const CString& BindingDirectory)
{
	NLOG_SCRIPT(Info, "Loading TypeScript type definitions from: {}", BindingDirectory.GetData());

	// TypeScript绑定主要是类型定义文件，不需要在运行时加载
	// 这里主要是验证文件存在性
	NPath TypeScriptBindingDir = NPath::Combine(BindingDirectory, TEXT("TypeScript"));
	if (!NFileSystem::IsDirectory(TypeScriptBindingDir))
	{
		NLOG_SCRIPT(Warning, "TypeScript binding directory not found: {}", TypeScriptBindingDir.ToString().GetData());
		return false;
	}

	auto TypeScriptFiles = NFileSystem::FindFiles(TypeScriptBindingDir, TEXT("*.d.ts"));
	NLOG_SCRIPT(Info, "Found {} TypeScript definition files", TypeScriptFiles.Size());

	return TypeScriptFiles.Size() > 0;
}

TArray<const SClassReflection*, CMemoryManager> CScriptBindingLoader::GetScriptBindableClasses() const
{
	TArray<const SClassReflection*, CMemoryManager> Result;

	std::lock_guard<std::mutex> Lock(BindingMutex);
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();

	// 遍历所有已注册的类绑定
	for (const auto& ClassBindingPair : ClassBindings)
	{
		const CString& ClassName = ClassBindingPair.first;
		const SScriptBindingInfo& BindingInfo = ClassBindingPair.second;

		if (BindingInfo.ShouldBind())
		{
			const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName.GetData());
			if (ClassReflection)
			{
				Result.PushBack(ClassReflection);
			}
			else
			{
				NLOG_SCRIPT(
				    Warning, "Script bindable class '{}' not found in reflection registry", ClassName.GetData());
			}
		}
	}

	return Result;
}

TArray<const SClassReflection*, CMemoryManager> CScriptBindingLoader::GetClassesForLanguage(
    EScriptLanguage Language) const
{
	TArray<const SClassReflection*, CMemoryManager> Result;

	std::lock_guard<std::mutex> Lock(BindingMutex);
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();

	// 遍历所有已注册的类绑定
	for (const auto& ClassBindingPair : ClassBindings)
	{
		const CString& ClassName = ClassBindingPair.first;
		const SScriptBindingInfo& BindingInfo = ClassBindingPair.second;

		if (BindingInfo.ShouldBind() && BindingInfo.SupportsLanguage(Language))
		{
			const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName.GetData());
			if (ClassReflection)
			{
				Result.PushBack(ClassReflection);
			}
		}
	}

	return Result;
}

NObject* CScriptBindingLoader::CreateScriptObject(const char* ClassName,
                                                  const TArray<TSharedPtr<NScriptValue>, CMemoryManager>& Args)
{
	if (!ClassName)
	{
		NLOG_SCRIPT(Error, "Cannot create script object with null class name");
		return nullptr;
	}

	// 检查类是否支持脚本创建
	const SScriptBindingInfo* BindingInfo = GetClassBindingInfo(ClassName);
	if (!BindingInfo || !BindingInfo->bScriptCreatable)
	{
		NLOG_SCRIPT(Error, "Class '{}' is not script creatable", ClassName);
		return nullptr;
	}

	// 通过反射系统创建对象
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();
	NObject* Object = ReflectionRegistry.CreateObject(ClassName);

	if (Object)
	{
		NLOG_SCRIPT(Debug, "Created script object of class: {}", ClassName);
	}
	else
	{
		NLOG_SCRIPT(Error, "Failed to create script object of class: {}", ClassName);
	}

	return Object;
}

TSharedPtr<NScriptValue> CScriptBindingLoader::CallScriptFunction(NObject* Object,
                                                                  const char* ClassName,
                                                                  const char* FunctionName,
                                                                  const TArray<TSharedPtr<NScriptValue>, CMemoryManager>& Args)
{
	if (!ClassName || !FunctionName)
	{
		NLOG_SCRIPT(Error, "Cannot call script function with null names");
		return nullptr;
	}

	// 检查函数是否支持脚本调用
	const SScriptBindingInfo* BindingInfo = GetFunctionBindingInfo(ClassName, FunctionName);
	if (!BindingInfo || !BindingInfo->bScriptCallable)
	{
		NLOG_SCRIPT(Error, "Function '{}::{}' is not script callable", ClassName, FunctionName);
		return nullptr;
	}

	// 通过反射系统调用函数
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();
	const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName);
	if (!ClassReflection)
	{
		NLOG_SCRIPT(Error, "Class '{}' not found in reflection registry", ClassName);
		return nullptr;
	}

	const SFunctionReflection* FunctionReflection = ClassReflection->FindFunction(FunctionName);
	if (!FunctionReflection)
	{
		NLOG_SCRIPT(Error, "Function '{}' not found in class '{}'", FunctionName, ClassName);
		return nullptr;
	}

	// TODO: 实现实际的函数调用
	NLOG_SCRIPT(Debug, "Called script function: {}::{}", ClassName, FunctionName);

	return nullptr;
}

TSharedPtr<NScriptValue> CScriptBindingLoader::GetScriptProperty(NObject* Object, const char* ClassName, const char* PropertyName)
{
	if (!Object || !ClassName || !PropertyName)
	{
		NLOG_SCRIPT(Error, "Cannot get script property with null parameters");
		return nullptr;
	}

	// 检查属性是否支持脚本读取
	const SScriptBindingInfo* BindingInfo = GetPropertyBindingInfo(ClassName, PropertyName);
	if (!BindingInfo || !BindingInfo->bScriptReadable)
	{
		NLOG_SCRIPT(Error, "Property '{}::{}' is not script readable", ClassName, PropertyName);
		return nullptr;
	}

	// 通过反射系统获取属性值
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();
	const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName);
	if (!ClassReflection)
	{
		NLOG_SCRIPT(Error, "Class '{}' not found in reflection registry", ClassName);
		return nullptr;
	}

	const SPropertyReflection* PropertyReflection = ClassReflection->FindProperty(PropertyName);
	if (!PropertyReflection)
	{
		NLOG_SCRIPT(Error, "Property '{}' not found in class '{}'", PropertyName, ClassName);
		return nullptr;
	}

	// TODO: 实现实际的属性读取
	NLOG_SCRIPT(Debug, "Got script property: {}::{}", ClassName, PropertyName);

	return nullptr;
}

bool CScriptBindingLoader::SetScriptProperty(NObject* Object,
                                             const char* ClassName,
                                             const char* PropertyName,
                                             const TSharedPtr<NScriptValue>& Value)
{
	if (!Object || !ClassName || !PropertyName)
	{
		NLOG_SCRIPT(Error, "Cannot set script property with null parameters");
		return false;
	}

	// 检查属性是否支持脚本写入
	const SScriptBindingInfo* BindingInfo = GetPropertyBindingInfo(ClassName, PropertyName);
	if (!BindingInfo || !BindingInfo->bScriptWritable)
	{
		NLOG_SCRIPT(Error, "Property '{}::{}' is not script writable", ClassName, PropertyName);
		return false;
	}

	// 通过反射系统设置属性值
	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();
	const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName);
	if (!ClassReflection)
	{
		NLOG_SCRIPT(Error, "Class '{}' not found in reflection registry", ClassName);
		return false;
	}

	const SPropertyReflection* PropertyReflection = ClassReflection->FindProperty(PropertyName);
	if (!PropertyReflection)
	{
		NLOG_SCRIPT(Error, "Property '{}' not found in class '{}'", PropertyName, ClassName);
		return false;
	}

	// TODO: 实现实际的属性设置
	NLOG_SCRIPT(Debug, "Set script property: {}::{}", ClassName, PropertyName);

	return true;
}

void CScriptBindingLoader::PrintBindingInfo() const
{
	std::lock_guard<std::mutex> Lock(BindingMutex);

	NLOG_SCRIPT(Info, "=== Script Binding Information ===");

	NLOG_SCRIPT(Info, "Classes: {}", ClassBindings.Size());
	for (const auto& ClassPair : ClassBindings)
	{
		const CString& ClassName = ClassPair.first;
		const SScriptBindingInfo& Info = ClassPair.second;

		NLOG_SCRIPT(Info,
		            "  {} -> {} (Creatable: {}, Visible: {})",
		            ClassName.GetData(),
		            Info.ScriptName.IsEmpty() ? ClassName.GetData() : Info.ScriptName.GetData(),
		            Info.bScriptCreatable,
		            Info.bScriptVisible);
	}

	NLOG_SCRIPT(Info, "Functions: {}", FunctionBindings.Size());
	for (const auto& FunctionPair : FunctionBindings)
	{
		const CString& Key = FunctionPair.first;
		const SScriptBindingInfo& Info = FunctionPair.second;

		NLOG_SCRIPT(Info, "  {} (Callable: {}, Static: {})", Key.GetData(), Info.bScriptCallable, Info.bScriptStatic);
	}

	NLOG_SCRIPT(Info, "Properties: {}", PropertyBindings.Size());
	for (const auto& PropertyPair : PropertyBindings)
	{
		const CString& Key = PropertyPair.first;
		const SScriptBindingInfo& Info = PropertyPair.second;

		NLOG_SCRIPT(
		    Info, "  {} (Readable: {}, Writable: {})", Key.GetData(), Info.bScriptReadable, Info.bScriptWritable);
	}

	NLOG_SCRIPT(Info, "Enums: {}", EnumBindings.Size());
}

CScriptBindingLoader::SBindingStats CScriptBindingLoader::GetBindingStats() const
{
	std::lock_guard<std::mutex> Lock(BindingMutex);

	SBindingStats Stats;
	Stats.ClassCount = ClassBindings.Size();
	Stats.FunctionCount = FunctionBindings.Size();
	Stats.PropertyCount = PropertyBindings.Size();
	Stats.EnumCount = EnumBindings.Size();

	return Stats;
}

CString CScriptBindingLoader::GenerateBindingKey(const char* ClassName, const char* MemberName) const
{
	return CString(ClassName) + TEXT("::") + CString(MemberName);
}

bool CScriptBindingLoader::LoadBindingFile(const CString& FilePath,
                                           EScriptLanguage Language,
                                           TSharedPtr<CScriptContext> Context)
{
	if (!NFileSystem::IsFile(NPath(FilePath)))
	{
		NLOG_SCRIPT(Error, "Binding file not found: {}", FilePath.GetData());
		return false;
	}

	switch (Language)
	{
	case EScriptLanguage::Lua: {
		// 执行Lua绑定文件
		auto ExecuteResult = Context->ExecuteFile(FilePath);
		if (ExecuteResult.Result != EScriptResult::Success)
		{
			NLOG_SCRIPT(Error,
			            "Failed to execute Lua binding file '{}': {}",
			            FilePath.GetData(),
			            ExecuteResult.ErrorMessage.GetData());
			return false;
		}

		NLOG_SCRIPT(Debug, "Loaded Lua binding file: {}", FilePath.GetData());
		return true;
	}

	case EScriptLanguage::TypeScript:
		// TypeScript类型定义文件不需要在运行时加载
		NLOG_SCRIPT(Debug, "TypeScript definition file: {}", FilePath.GetData());
		return true;

	default:
		NLOG_SCRIPT(Warning, "Unsupported binding file language for: {}", FilePath.GetData());
		return false;
	}
}

} // namespace NLib