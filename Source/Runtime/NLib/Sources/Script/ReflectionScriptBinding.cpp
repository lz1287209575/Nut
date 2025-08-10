#include "ReflectionScriptBinding.h"

#include "IO/FileSystem.h"
#include "IO/IO.h"
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

	Generators.Insert(Language, Generator);
	NLOG_SCRIPT(Info, "Registered script binding generator for language {}", static_cast<int>(Language));
}

void CScriptBindingRegistry::RegisterClassBinding(const char* ClassName, const SScriptBindingInfo& BindingInfo)
{
	if (!ClassName)
	{
		NLOG_SCRIPT(Error, "Cannot register class binding with null name");
		return;
	}

	CString ClassNameStr(ClassName);
	ClassBindings.Insert(ClassNameStr, BindingInfo);

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

	CString Key = CString(ClassName) + TEXT("::") + CString(FunctionName);
	FunctionBindings.Insert(Key, BindingInfo);

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

	CString Key = CString(ClassName) + TEXT("::") + CString(PropertyName);
	PropertyBindings.Insert(Key, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for property: {}::{}", ClassName, PropertyName);
}

void CScriptBindingRegistry::RegisterEnumBinding(const char* EnumName, const SScriptBindingInfo& BindingInfo)
{
	if (!EnumName)
	{
		NLOG_SCRIPT(Error, "Cannot register enum binding with null name");
		return;
	}

	CString EnumNameStr(EnumName);
	EnumBindings.Insert(EnumNameStr, BindingInfo);

	NLOG_SCRIPT(Debug, "Registered script binding for enum: {}", EnumName);
}

const SScriptBindingInfo* CScriptBindingRegistry::GetClassBindingInfo(const char* ClassName) const
{
	if (!ClassName)
		return nullptr;

	CString ClassNameStr(ClassName);
	auto Found = ClassBindings.Find(ClassNameStr);
	return Found ? &(*Found) : nullptr;
}

const SScriptBindingInfo* CScriptBindingRegistry::GetFunctionBindingInfo(const char* ClassName,
                                                                         const char* FunctionName) const
{
	if (!ClassName || !FunctionName)
		return nullptr;

	CString Key = CString(ClassName) + TEXT("::") + CString(FunctionName);
	auto Found = FunctionBindings.Find(Key);
	return Found ? &(*Found) : nullptr;
}

const SScriptBindingInfo* CScriptBindingRegistry::GetPropertyBindingInfo(const char* ClassName,
                                                                         const char* PropertyName) const
{
	if (!ClassName || !PropertyName)
		return nullptr;

	CString Key = CString(ClassName) + TEXT("::") + CString(PropertyName);
	auto Found = PropertyBindings.Find(Key);
	return Found ? &(*Found) : nullptr;
}

CString CScriptBindingRegistry::GenerateBindingCode(EScriptLanguage Language) const
{
	auto FoundGenerator = Generators.Find(Language);
	if (!FoundGenerator)
	{
		NLOG_SCRIPT(Error, "No binding generator found for language {}", static_cast<int>(Language));
		return CString();
	}

	auto ScriptableClasses = GetScriptBindableClasses();
	return (*FoundGenerator)->GenerateBindingFile(ScriptableClasses);
}

void CScriptBindingRegistry::GenerateAllBindings(const CString& OutputDirectory) const
{
	NLOG_SCRIPT(Info, "Generating script bindings to directory: {}", OutputDirectory.GetData());

	// 确保输出目录存在
	if (!DirectoryExists(NPath(OutputDirectory)))
	{
		NFileSystem::CreateDirectory(NPath(OutputDirectory), true);
	}

	for (const auto& GeneratorPair : Generators)
	{
		EScriptLanguage Language = GeneratorPair.first;
		auto Generator = GeneratorPair.second;

		CString BindingCode = Generator->GenerateBindingFile(GetScriptBindableClasses());
		if (!BindingCode.IsEmpty())
		{
			CString FileName;
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
				FileName = CString(TEXT("NLibBindings_")) + CString::FromInt(static_cast<int>(Language));
				break;
			}

			CString FilePath = NPath::Combine(OutputDirectory, FileName).ToString();
			auto WriteResult = NFileSystem::WriteAllText(NPath(FilePath), BindingCode);

			if (WriteResult.bSuccess)
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

// === CLuaBindingGenerator 实现 ===

CString CLuaBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                   const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return CString();
	}

	CString Code;
	CString ClassName = BindingInfo.ScriptName.IsEmpty() ? CString(ClassReflection->Name) : BindingInfo.ScriptName;

	Code += CString(TEXT("-- ")) + ClassName + TEXT(" class binding\n");
	Code += CString(TEXT("local ")) + ClassName + TEXT(" = {}\n");
	Code += ClassName + TEXT(".__index = ") + ClassName + TEXT("\n\n");

	// 构造函数
	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += CString(TEXT("function ")) + ClassName + TEXT(".new(...)\n");
		Code += TEXT("    local instance = setmetatable({}, ") + ClassName + TEXT(")\n");
		Code += TEXT("    -- Call C++ constructor\n");
		Code += TEXT("    instance._cppObject = NLib.CreateObject(\"") + CString(ClassReflection->Name) +
		        TEXT("\", ...)\n");
		Code += TEXT("    return instance\n");
		Code += TEXT("end\n\n");
	}

	// 析构函数
	Code += CString(TEXT("function ")) + ClassName + TEXT(":__gc()\n");
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
	Code += CString(TEXT("-- Register ")) + ClassName + TEXT(" globally\n");
	Code += CString(TEXT("_G.")) + ClassName + TEXT(" = ") + ClassName + TEXT("\n\n");

	return Code;
}

CString CLuaBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                      const SScriptBindingInfo& BindingInfo,
                                                      const CString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return CString();
	}

	CString FunctionName = BindingInfo.ScriptName.IsEmpty() ? CString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;

	if (BindingInfo.bScriptStatic)
	{
		// 静态函数
		Code += CString(TEXT("function ")) + ClassName + TEXT(".") + FunctionName + TEXT("(...)\n");
		Code += TEXT("    return NLib.CallStaticFunction(\"") + ClassName + TEXT("\", \"") +
		        CString(FunctionReflection->Name) + TEXT("\", ...)\n");
	}
	else
	{
		// 实例方法
		Code += CString(TEXT("function ")) + ClassName + TEXT(":") + FunctionName + TEXT("(...)\n");
		Code += TEXT("    return NLib.CallMethod(self._cppObject, \"") + CString(FunctionReflection->Name) +
		        TEXT("\", ...)\n");
	}

	Code += TEXT("end\n\n");

	return Code;
}

CString CLuaBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                      const SScriptBindingInfo& BindingInfo,
                                                      const CString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return CString();
	}

	CString PropertyName = BindingInfo.ScriptName.IsEmpty() ? CString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;

	if (BindingInfo.bScriptReadable)
	{
		// Getter
		Code += CString(TEXT("function ")) + ClassName + TEXT(":get") + PropertyName + TEXT("()\n");
		Code += TEXT("    return NLib.GetProperty(self._cppObject, \"") + CString(PropertyReflection->Name) +
		        TEXT("\")\n");
		Code += TEXT("end\n\n");
	}

	if (BindingInfo.bScriptWritable)
	{
		// Setter
		Code += CString(TEXT("function ")) + ClassName + TEXT(":set") + PropertyName + TEXT("(value)\n");
		Code += TEXT("    NLib.SetProperty(self._cppObject, \"") + CString(PropertyReflection->Name) +
		        TEXT("\", value)\n");
		Code += TEXT("end\n\n");
	}

	return Code;
}

CString CLuaBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                  const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Lua))
	{
		return CString();
	}

	CString EnumName = BindingInfo.ScriptName.IsEmpty() ? CString(EnumReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	Code += CString(TEXT("-- ")) + EnumName + TEXT(" enum\n");
	Code += CString(TEXT("local ")) + EnumName + TEXT(" = {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += CString(TEXT("    ")) + CString(ValueInfo.Name) + TEXT(" = ") +
		        CString::FromInt(static_cast<int32_t>(ValueInfo.Value)) + TEXT(",\n");
	}

	Code += TEXT("}\n\n");
	Code += CString(TEXT("_G.")) + EnumName + TEXT(" = ") + EnumName + TEXT("\n\n");

	return Code;
}

CString CLuaBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	CString Code;

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

CString CTypeScriptBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                          const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return CString();
	}

	CString ClassName = BindingInfo.ScriptName.IsEmpty() ? CString(ClassReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	// 生成类型定义
	Code += CString(TEXT("declare class ")) + ClassName + TEXT(" {\n");

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

CString CTypeScriptBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                             const SScriptBindingInfo& BindingInfo,
                                                             const CString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return CString();
	}

	CString FunctionName = BindingInfo.ScriptName.IsEmpty() ? CString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code = TEXT("    ");

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
		Code += CString(Param.Name) + TEXT(": ") + ConvertTypeToTypeScript(*Param.TypeInfo);
	}

	Code += TEXT("): ") + ConvertTypeToTypeScript(*FunctionReflection->ReturnTypeInfo) + TEXT(";\n");

	return Code;
}

CString CTypeScriptBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                             const SScriptBindingInfo& BindingInfo,
                                                             const CString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return CString();
	}

	CString PropertyName = BindingInfo.ScriptName.IsEmpty() ? CString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code = TEXT("    ");

	if (BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable)
	{
		Code += TEXT("readonly ");
	}

	Code += PropertyName + TEXT(": ") + ConvertTypeToTypeScript(*PropertyReflection->TypeInfo) + TEXT(";\n");

	return Code;
}

CString CTypeScriptBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                         const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::TypeScript))
	{
		return CString();
	}

	CString EnumName = BindingInfo.ScriptName.IsEmpty() ? CString(EnumReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	Code += CString(TEXT("declare enum ")) + EnumName + TEXT(" {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += CString(TEXT("    ")) + CString(ValueInfo.Name) + TEXT(" = ") +
		        CString::FromInt(static_cast<int32_t>(ValueInfo.Value)) + TEXT(",\n");
	}

	Code += TEXT("}\n\n");

	return Code;
}

CString CTypeScriptBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	CString Code;

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

CString CTypeScriptBindingGenerator::ConvertTypeToTypeScript(const std::type_info& TypeInfo) const
{
	// 简化的类型转换，实际实现需要更完整
	if (TypeInfo == typeid(bool))
		return TEXT("boolean");
	else if (TypeInfo == typeid(int32_t) || TypeInfo == typeid(int64_t) || TypeInfo == typeid(float) ||
	         TypeInfo == typeid(double))
		return TEXT("number");
	else if (TypeInfo == typeid(CString))
		return TEXT("string");
	else if (TypeInfo == typeid(void))
		return TEXT("void");
	else
		return TEXT("any");
}

CString CTypeScriptBindingGenerator::GenerateTypeDefinition(const SClassReflection* ClassReflection,
                                                            const SScriptBindingInfo& BindingInfo) const
{
	// Call non-const GenerateClassBinding by const_cast since this is a helper method
	return const_cast<CTypeScriptBindingGenerator*>(this)->GenerateClassBinding(ClassReflection, BindingInfo);
}

CString CTypeScriptBindingGenerator::GenerateInterfaceDefinition(const SClassReflection* ClassReflection) const
{
	CString Code;
	Code += CString(TEXT("interface I")) + CString(ClassReflection->Name) + TEXT(" {\n");

	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		Code += CString(TEXT("    ")) + CString(PropInfo.Name) + TEXT(": ") +
		        ConvertTypeToTypeScript(*PropInfo.TypeInfo) + TEXT(";\n");
	}

	for (size_t i = 0; i < ClassReflection->FunctionCount; ++i)
	{
		const SFunctionReflection& FuncInfo = ClassReflection->Functions[i];
		Code += CString(TEXT("    ")) + CString(FuncInfo.Name) + TEXT("(");

		for (size_t j = 0; j < FuncInfo.Parameters.size(); ++j)
		{
			if (j > 0)
				Code += TEXT(", ");
			const auto& Param = FuncInfo.Parameters[j];
			Code += CString(Param.Name) + TEXT(": ") + ConvertTypeToTypeScript(*Param.TypeInfo);
		}

		Code += TEXT("): ") + ConvertTypeToTypeScript(*FuncInfo.ReturnTypeInfo) + TEXT(";\n");
	}

	Code += TEXT("}\n\n");
	return Code;
}

// === CCSharpBindingGenerator 实现 ===

CString CCSharpBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                      const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return CString();
	}

	CString ClassName = BindingInfo.ScriptName.IsEmpty() ? CString(ClassReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	Code += TEXT("using System;\n");
	Code += TEXT("using System.Runtime.InteropServices;\n");
	Code += TEXT("using NLib.Interop;\n\n");

	Code += TEXT("namespace NLib.Generated\n{\n");
	Code += CString(TEXT("    public class ")) + ClassName;

	if (ClassReflection->BaseClassName && strlen(ClassReflection->BaseClassName) > 0)
	{
		Code += CString(TEXT(" : ")) + CString(ClassReflection->BaseClassName);
	}
	else
	{
		Code += TEXT(" : NLibObject");
	}

	Code += TEXT("\n    {\n");

	if (BindingInfo.bScriptCreatable && ClassReflection->Constructor)
	{
		Code += CString(TEXT("        public ")) + ClassName + TEXT("()\n");
		Code += TEXT("        {\n");
		Code += CString(TEXT("            _nativePtr = NLibInterop.CreateObject(\"")) + CString(ClassReflection->Name) +
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

CString CCSharpBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const CString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return CString();
	}

	CString FunctionName = BindingInfo.ScriptName.IsEmpty() ? CString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;

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
		Code += ConvertTypeToCSharp(*Param.TypeInfo) + TEXT(" ") + CString(Param.Name);
	}

	Code += TEXT(")\n        {\n");

	if (BindingInfo.bScriptStatic)
	{
		Code += CString(TEXT("            return NLibInterop.CallStaticMethod<")) +
		        ConvertTypeToCSharp(*FunctionReflection->ReturnTypeInfo) + TEXT(">(\"") + ClassName + TEXT("\", \"") +
		        CString(FunctionReflection->Name) + TEXT("\"");
	}
	else
	{
		Code += CString(TEXT("            return NLibInterop.CallMethod<")) +
		        ConvertTypeToCSharp(*FunctionReflection->ReturnTypeInfo) + TEXT(">(_nativePtr, \"") +
		        CString(FunctionReflection->Name) + TEXT("\"");
	}

	for (size_t i = 0; i < FunctionReflection->Parameters.size(); ++i)
	{
		const auto& Param = FunctionReflection->Parameters[i];
		Code += TEXT(", ") + CString(Param.Name);
	}

	Code += TEXT(");\n        }\n\n");
	return Code;
}

CString CCSharpBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const CString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return CString();
	}

	CString PropertyName = BindingInfo.ScriptName.IsEmpty() ? CString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;

	Code += CString(TEXT("        public ")) + ConvertTypeToCSharp(*PropertyReflection->TypeInfo) + TEXT(" ") +
	        PropertyName + TEXT("\n");
	Code += TEXT("        {\n");

	if (BindingInfo.bScriptReadable)
	{
		Code += TEXT("            get => NLibInterop.GetProperty<") +
		        ConvertTypeToCSharp(*PropertyReflection->TypeInfo) + TEXT(">(_nativePtr, \"") +
		        CString(PropertyReflection->Name) + TEXT("\");\n");
	}

	if (BindingInfo.bScriptWritable)
	{
		Code += TEXT("            set => NLibInterop.SetProperty(_nativePtr, \"") + CString(PropertyReflection->Name) +
		        TEXT("\", value);\n");
	}

	Code += TEXT("        }\n\n");
	return Code;
}

CString CCSharpBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::CSharp))
	{
		return CString();
	}

	CString EnumName = BindingInfo.ScriptName.IsEmpty() ? CString(EnumReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	Code += CString(TEXT("    public enum ")) + EnumName + TEXT("\n    {\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += CString(TEXT("        ")) + CString(ValueInfo.Name) + TEXT(" = ") +
		        CString::FromInt(static_cast<int32_t>(ValueInfo.Value));

		if (i < EnumReflection->ValueCount - 1)
			Code += TEXT(",");
		Code += TEXT("\n");
	}

	Code += TEXT("    }\n\n");
	return Code;
}

CString CCSharpBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	CString Code;

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

CString CCSharpBindingGenerator::ConvertTypeToCSharp(const std::type_info& TypeInfo) const
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
	else if (TypeInfo == typeid(CString))
		return TEXT("string");
	else if (TypeInfo == typeid(void))
		return TEXT("void");
	else
		return TEXT("object");
}

CString CCSharpBindingGenerator::GenerateCSharpClass(const SClassReflection* ClassReflection,
                                                     const SScriptBindingInfo& BindingInfo) const
{
	// Call non-const GenerateClassBinding by const_cast since this is a helper method
	return const_cast<CCSharpBindingGenerator*>(this)->GenerateClassBinding(ClassReflection, BindingInfo);
}

CString CCSharpBindingGenerator::GeneratePInvokeDeclarations(const SClassReflection* ClassReflection) const
{
	CString Code;
	Code += TEXT("    [DllImport(\"NLib\")]\n");
	Code += CString(TEXT("    public static extern IntPtr Create")) + CString(ClassReflection->Name) + TEXT("();\n");
	return Code;
}


// === CPythonBindingGenerator 实现 ===

CString CPythonBindingGenerator::GenerateClassBinding(const SClassReflection* ClassReflection,
                                                      const SScriptBindingInfo& BindingInfo)
{
	if (!ClassReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return CString();
	}

	CString ClassName = BindingInfo.ScriptName.IsEmpty() ? CString(ClassReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	// Python 类定义开始
	Code += CString::Format(TEXT("class {}:\n"), ClassName.GetData());
	Code += TEXT("    \"\"\"\n");
	Code += CString::Format(TEXT("    NLib {} class binding\n"), ClassName.GetData());
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

			CString PropName = PropBindingInfo->ScriptName.IsEmpty() ? CString(PropInfo.Name)
			                                                         : PropBindingInfo->ScriptName;
			CString PropType = ConvertTypeToPython(*PropInfo.TypeInfo);
			Code += CString::Format(TEXT("    {}: {}\n"), PropName.GetData(), PropType.GetData());
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
	Code += CString::Format(TEXT("        return f\"<{} object at {{hex(id(self))}}>\"\n"), ClassName.GetData());
	Code += TEXT("\n");

	Code += TEXT("    def __repr__(self):\n");
	Code += TEXT("        return self.__str__()\n\n");

	return Code;
}

CString CPythonBindingGenerator::GenerateFunctionBinding(const SFunctionReflection* FunctionReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const CString& ClassName)
{
	if (!FunctionReflection || !BindingInfo.bScriptCallable || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return CString();
	}

	CString FunctionName = BindingInfo.ScriptName.IsEmpty() ? CString(FunctionReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;

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
		Code += CString(Param.Name);

		// 类型注解
		CString ParamType = ConvertTypeToPython(*Param.TypeInfo);
		if (!ParamType.IsEmpty())
		{
			Code += TEXT(": ") + ParamType;
		}
	}

	Code += TEXT(")");

	// 返回类型注解
	CString ReturnType = ConvertTypeToPython(*FunctionReflection->ReturnTypeInfo);
	if (!ReturnType.IsEmpty())
	{
		Code += TEXT(" -> ") + ReturnType;
	}

	Code += TEXT(":\n");

	// 文档字符串
	Code += TEXT("        \"\"\"\n");
	Code += CString::Format(TEXT("        {} method binding\n"), FunctionName.GetData());

	if (!FunctionReflection->Parameters.empty())
	{
		Code += TEXT("        \n        Args:\n");
		for (const auto& Param : FunctionReflection->Parameters)
		{
			CString ParamType = ConvertTypeToPython(*Param.TypeInfo);
			Code += CString::Format(TEXT("            {} ({}): Parameter\n"), Param.Name, ParamType.GetData());
		}
	}

	if (*FunctionReflection->ReturnTypeInfo != typeid(void))
	{
		CString RetType = ConvertTypeToPython(*FunctionReflection->ReturnTypeInfo);
		Code += TEXT("        \n        Returns:\n");
		Code += CString::Format(TEXT("            {}: Return value\n"), RetType.GetData());
	}

	Code += TEXT("        \"\"\"\n");

	// 方法体（调用原生C++方法）
	Code += TEXT("        # Call native C++ method\n");
	Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");

	return Code;
}

CString CPythonBindingGenerator::GeneratePropertyBinding(const SPropertyReflection* PropertyReflection,
                                                         const SScriptBindingInfo& BindingInfo,
                                                         const CString& ClassName)
{
	if (!PropertyReflection || (!BindingInfo.bScriptReadable && !BindingInfo.bScriptWritable) ||
	    !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return CString();
	}

	CString PropertyName = BindingInfo.ScriptName.IsEmpty() ? CString(PropertyReflection->Name)
	                                                        : BindingInfo.ScriptName;
	CString Code;
	CString PropertyType = ConvertTypeToPython(*PropertyReflection->TypeInfo);

	// Getter方法
	if (BindingInfo.bScriptReadable)
	{
		Code += TEXT("    @property\n");
		Code += CString::Format(TEXT("    def {}(self)"), PropertyName.GetData());

		if (!PropertyType.IsEmpty())
		{
			Code += TEXT(" -> ") + PropertyType;
		}

		Code += TEXT(":\n");
		Code += CString::Format(TEXT("        \"\"\"Get {} property\"\"\"\n"), PropertyName.GetData());
		Code += TEXT("        # Get native C++ property\n");
		Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");
	}

	// Setter方法
	if (BindingInfo.bScriptWritable)
	{
		Code += CString::Format(TEXT("    @{}.setter\n"), PropertyName.GetData());
		Code += CString::Format(TEXT("    def {}(self, value"), PropertyName.GetData());

		if (!PropertyType.IsEmpty())
		{
			Code += TEXT(": ") + PropertyType;
		}

		Code += TEXT("):\n");
		Code += CString::Format(TEXT("        \"\"\"Set {} property\"\"\"\n"), PropertyName.GetData());
		Code += TEXT("        # Set native C++ property\n");
		Code += TEXT("        pass  # Implementation provided by C++ binding\n\n");
	}

	return Code;
}

CString CPythonBindingGenerator::GenerateEnumBinding(const SEnumReflection* EnumReflection,
                                                     const SScriptBindingInfo& BindingInfo)
{
	if (!EnumReflection || !BindingInfo.ShouldBind() || !BindingInfo.SupportsLanguage(EScriptLanguage::Python))
	{
		return CString();
	}

	CString EnumName = BindingInfo.ScriptName.IsEmpty() ? CString(EnumReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	// 使用Python Enum类
	Code += TEXT("from enum import Enum, IntEnum\n\n");
	Code += CString::Format(TEXT("class {}(IntEnum):\n"), EnumName.GetData());
	Code += TEXT("    \"\"\"\n");
	Code += CString::Format(TEXT("    {} enumeration\n"), EnumName.GetData());
	Code += TEXT("    Auto-generated by NutHeaderTools\n");
	Code += TEXT("    \"\"\"\n");

	for (size_t i = 0; i < EnumReflection->ValueCount; ++i)
	{
		const SEnumValueReflection& ValueInfo = EnumReflection->Values[i];
		Code += CString::Format(TEXT("    {} = {}\n"), ValueInfo.Name, static_cast<int32_t>(ValueInfo.Value));
	}

	Code += TEXT("\n");

	return Code;
}

CString CPythonBindingGenerator::GenerateBindingFile(const TArray<const SClassReflection*, CMemoryManager>& Classes)
{
	CString Code;

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
			CString ClassName = BindingInfo->ScriptName.IsEmpty() ? CString(ClassReflection->Name)
			                                                      : BindingInfo->ScriptName;
			Code += CString::Format(TEXT("    \"{}\",\n"), ClassName.GetData());
		}
	}

	Code += TEXT("]\n");

	return Code;
}

CString CPythonBindingGenerator::ConvertTypeToPython(const std::type_info& TypeInfo) const
{
	// Python类型转换映射
	if (TypeInfo == typeid(bool))
		return TEXT("bool");
	else if (TypeInfo == typeid(int32_t) || TypeInfo == typeid(int64_t))
		return TEXT("int");
	else if (TypeInfo == typeid(float) || TypeInfo == typeid(double))
		return TEXT("float");
	else if (TypeInfo == typeid(CString))
		return TEXT("str");
	else if (TypeInfo == typeid(void))
		return TEXT("None");
	else
		return TEXT("Any");
}

CString CPythonBindingGenerator::GenerateTypeStub(const SClassReflection* ClassReflection,
                                                  const SScriptBindingInfo& BindingInfo) const
{
	// 生成.pyi类型存根文件
	CString ClassName = BindingInfo.ScriptName.IsEmpty() ? CString(ClassReflection->Name) : BindingInfo.ScriptName;
	CString Code;

	Code += CString::Format(TEXT("class {}:\n"), ClassName.GetData());

	// 属性类型注解
	for (size_t i = 0; i < ClassReflection->PropertyCount; ++i)
	{
		const SPropertyReflection& PropInfo = ClassReflection->Properties[i];
		const SScriptBindingInfo* PropBindingInfo = CScriptBindingRegistry::GetInstance().GetPropertyBindingInfo(
		    ClassReflection->Name, PropInfo.Name);

		if (PropBindingInfo && PropBindingInfo->ShouldBind() &&
		    PropBindingInfo->SupportsLanguage(EScriptLanguage::Python))
		{
			CString PropName = PropBindingInfo->ScriptName.IsEmpty() ? CString(PropInfo.Name)
			                                                         : PropBindingInfo->ScriptName;
			CString PropType = ConvertTypeToPython(*PropInfo.TypeInfo);
			Code += CString::Format(TEXT("    {}: {}\n"), PropName.GetData(), PropType.GetData());
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
			CString FunctionName = FuncBindingInfo->ScriptName.IsEmpty() ? CString(FuncInfo.Name)
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
				Code += CString(Param.Name) + TEXT(": ") + ConvertTypeToPython(*Param.TypeInfo);
			}

			Code += TEXT(") -> ") + ConvertTypeToPython(*FuncInfo.ReturnTypeInfo) + TEXT(": ...\n");
		}
	}

	return Code;
}

CString CPythonBindingGenerator::GeneratePyiFile(const TArray<const SClassReflection*, CMemoryManager>& Classes) const
{
	CString Code;

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