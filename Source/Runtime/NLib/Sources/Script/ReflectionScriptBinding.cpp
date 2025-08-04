#include "ReflectionScriptBinding.h"

#include "IO/FileSystem.h"
#include "Logging/LogCategory.h"

namespace NLib
{
// === CScriptBindingRegistry 实现 ===

CScriptBindingRegistry& CScriptBindingRegistry::GetInstance()
{
	static CScriptBindingRegistry Instance;
	return Instance;
}

void CScriptBindingRegistry::RegisterGenerator(EScriptLanguage Language, TSharedPtr<IScriptBindingGenerator> Generator)
{
	if (!Generator)
	{
		NLOG_SCRIPT(Error, "Cannot register null script binding generator");
		return;
	}

	Generators.Add(Language, Generator);
	NLOG_SCRIPT(Info, "Registered script binding generator for language {}", static_cast<int>(Language));
}

void CScriptBindingRegistry::RegisterClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName)
	{
		NLOG_SCRIPT(Error, "Cannot register class binding with null name");
		return;
	}

	TString ClassNameStr(ClassName);
	ClassBindings.Add(ClassNameStr, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for class: {}", ClassName);
}

void CScriptBindingRegistry::RegisterFunctionBinding(const char* ClassName,
                                                     const char* FunctionName,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName || !FunctionName)
	{
		NLOG_SCRIPT(Error, "Cannot register function binding with null names");
		return;
	}

	TString Key = TString(ClassName) + TEXT("::") + TString(FunctionName);
	FunctionBindings.Add(Key, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for function: {}::{}", ClassName, FunctionName);
}

void CScriptBindingRegistry::RegisterPropertyBinding(const char* ClassName,
                                                     const char* PropertyName,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName || !PropertyName)
	{
		NLOG_SCRIPT(Error, "Cannot register property binding with null names");
		return;
	}

	TString Key = TString(ClassName) + TEXT("::") + TString(PropertyName);
	PropertyBindings.Add(Key, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for property: {}::{}", ClassName, PropertyName);
}

void CScriptBindingRegistry::RegisterEnumBinding(const char* EnumName, const SScriptBindingInfo& BindingInfo)
{
	if (!EnumName)
	{
		NLOG_SCRIPT(Error, "Cannot register enum binding with null name");
		return;
	}

	TString EnumNameStr(EnumName);
	EnumBindings.Add(EnumNameStr, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for enum: {}", EnumName);
}

const SScriptBindingInfo* CScriptBindingRegistry::GetClassBindingInfo(const char* ClassName) const
{
	if (!ClassName)
		return nullptr;

	TString ClassNameStr(ClassName);
	auto Found = ClassBindings.Find(ClassNameStr);
	return Found ? &Found->Value : nullptr;
}

const SScriptBindingInfo* CScriptBindingRegistry::GetFunctionBindingInfo(const char* ClassName,
                                                                         const char* FunctionName) const
{
	if (!ClassName || !FunctionName)
		return nullptr;

	TString Key = TString(ClassName) + TEXT("::") + TString(FunctionName);
	auto Found = FunctionBindings.Find(Key);
	return Found ? &Found->Value : nullptr;
}

const SScriptBindingInfo* CScriptBindingRegistry::GetPropertyBindingInfo(const char* ClassName,
                                                                         const char* PropertyName) const
{
	if (!ClassName || !PropertyName)
		return nullptr;

	TString Key = TString(ClassName) + TEXT("::") + TString(PropertyName);
	auto Found = PropertyBindings.Find(Key);
	return Found ? &Found->Value : nullptr;
}

TString CScriptBindingRegistry::GenerateBindingCode(EScriptLanguage Language) const
{
	auto FoundGenerator = Generators.Find(Language);
	if (!FoundGenerator)
	{
		NLOG_SCRIPT(Error, "No binding generator found for language {}", static_cast<int>(Language));
		return TString();
	}

	auto ScriptableClasses = GetScriptBindableClasses();
	return FoundGenerator->Value->GenerateBindingFile(ScriptableClasses);
}

void CScriptBindingRegistry::GenerateAllBindings(const TString& OutputDirectory) const
{
	NLOG_SCRIPT(Info, "Generating script bindings to directory: {}", OutputDirectory.GetData());

	// 确保输出目录存在
	if (!NFileSystem::DirectoryExists(OutputDirectory))
	{
		NFileSystem::CreateDirectories(OutputDirectory);
	}

	for (const auto& GeneratorPair : Generators)
	{
		EScriptLanguage Language = GeneratorPair.Key;
		auto Generator = GeneratorPair.Value;

		TString BindingCode = Generator->GenerateBindingFile(GetScriptBindableClasses());
		if (!BindingCode.IsEmpty())
		{
			TString FileName;
			switch (Language)
			{
			case EScriptLanguage::Lua:
				FileName = TEXT("NLibBindings.lua");
				break;
			case EScriptLanguage::TypeScript:
				FileName = TEXT("NLibBindings.d.ts");
				break;
			case EScriptLanguage::Python:
				FileName = TEXT("nlib_bindings.py");
				break;
			case EScriptLanguage::CSharp:
				FileName = TEXT("NLibBindings.cs");
				break;
			default:
				FileName = TString(TEXT("NLibBindings_")) + TString::FromInt(static_cast<int>(Language));
				break;
			}

			TString FilePath = NPath::Combine(OutputDirectory, FileName);
			auto WriteResult = NFileSystem::WriteFileAsString(FilePath, BindingCode);

			if (WriteResult.IsSuccess())
			{
				NLOG_SCRIPT(Info, "Generated binding file: {}", FilePath.GetData());
			}
			else
			{
				NLOG_SCRIPT(Error, "Failed to write binding file: {}", FilePath.GetData());
			}
		}
	}
}

TArray<const SClassReflection*, CMemoryManager> CScriptBindingRegistry::GetScriptBindableClasses() const
{
	TArray<const SClassReflection*, CMemoryManager> Result;

	auto& ReflectionRegistry = CReflectionRegistry::GetInstance();

	// 遍历所有已注册的类
	for (const auto& ClassBindingPair : ClassBindings)
	{
		const TString& ClassName = ClassBindingPair.Key;
		const SScriptBindingInfo& BindingInfo = ClassBindingPair.Value;

		if (BindingInfo.ShouldBind())
		{
			const SClassReflection* ClassReflection = ReflectionRegistry.FindClass(ClassName.GetData());
			if (ClassReflection)
			{
				Result.Add(ClassReflection);
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

// === CLuaBindingGenerator 实现 ===

TString CLuaBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                   const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return TString();
	}

	TString Code;
	TString ClassName = BindingInfo.ScriptName.IsEmpty() ? TString(ClassReflection->Name) : BindingInfo.ScriptName;

	Code += TString(TEXT("-- ")) + ClassName + TEXT(" class binding\n");
	Code += TString(TEXT("local ")) + ClassName + TEXT(" = {}\n");
	Code += ClassName + TEXT(".__index = ") + ClassName + TEXT("\n\n");

	// 构造函数
	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += TString(TEXT("function ")) + ClassName + TEXT(".new(...)\n");
		Code += TEXT("    local instance = setmetatable({}, ") + ClassName + TEXT(")\n");
		Code += TEXT("    -- Call C++ constructor\n");
		Code += TEXT("    instance._cppObject = NLib.CreateObject(\"") + TString(ClassReflection->Name) +
		        TEXT("\", ...)\n");
		Code += TEXT("    return instance\n");
		Code += TEXT("end\n\n");
	}

	// 析构函数
	Code += TString(TEXT("function ")) + ClassName + TEXT(":__gc()\n");
	Code += TEXT("    if self._cppObject then\n");
	Code += TEXT("        NLib.DestroyObject(self._cppObject)\n");
	Code += TEXT("    end\n");
	Code += TEXT("end\n\n");

	// 属性getter/setter
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() && PropBindingInfo->SupportsLanguage(EScriptLanguage::Lua))
		{
			Code += GeneratePropertyBinding(&PropInfo, *PropBindingInfo, ClassName);
		}
	}

	// 方法
	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		const SScriptBindingInfo* FuncBindingInfo = CScriptBindingRegistry::GetInstance().GetFunctionBindingInfo(
		    ClassReflection->Name, FuncInfo.Name);

		if (FuncBindingInfo && FuncBindingInfo->ShouldBind() && FuncBindingInfo->SupportsLanguage(EScriptLanguage::Lua))
		{
			Code += GenerateFunctionBinding(&FuncInfo, *FuncBindingInfo, ClassName);
		}
	}

	// 注册到全局
	Code += TString(TEXT("-- Register ")) + ClassName + TEXT(" globally\n");
	Code += TString(TEXT("_G.")) + ClassName + TEXT(" = ") + ClassName + TEXT("\n\n");

	return Code;
}

TString CLuaBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                      const SScriptBindingInfo& BindingInfo,
                                                      const TString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return TString();
	}

	TString FunctionName = BindingInfo.ScriptName.IsEmpty() ? TString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;

	if (BindingInfo.bScriptStatic)
	{
		// 静态函数
		Code += TString(TEXT("function ")) + ClassName + TEXT(".") + FunctionName + TEXT("(...)\n");
		Code += TEXT("    return NLib.CallStaticFunction(\"") + ClassName + TEXT("\", \"") +
		        TString(FunctionReflection->Name) + TEXT("\", ...)\n");
	}
	else
	{
		// 实例方法
		Code += TString(TEXT("function ")) + ClassName + TEXT(":") + FunctionName + TEXT("(...)\n");
		Code += TEXT("    return NLib.CallMethod(self._cppObject, \"") + TString(FunctionReflection->Name) +
		        TEXT("\", ...)\n");
	}

	Code += TEXT("end\n\n");

	return Code;
}

TString CLuaBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                      const SScriptBindingInfo& BindingInfo,
                                                      const TString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return TString();
	}

	TString PropertyName = BindingInfo.ScriptName.IsEmpty() ? TString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;

	if (BindingInfo.bScriptReadable)
	{
		// Getter
		Code += TString(TEXT("function ")) + ClassName + TEXT(":get") + PropertyName + TEXT("()\n");
		Code += TEXT("    return NLib.GetProperty(self._cppObject, \"") + TString(PropertyReflection->Name) +
		        TEXT("\")\n");
		Code += TEXT("end\n\n");
	}

	if (BindingInfo.bScriptWritable)
	{
		// Setter
		Code += TString(TEXT("function ")) + ClassName + TEXT(":set") + PropertyName + TEXT("(value)\n");
		Code += TEXT("    NLib.SetProperty(self._cppObject, \"") + TString(PropertyReflection->Name) +
		        TEXT("\", value)\n");
		Code += TEXT("end\n\n");
	}

	return Code;
}

TString CLuaBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                  const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return TString();
	}

	TString EnumName = BindingInfo.ScriptName.IsEmpty() ? TString(EnumReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	Code += TString(TEXT("-- ")) + EnumName + TEXT(" enum\n");
	Code += TString(TEXT("local ")) + EnumName + TEXT(" = {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += TString(TEXT("    ")) + TString(ValueInfo.Name) + TEXT(" = ") +
		        TString::FromInt(static_cast<int32_t>(ValueInfo.Value)) + TEXT(",\n");
	}

	Code += TEXT("}\n\n");
	Code += TString(TEXT("_G.")) + EnumName + TEXT(" = ") + EnumName + TEXT("\n\n");

	return Code;
}

TString CLuaBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	TString Code;

	Code += TEXT("-- NLib Lua Bindings\n");
	Code += TEXT("-- Auto-generated by NutHeaderTools\n");
	Code += TEXT("-- Do not modify this file directly\n\n");

	// 生成所有类的绑定
	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo)
		{
			Code += GenerateClassBinding(ClassReflection, *BindingInfo);
		}
	}

	return Code;
}

// === CTypeScriptBindingGenerator 实现 ===

TString CTypeScriptBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                          const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return TString();
	}

	TString ClassName = BindingInfo.ScriptName.IsEmpty() ? TString(ClassReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	// 生成类型定义
	Code += TString(TEXT("declare class ")) + ClassName + TEXT(" {\n");

	// 构造函数
	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += TEXT("    constructor(...args: any[]);\n");
	}

	// 属性
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::TypeScript))
		{
			Code += GeneratePropertyBinding(&PropInfo, *PropBindingInfo, ClassName);
		}
	}

	// 方法
	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		const SScriptBindingInfo* FuncBindingInfo = CScriptBindingRegistry::GetInstance().GetFunctionBindingInfo(
		    ClassReflection->Name, FuncInfo.Name);

		if (FuncBindingInfo && FuncBindingInfo->ShouldBind() &&
		    FuncBindingInfo->SupportsLanguage(EScriptLanguage::TypeScript))
		{
			Code += GenerateFunctionBinding(&FuncInfo, *FuncBindingInfo, ClassName);
		}
	}

	Code += TEXT("}\n\n");

	return Code;
}

TString CTypeScriptBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                             const SScriptBindingInfo& BindingInfo,
                                                             const TString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return TString();
	}

	TString FunctionName = BindingInfo.ScriptName.IsEmpty() ? TString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code = TEXT("    ");

	if (BindingInfo.bScriptStatic)
	{
		Code += TEXT("static ");
	}

	Code += FunctionName + TEXT("(");

	// 参数
	for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
	{
		if (i > 0)
			Code += TEXT(", ");
		const auto& Param = FunctionReflection->Parameters[i];
		Code += TString(Param.Name) + TEXT(": ") + ConvertTypeToTypeScript(*Param.TypeInfo);
	}

	Code += TEXT("): ") + ConvertTypeToTypeScript(*FunctionReflection->ReturnTypeInfo) + TEXT(";\n");

	return Code;
}

TString CTypeScriptBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                             const SScriptBindingInfo& BindingInfo,
                                                             const TString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return TString();
	}

	TString PropertyName = BindingInfo.ScriptName.IsEmpty() ? TString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code = TEXT("    ");

	if (BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable)
	{
		Code += TEXT("readonly ");
	}

	Code += PropertyName + TEXT(": ") + ConvertTypeToTypeScript(*PropertyReflection->TypeInfo) + TEXT(";\n");

	return Code;
}

TString CTypeScriptBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                         const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return TString();
	}

	TString EnumName = BindingInfo.ScriptName.IsEmpty() ? TString(EnumReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	Code += TString(TEXT("declare enum ")) + EnumName + TEXT(" {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += TString(TEXT("    ")) + TString(ValueInfo.Name) + TEXT(" = ") +
		        TString::FromInt(static_cast<int32_t>(ValueInfo.Value)) + TEXT(",\n");
	}

	Code += TEXT("}\n\n");

	return Code;
}

TString CTypeScriptBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	TString Code;

	Code += TEXT("// NLib TypeScript Bindings\n");
	Code += TEXT("// Auto-generated by NutHeaderTools\n");
	Code += TEXT("// Do not modify this file directly\n\n");

	// 生成所有类的类型定义
	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo)
		{
			Code += GenerateClassBinding(ClassReflection, *BindingInfo);
		}
	}

	return Code;
}

TString CTypeScriptBindingGenerator::ConvertTypeToTypeScript(const std::type_info& TypeInfo) const
{
	// 简化的类型转换，实际实现需要更完整
	if (TypeInfo == typeid(bool))
		return TEXT("boolean");
	else if (TypeInfo == typeid(int32_t) || TypeInfo == typeid(int64_t) || TypeInfo == typeid(float) ||
	         TypeInfo == typeid(double))
		return TEXT("number");
	else if (TypeInfo == typeid(TString))
		return TEXT("string");
	else if (TypeInfo == typeid(void))
		return TEXT("void");
	else
		return TEXT("any");
}

TString CTypeScriptBindingGenerator::GenerateTypeDefinition(const SClassReflection* ClassReflection,
                                                            const SScriptBindingInfo& BindingInfo) const
{
	return GenerateClassBinding(ClassReflection, BindingInfo);
}

TString CTypeScriptBindingGenerator::GenerateInterfaceDefinition(const SClassReflection* ClassReflection) const
{
	TString Code;
	Code += TString(TEXT("interface I")) + TString(ClassReflection->Name) + TEXT(" {\n");

	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		Code += TString(TEXT("    ")) + TString(PropInfo.Name) + TEXT(": ") +
		        ConvertTypeToTypeScript(*PropInfo.TypeInfo) + TEXT(";\n");
	}

	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		Code += TString(TEXT("    ")) + TString(FuncInfo.Name) + TEXT("(");

		for (size_t j = 0; j < FuncInfo.Parameters.size(); ++j)
		{
			if (j > 0)
				Code += TEXT(", ");
			const auto& Param = FuncInfo.Parameters[j];
			Code += TString(Param.Name) + TEXT(": ") + ConvertTypeToTypeScript(*Param.TypeInfo);
		}

		Code += TEXT("): ") + ConvertTypeToTypeScript(*FuncInfo.ReturnTypeInfo) + TEXT(";\n");
	}

	Code += TEXT("}\n\n");
	return Code;
}

// === CCSharpBindingGenerator 实现 ===

TString CCSharpBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                      const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return TString();
	}

	TString ClassName = BindingInfo.ScriptName.IsEmpty() ? TString(ClassReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	Code += TEXT("using System;\n");
	Code += TEXT("using System.Runtime.InteropServices;\n");
	Code += TEXT("using NLib.Interop;\n\n");

	Code += TEXT("namespace NLib.Generated\n{\n");
	Code += TString(TEXT("    public class ")) + ClassName;

	if (ClassReflection->BaseClassName && strlen(ClassReflection->BaseClassName) > 0)
	{
		Code += TString(TEXT(" : ")) + TString(ClassReflection->BaseClassName);
	}
	else
	{
		Code += TEXT(" : NLibObject");
	}

	Code += TEXT("\n    {\n");

	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += TString(TEXT("        public ")) + ClassName + TEXT("()\n");
		Code += TEXT("        {\n");
		Code += TString(TEXT("            _nativePtr = NLibInterop.CreateObject(\"")) + TString(ClassReflection->Name) +
		        TEXT("\");\n");
		Code += TEXT("        }\n\n");
	}

	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::CSharp))
		{
			Code += GeneratePropertyBinding(&PropInfo, *PropBindingInfo, ClassName);
		}
	}

	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		const SScriptBindingInfo* FuncBindingInfo = CScriptBindingRegistry::GetInstance().GetFunctionBindingInfo(
		    ClassReflection->Name, FuncInfo.Name);

		if (FuncBindingInfo && FuncBindingInfo->ShouldBind() &&
		    FuncBindingInfo->SupportsLanguage(EScriptLanguage::CSharp))
		{
			Code += GenerateFunctionBinding(&FuncInfo, *FuncBindingInfo, ClassName);
		}
	}

	Code += TEXT("        private IntPtr _nativePtr = IntPtr.Zero;\n");
	Code += TEXT("        public IntPtr NativePtr => _nativePtr;\n");
	Code += TEXT("    }\n");
	Code += TEXT("}\n\n");

	return Code;
}

TString CCSharpBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return TString();
	}

	TString FunctionName = BindingInfo.ScriptName.IsEmpty() ? TString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;

	Code += TEXT("        public ");
	if (BindingInfo.bScriptStatic)
		Code += TEXT("static ");

	Code += ConvertTypeToCSharp(*FunctionReflection->ReturnTypeInfo) + TEXT(" ");
	Code += FunctionName + TEXT("(");

	for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
	{
		if (i > 0)
			Code += TEXT(", ");
		const auto& Param = FunctionReflection->Parameters[i];
		Code += ConvertTypeToCSharp(*Param.TypeInfo) + TEXT(" ") + TString(Param.Name);
	}

	Code += TEXT(")\n        {\n");

	if (BindingInfo.bScriptStatic)
	{
		Code += TString(TEXT("            return NLibInterop.CallStaticMethod<")) +
		        ConvertTypeToCSharp(*FunctionReflection->ReturnTypeInfo) + TEXT(">(\"") + ClassName + TEXT("\", \"") +
		        TString(FunctionReflection->Name) + TEXT("\"");
	}
	else
	{
		Code += TString(TEXT("            return NLibInterop.CallMethod<")) +
		        ConvertTypeToCSharp(*FunctionReflection->ReturnTypeInfo) + TEXT(">(_nativePtr, \"") +
		        TString(FunctionReflection->Name) + TEXT("\"");
	}

	for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
	{
		const auto& Param = FunctionReflection->Parameters[i];
		Code += TEXT(", ") + TString(Param.Name);
	}

	Code += TEXT(");\n        }\n\n");
	return Code;
}

TString CCSharpBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return TString();
	}

	TString PropertyName = BindingInfo.ScriptName.IsEmpty() ? TString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;

	Code += TString(TEXT("        public ")) + ConvertTypeToCSharp(*PropertyReflection->TypeInfo) + TEXT(" ") +
	        PropertyName + TEXT("\n");
	Code += TEXT("        {\n");

	if (BindingInfo.bScriptReadable)
	{
		Code += TEXT("            get => NLibInterop.GetProperty<") +
		        ConvertTypeToCSharp(*PropertyReflection->TypeInfo) + TEXT(">(_nativePtr, \"") +
		        TString(PropertyReflection->Name) + TEXT("\");\n");
	}

	if (BindingInfo.bScriptWritable)
	{
		Code += TEXT("            set => NLibInterop.SetProperty(_nativePtr, \"") + TString(PropertyReflection->Name) +
		        TEXT("\", value);\n");
	}

	Code += TEXT("        }\n\n");
	return Code;
}

TString CCSharpBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return TString();
	}

	TString EnumName = BindingInfo.ScriptName.IsEmpty() ? TString(EnumReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	Code += TString(TEXT("    public enum ")) + EnumName + TEXT("\n    {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += TString(TEXT("        ")) + TString(ValueInfo.Name) + TEXT(" = ") +
		        TString::FromInt(static_cast<int32_t>(ValueInfo.Value));

		if (i < EnumReflection->ValueCount - 1)
			Code += TEXT(",");
		Code += TEXT("\n");
	}

	Code += TEXT("    }\n\n");
	return Code;
}

TString CCSharpBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	TString Code;

	Code += TEXT("// NLib C# Bindings\n");
	Code += TEXT("// Auto-generated by NutHeaderTools\n\n");

	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo)
		{
			Code += GenerateClassBinding(ClassReflection, *BindingInfo);
		}
	}

	return Code;
}

TString CCSharpBindingGenerator::ConvertTypeToCSharp(const std::type_info& TypeInfo) const
{
	if (TypeInfo == typeid(bool))
		return TEXT("bool");
	else if (TypeInfo == typeid(int32_t))
		return TEXT("int");
	else if (TypeInfo == typeid(int64_t))
		return TEXT("long");
	else if (TypeInfo == typeid(float))
		return TEXT("float");
	else if (TypeInfo == typeid(double))
		return TEXT("double");
	else if (TypeInfo == typeid(TString))
		return TEXT("string");
	else if (TypeInfo == typeid(void))
		return TEXT("void");
	else
		return TEXT("object");
}

TString CCSharpBindingGenerator::GenerateCSharpClass(const SClassReflection* ClassReflection,
                                                     const SScriptBindingInfo& BindingInfo) const
{
	return GenerateClassBinding(ClassReflection, BindingInfo);
}

TString CCSharpBindingGenerator::GeneratePInvokeDeclarations(const SClassReflection* ClassReflection) const
{
	TString Code;
	Code += TEXT("    [DllImport(\"NLib\")]\n");
	Code += TString(TEXT("    public static extern IntPtr Create")) + TString(ClassReflection->Name) + TEXT("();\n");
	return Code;
}

// === 空的Python绑定生成器实现（占位符） ===

TString CPythonBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                      const SScriptBindingInfo& BindingInfo)
{
	NLOG_SCRIPT(Warning, "Python binding generation not implemented yet");
	return TString();
}

TString CPythonBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	return TString();
}

TString CPythonBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	return TString();
}

TString CPythonBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                     const SScriptBindingInfo& BindingInfo)
{
	return TString();
}

TString CPythonBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	return TString();
}

TString CPythonBindingGenerator::ConvertTypeToPython(const std::type_info& TypeInfo) const
{
	return TString();
}

TString CPythonBindingGenerator::GeneratePythonClass(const SClassReflection* ClassReflection,
                                                     const SScriptBindingInfo& BindingInfo) const
{
	return TString();
}

TString CPythonBindingGenerator::GeneratePythonStub(const SClassReflection* ClassReflection) const
{
	return TString();
}

// === CPythonBindingGenerator 实现 ===

TString CPythonBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                      const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return TString();
	}

	TString ClassName = BindingInfo.ScriptName.IsEmpty() ? TString(ClassReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	// Python 类定义开始
	Code += TString::Format(TEXT("class {}:\n"), ClassName.GetData());
	Code += TEXT("    \"\"\"\n");
	Code += TString::Format(TEXT("    NLib {} class binding\n"), ClassName.GetData());
	Code += TEXT("    Auto-generated by NutHeaderTools\n");
	Code += TEXT("    \"\"\"\n\n");

	// 类属性（类型注解）
	bool hasProperties = false;
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			if (!hasProperties)
			{
				Code += TEXT("    # Class properties\n");
				hasProperties = true;
			}

			TString PropName = PropBindingInfo->ScriptName.IsEmpty() ? TString(PropInfo.Name)
			                                                         : PropBindingInfo->ScriptName;
			TString PropType = ConvertTypeToPython(*PropInfo.TypeInfo);
			Code += TString::Format(TEXT("    {}: {}\n"), PropName.GetData(), PropType.GetData());
		}
	}

	if (hasProperties)
	{
		Code += TEXT("\n");
	}

	// 构造函数
	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += TEXT("    def __init__(self, *args, **kwargs):\n");
		Code += TEXT("        \"\"\"Initialize the object with native C++ constructor\"\"\"\n");
		Code += TEXT("        self._native_ptr = None  # Will be set by C++ binding\n");
		Code += TEXT("        pass\n\n");
	}

	// 方法
	bool hasMethods = false;
	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		const SScriptBindingInfo* FuncBindingInfo = CScriptBindingRegistry::GetInstance().GetFunctionBindingInfo(
		    ClassReflection->Name, FuncInfo.Name);

		if (FuncBindingInfo && FuncBindingInfo->ShouldBind() &&
		    FuncBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			if (!hasMethods)
			{
				Code += TEXT("    # Class methods\n");
				hasMethods = true;
			}

			Code += GenerateFunctionBinding(&FuncInfo, *FuncBindingInfo, ClassName);
		}
	}

	// 属性访问器（Property decorators）
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			Code += GeneratePropertyBinding(&PropInfo, *PropBindingInfo, ClassName);
		}
	}

	// 特殊方法
	Code += TEXT("    def __str__(self):\n");
	Code += TString::Format(TEXT("        return f\"<{} object at {{hex(id(self))}}>\"\n"), ClassName.GetData());
	Code += TEXT("\n");

	Code += TEXT("    def __repr__(self):\n");
	Code += TEXT("        return self.__str__()\n\n");

	return Code;
}

TString CPythonBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return TString();
	}

	TString FunctionName = BindingInfo.ScriptName.IsEmpty() ? TString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;

	// 静态方法装饰器
	if (BindingInfo.bScriptStatic)
	{
		Code += TEXT("    @staticmethod\n");
	}

	// 函数签名
	Code += TEXT("    def ") + FunctionName + TEXT("(");

	if (!BindingInfo.bScriptStatic)
	{
		Code += TEXT("self");
		if (!FunctionReflection->Parameters.empty())
		{
			Code += TEXT(", ");
		}
	}

	// 参数
	for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
	{
		if (i > 0)
			Code += TEXT(", ");
		const auto& Param = FunctionReflection->Parameters[i];
		Code += TString(Param.Name);

		// 类型注解
		TString ParamType = ConvertTypeToPython(*Param.TypeInfo);
		if (!ParamType.IsEmpty())
		{
			Code += TEXT(": ") + ParamType;
		}
	}

	Code += TEXT(")");

	// 返回类型注解
	TString ReturnType = ConvertTypeToPython(*FunctionReflection->ReturnTypeInfo);
	if (!ReturnType.IsEmpty())
	{
		Code += TEXT(" -> ") + ReturnType;
	}

	Code += TEXT(":\n");

	// 文档字符串
	Code += TEXT("        \"\"\"\n");
	Code += TString::Format(TEXT("        {} method binding\n"), FunctionName.GetData());

	if (!FunctionReflection->Parameters.empty())
	{
		Code += TEXT("        \n        Args:\n");
		for (const auto& Param : FunctionReflection->Parameters)
		{
			TString ParamType = ConvertTypeToPython(*Param.TypeInfo);
			Code += TString::Format(TEXT("            {} ({}): Parameter\n"), Param.Name, ParamType.GetData());
		}
	}

	if (*FunctionReflection->ReturnTypeInfo != typeid(void))
	{
		TString RetType = ConvertTypeToPython(*FunctionReflection->ReturnTypeInfo);
		Code += TEXT("        \n        Returns:\n");
		Code += TString::Format(TEXT("            {}: Return value\n"), RetType.GetData());
	}

	Code += TEXT("        \"\"\"\n");

	// 方法体（调用原生C++方法）
	Code += TEXT("        # Call native C++ method\n");
	Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");

	return Code;
}

TString CPythonBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const TString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return TString();
	}

	TString PropertyName = BindingInfo.ScriptName.IsEmpty() ? TString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	TString Code;
	TString PropertyType = ConvertTypeToPython(*PropertyReflection->TypeInfo);

	// Getter方法
	if (BindingInfo.bScriptReadable)
	{
		Code += TEXT("    @property\n");
		Code += TString::Format(TEXT("    def {}(self)"), PropertyName.GetData());

		if (!PropertyType.IsEmpty())
		{
			Code += TEXT(" -> ") + PropertyType;
		}

		Code += TEXT(":\n");
		Code += TString::Format(TEXT("        \"\"\"Get {} property\"\"\"\n"), PropertyName.GetData());
		Code += TEXT("        # Get native C++ property\n");
		Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");
	}

	// Setter方法
	if (BindingInfo.bScriptWritable)
	{
		Code += TString::Format(TEXT("    @{}.setter\n"), PropertyName.GetData());
		Code += TString::Format(TEXT("    def {}(self, value"), PropertyName.GetData());

		if (!PropertyType.IsEmpty())
		{
			Code += TEXT(": ") + PropertyType;
		}

		Code += TEXT("):\n");
		Code += TString::Format(TEXT("        \"\"\"Set {} property\"\"\"\n"), PropertyName.GetData());
		Code += TEXT("        # Set native C++ property\n");
		Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");
	}

	return Code;
}

TString CPythonBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return TString();
	}

	TString EnumName = BindingInfo.ScriptName.IsEmpty() ? TString(EnumReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	// 使用Python Enum类
	Code += TEXT("from enum import Enum, IntEnum\n\n");
	Code += TString::Format(TEXT("class {}(IntEnum):\n"), EnumName.GetData());
	Code += TEXT("    \"\"\"\n");
	Code += TString::Format(TEXT("    {} enumeration\n"), EnumName.GetData());
	Code += TEXT("    Auto-generated by NutHeaderTools\n");
	Code += TEXT("    \"\"\"\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += TString::Format(TEXT("    {} = {}\n"), ValueInfo.Name, static_cast<int32_t>(ValueInfo.Value));
	}

	Code += TEXT("\n");

	return Code;
}

TString CPythonBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	TString Code;

	Code += TEXT("#!/usr/bin/env python3\n");
	Code += TEXT("\"\"\"\n");
	Code += TEXT("NLib Python Bindings\n");
	Code += TEXT("Auto-generated by NutHeaderTools\n");
	Code += TEXT("Do not modify this file directly\n");
	Code += TEXT("\"\"\"\n\n");

	// 导入必要的模块
	Code += TEXT("from typing import Any, Optional, List, Dict, Union\n");
	Code += TEXT("from enum import Enum, IntEnum\n");
	Code += TEXT("import sys\n\n");

	// 生成所有类的绑定
	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo)
		{
			Code += GenerateClassBinding(ClassReflection, *BindingInfo);
		}
	}

	// 添加模块级别的功能
	Code += TEXT("# Module-level functions\n");
	Code += TEXT("def initialize_nlib():\n");
	Code += TEXT("    \"\"\"Initialize NLib Python bindings\"\"\"\n");
	Code += TEXT("    pass\n\n");

	Code += TEXT("def cleanup_nlib():\n");
	Code += TEXT("    \"\"\"Clean up NLib Python bindings\"\"\"\n");
	Code += TEXT("    pass\n\n");

	// 模块信息
	Code += TEXT("__version__ = \"1.0.0\"\n");
	Code += TEXT("__author__ = \"NutHeaderTools\"\n");
	Code += TEXT("__all__ = [\n");

	// 导出所有类名
	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo && BindingInfo->ShouldBind() && BindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			TString ClassName = BindingInfo->ScriptName.IsEmpty() ? TString(ClassReflection->Name)
			                                                      : BindingInfo->ScriptName;
			Code += TString::Format(TEXT("    \"{}\",\n"), ClassName.GetData());
		}
	}

	Code += TEXT("]\n");

	return Code;
}

TString CPythonBindingGenerator::ConvertTypeToPython(const std::type_info& TypeInfo) const
{
	// Python类型转换映射
	if (TypeInfo == typeid(bool))
		return TEXT("bool");
	else if (TypeInfo == typeid(int32_t) || TypeInfo == typeid(int64_t))
		return TEXT("int");
	else if (TypeInfo == typeid(float) || TypeInfo == typeid(double))
		return TEXT("float");
	else if (TypeInfo == typeid(TString))
		return TEXT("str");
	else if (TypeInfo == typeid(void))
		return TEXT("None");
	else
		return TEXT("Any");
}

TString CPythonBindingGenerator::GenerateTypeStub(const SClassReflection* ClassReflection,
                                                  const SScriptBindingInfo& BindingInfo) const
{
	// 生成.pyi类型存根文件
	TString ClassName = BindingInfo.ScriptName.IsEmpty() ? TString(ClassReflection->Name) : BindingInfo.ScriptName;
	TString Code;

	Code += TString::Format(TEXT("class {}:\n"), ClassName.GetData());

	// 属性类型注解
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			TString PropName = PropBindingInfo->ScriptName.IsEmpty() ? TString(PropInfo.Name)
			                                                         : PropBindingInfo->ScriptName;
			TString PropType = ConvertTypeToPython(*PropInfo.TypeInfo);
			Code += TString::Format(TEXT("    {}: {}\n"), PropName.GetData(), PropType.GetData());
		}
	}

	// 方法签名
	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		const SScriptBindingInfo* FuncBindingInfo = CScriptBindingRegistry::GetInstance().GetFunctionBindingInfo(
		    ClassReflection->Name, FuncInfo.Name);

		if (FuncBindingInfo && FuncBindingInfo->ShouldBind() &&
		    FuncBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			TString FunctionName = FuncBindingInfo->ScriptName.IsEmpty() ? TString(FuncInfo.Name)
			                                                             : FuncBindingInfo->ScriptName;
			Code += TEXT("    def ") + FunctionName + TEXT("(");

			if (!FuncBindingInfo->bScriptStatic)
			{
				Code += TEXT("self");
				if (!FuncInfo.Parameters.empty())
					Code += TEXT(", ");
			}

			for (size_t j = 0; j < FuncInfo.Parameters.size(); ++j)
			{
				if (j > 0)
					Code += TEXT(", ");
				const auto& Param = FuncInfo.Parameters[j];
				Code += TString(Param.Name) + TEXT(": ") + ConvertTypeToPython(*Param.TypeInfo);
			}

			Code += TEXT(") -> ") + ConvertTypeToPython(*FuncInfo.ReturnTypeInfo) + TEXT(": ...\n");
		}
	}

	return Code;
}

TString CPythonBindingGenerator::GeneratePyiFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) const
{
	TString Code;

	Code += TEXT("# NLib Python Type Stubs\n");
	Code += TEXT("# Auto-generated by NutHeaderTools\n\n");

	Code += TEXT("from typing import Any, Optional, List, Dict, Union\n");
	Code += TEXT("from enum import IntEnum\n\n");

	for (const auto* ClassReflection : Classes)
	{
		const SScriptBindingInfo* BindingInfo = CScriptBindingRegistry::GetInstance().GetClassBindingInfo(
		    ClassReflection->Name);
		if (BindingInfo && BindingInfo->ShouldBind() && BindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			Code += GenerateTypeStub(ClassReflection, *BindingInfo);
			Code += TEXT("\n");
		}
	}

	return Code;
}

} // namespace NLib