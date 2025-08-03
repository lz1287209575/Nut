#include "Scripting/NScriptBindingGenerator.h"
#include "Scripting/NScriptMeta.h"
#include "Memory/CAllocator.h"
#include "Core/CLogger.h"
#include "FileSystem/NFileSystem.h"

namespace NLib
{

// NScriptBindingGenerator Implementation
NScriptBindingGenerator::NScriptBindingGenerator()
    : TargetLanguages(EScriptLanguage::All)
    , bGenerateComments(true)
    , bGenerateTypeChecks(true)
    , IndentSize(4)
{
}

NScriptBindingGenerator::~NScriptBindingGenerator()
{
}

bool NScriptBindingGenerator::GenerateBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory)
{
    if (ClassNames.IsEmpty())
    {
        CLogger::Warning("No classes specified for binding generation");
        return false;
    }
    
    if (!NFileSystem::DirectoryExists(OutputDirectory))
    {
        if (!NFileSystem::CreateDirectoryTree(OutputDirectory))
        {
            CLogger::Error("Failed to create output directory: %s", *OutputDirectory);
            return false;
        }
    }
    
    CLogger::Info("Generating script bindings for %d classes...", ClassNames.Num());
    
    bool bSuccess = true;
    auto& MetaRegistry = NScriptMetaRegistry::Get();
    
    // Generate bindings for each target language
    if (EnumHasAnyFlags(TargetLanguages, EScriptLanguage::Lua))
    {
        bSuccess &= GenerateLuaBindings(ClassNames, OutputDirectory, MetaRegistry);
    }
    
    if (EnumHasAnyFlags(TargetLanguages, EScriptLanguage::Python))
    {
        bSuccess &= GeneratePythonBindings(ClassNames, OutputDirectory, MetaRegistry);
    }
    
    if (EnumHasAnyFlags(TargetLanguages, EScriptLanguage::TypeScript))
    {
        bSuccess &= GenerateTypeScriptBindings(ClassNames, OutputDirectory, MetaRegistry);
    }
    
    if (EnumHasAnyFlags(TargetLanguages, EScriptLanguage::CSharp))
    {
        bSuccess &= GenerateCSharpBindings(ClassNames, OutputDirectory, MetaRegistry);
    }
    
    if (EnumHasAnyFlags(TargetLanguages, EScriptLanguage::NBP))
    {
        bSuccess &= GenerateNBPBindings(ClassNames, OutputDirectory, MetaRegistry);
    }
    
    if (bSuccess)
    {
        CLogger::Info("Script bindings generated successfully");
    }
    else
    {
        CLogger::Error("Failed to generate some script bindings");
    }
    
    return bSuccess;
}

bool NScriptBindingGenerator::GenerateClassBinding(const CString& ClassName, EScriptLanguage Language, CString& OutCode)
{
    auto& MetaRegistry = NScriptMetaRegistry::Get();
    const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
    
    if (!ClassMeta)
    {
        CLogger::Error("Class metadata not found: %s", *ClassName);
        return false;
    }
    
    switch (Language)
    {
        case EScriptLanguage::Lua:
            OutCode = GenerateLuaClass(ClassName, *ClassMeta);
            break;
        case EScriptLanguage::Python:
            OutCode = GeneratePythonClass(ClassName, *ClassMeta);
            break;
        case EScriptLanguage::TypeScript:
            OutCode = GenerateTypeScriptClass(ClassName, *ClassMeta);
            break;
        case EScriptLanguage::CSharp:
            OutCode = GenerateCSharpClass(ClassName, *ClassMeta);
            break;
        case EScriptLanguage::NBP:
            OutCode = GenerateNBPClass(ClassName, *ClassMeta);
            break;
        default:
            return false;
    }
    
    return !OutCode.IsEmpty();
}

CString NScriptBindingGenerator::GenerateModuleBinding(const CArray<CString>& ClassNames, EScriptLanguage Language)
{
    switch (Language)
    {
        case EScriptLanguage::Lua:
            return GenerateLuaModule(ClassNames);
        case EScriptLanguage::Python:
            return GeneratePythonModule(ClassNames);
        case EScriptLanguage::TypeScript:
            return GenerateTypeScriptModule(ClassNames);
        case EScriptLanguage::CSharp:
            return GenerateCSharpModule(ClassNames);
        case EScriptLanguage::NBP:
            return GenerateNBPModule(ClassNames);
        default:
            return CString();
    }
}

void NScriptBindingGenerator::SetTargetLanguages(EScriptLanguage Languages)
{
    TargetLanguages = Languages;
}

void NScriptBindingGenerator::SetGenerationOptions(const NScriptGenerationOptions& Options)
{
    bGenerateComments = Options.bGenerateComments;
    bGenerateTypeChecks = Options.bGenerateTypeChecks;
    IndentSize = Options.IndentSize;
    CustomTemplate = Options.CustomTemplate;
}

bool NScriptBindingGenerator::GenerateLuaBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory, NScriptMetaRegistry& MetaRegistry)
{
    CString LuaDir = NFileSystem::CombinePaths(OutputDirectory, "lua");
    if (!NFileSystem::CreateDirectoryTree(LuaDir))
    {
        return false;
    }
    
    // Generate individual class files
    for (const CString& ClassName : ClassNames)
    {
        const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
        if (!ClassMeta || !ClassMeta->MetaInfo.HasLanguage(EScriptLanguage::Lua))
        {
            continue;
        }
        
        CString ClassCode = GenerateLuaClass(ClassName, *ClassMeta);
        CString FilePath = NFileSystem::CombinePaths(LuaDir, ClassName + ".lua");
        
        if (!NFileSystem::WriteStringToFile(FilePath, ClassCode))
        {
            CLogger::Error("Failed to write Lua binding file: %s", *FilePath);
            return false;
        }
    }
    
    // Generate module init file
    CString ModuleCode = GenerateLuaModule(ClassNames);
    CString ModulePath = NFileSystem::CombinePaths(LuaDir, "init.lua");
    
    return NFileSystem::WriteStringToFile(ModulePath, ModuleCode);
}

bool NScriptBindingGenerator::GeneratePythonBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory, NScriptMetaRegistry& MetaRegistry)
{
    CString PythonDir = NFileSystem::CombinePaths(OutputDirectory, "python");
    if (!NFileSystem::CreateDirectoryTree(PythonDir))
    {
        return false;
    }
    
    // Generate __init__.py
    CString InitCode = GeneratePythonModule(ClassNames);
    CString InitPath = NFileSystem::CombinePaths(PythonDir, "__init__.py");
    
    if (!NFileSystem::WriteStringToFile(InitPath, InitCode))
    {
        return false;
    }
    
    // Generate individual class files
    for (const CString& ClassName : ClassNames)
    {
        const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
        if (!ClassMeta || !ClassMeta->MetaInfo.HasLanguage(EScriptLanguage::Python))
        {
            continue;
        }
        
        CString ClassCode = GeneratePythonClass(ClassName, *ClassMeta);
        CString FilePath = NFileSystem::CombinePaths(PythonDir, ClassName.ToLower() + ".py");
        
        if (!NFileSystem::WriteStringToFile(FilePath, ClassCode))
        {
            CLogger::Error("Failed to write Python binding file: %s", *FilePath);
            return false;
        }
    }
    
    return true;
}

bool NScriptBindingGenerator::GenerateTypeScriptBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory, NScriptMetaRegistry& MetaRegistry)
{
    CString TSDir = NFileSystem::CombinePaths(OutputDirectory, "typescript");
    if (!NFileSystem::CreateDirectoryTree(TSDir))
    {
        return false;
    }
    
    // Generate index.ts
    CString IndexCode = GenerateTypeScriptModule(ClassNames);
    CString IndexPath = NFileSystem::CombinePaths(TSDir, "index.ts");
    
    if (!NFileSystem::WriteStringToFile(IndexPath, IndexCode))
    {
        return false;
    }
    
    // Generate type definitions
    for (const CString& ClassName : ClassNames)
    {
        const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
        if (!ClassMeta || !ClassMeta->MetaInfo.HasLanguage(EScriptLanguage::TypeScript))
        {
            continue;
        }
        
        CString ClassCode = GenerateTypeScriptClass(ClassName, *ClassMeta);
        CString FilePath = NFileSystem::CombinePaths(TSDir, ClassName + ".ts");
        
        if (!NFileSystem::WriteStringToFile(FilePath, ClassCode))
        {
            CLogger::Error("Failed to write TypeScript binding file: %s", *FilePath);
            return false;
        }
    }
    
    return true;
}

bool NScriptBindingGenerator::GenerateCSharpBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory, NScriptMetaRegistry& MetaRegistry)
{
    CString CSharpDir = NFileSystem::CombinePaths(OutputDirectory, "csharp");
    if (!NFileSystem::CreateDirectoryTree(CSharpDir))
    {
        return false;
    }
    
    // Generate individual class files
    for (const CString& ClassName : ClassNames)
    {
        const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
        if (!ClassMeta || !ClassMeta->MetaInfo.HasLanguage(EScriptLanguage::CSharp))
        {
            continue;
        }
        
        CString ClassCode = GenerateCSharpClass(ClassName, *ClassMeta);
        CString FilePath = NFileSystem::CombinePaths(CSharpDir, ClassName + ".cs");
        
        if (!NFileSystem::WriteStringToFile(FilePath, ClassCode))
        {
            CLogger::Error("Failed to write C# binding file: %s", *FilePath);
            return false;
        }
    }
    
    return true;
}

bool NScriptBindingGenerator::GenerateNBPBindings(const CArray<CString>& ClassNames, const CString& OutputDirectory, NScriptMetaRegistry& MetaRegistry)
{
    CString NBPDir = NFileSystem::CombinePaths(OutputDirectory, "nbp");
    if (!NFileSystem::CreateDirectoryTree(NBPDir))
    {
        return false;
    }
    
    // Generate individual class files
    for (const CString& ClassName : ClassNames)
    {
        const NScriptClassMeta* ClassMeta = MetaRegistry.GetClassMeta(ClassName);
        if (!ClassMeta || !ClassMeta->MetaInfo.HasLanguage(EScriptLanguage::NBP))
        {
            continue;
        }
        
        CString ClassCode = GenerateNBPClass(ClassName, *ClassMeta);
        CString FilePath = NFileSystem::CombinePaths(NBPDir, ClassName + ".nbp");
        
        if (!NFileSystem::WriteStringToFile(FilePath, ClassCode))
        {
            CLogger::Error("Failed to write NBP binding file: %s", *FilePath);
            return false;
        }
    }
    
    return true;
}

CString NScriptBindingGenerator::GenerateLuaClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    if (bGenerateComments && !Meta.MetaInfo.Description.IsEmpty())
    {
        Code += CString::Printf("-- %s\n", *Meta.MetaInfo.Description);
        Code += CString::Printf("-- Category: %s\n\n", *Meta.MetaInfo.Category);
    }
    
    // Class table
    Code += CString::Printf("local %s = {}\n", *ClassName);
    Code += CString::Printf("%s.__index = %s\n\n", *ClassName, *ClassName);
    
    // Constructor
    Code += CString::Printf("function %s.new(...)\n", *ClassName);
    Code += GetIndent() + "local self = setmetatable({}, " + ClassName + ")\n";
    Code += GetIndent() + "return self\n";
    Code += "end\n\n";
    
    // Properties
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        if (PropMeta.IsReadable())
        {
            Code += CString::Printf("function %s:get_%s()\n", *ClassName, *PropName.ToLower());
            Code += GetIndent() + "-- TODO: Implement property getter\n";
            Code += GetIndent() + "return nil\n";
            Code += "end\n\n";
        }
        
        if (PropMeta.IsWritable())
        {
            Code += CString::Printf("function %s:set_%s(value)\n", *ClassName, *PropName.ToLower());
            Code += GetIndent() + "-- TODO: Implement property setter\n";
            Code += "end\n\n";
        }
    }
    
    // Functions
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += CString::Printf("function %s:%s(", *ClassName, *FuncName.ToLower());
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            if (i > 0) Code += ", ";
            Code += FuncMeta.GetParameterName(i).ToLower();
        }
        Code += ")\n";
        
        if (bGenerateComments && !FuncMeta.Description.IsEmpty())
        {
            Code += GetIndent() + "-- " + FuncMeta.Description + "\n";
        }
        
        Code += GetIndent() + "-- TODO: Implement function\n";
        Code += GetIndent() + "return nil\n";
        Code += "end\n\n";
    }
    
    Code += CString::Printf("return %s\n", *ClassName);
    return Code;
}

CString NScriptBindingGenerator::GeneratePythonClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    // Imports
    Code += "from typing import Optional, Any\n";
    Code += "from nlib.core import CObject\n\n";
    
    // Class definition
    Code += CString::Printf("class %s(CObject):\n", *ClassName);
    
    if (bGenerateComments && !Meta.MetaInfo.Description.IsEmpty())
    {
        Code += GetIndent() + "\"\"\"" + Meta.MetaInfo.Description + "\n";
        Code += GetIndent() + "\n";
        Code += GetIndent() + "Category: " + Meta.MetaInfo.Category + "\n";
        Code += GetIndent() + "\"\"\"\n\n";
    }
    
    // Constructor
    Code += GetIndent() + "def __init__(self):\n";
    Code += GetIndent(2) + "super().__init__()\n";
    Code += GetIndent(2) + "# TODO: Initialize properties\n\n";
    
    // Properties
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        Code += GetIndent() + "@property\n";
        Code += GetIndent() + CString::Printf("def %s(self) -> Any:\n", *PropName.ToLower());
        if (bGenerateComments && !PropMeta.Description.IsEmpty())
        {
            Code += GetIndent(2) + "\"\"\"" + PropMeta.Description + "\"\"\"\n";
        }
        Code += GetIndent(2) + "# TODO: Implement property getter\n";
        Code += GetIndent(2) + "return None\n\n";
        
        if (PropMeta.IsWritable())
        {
            Code += GetIndent() + CString::Printf("@%s.setter\n", *PropName.ToLower());
            Code += GetIndent() + CString::Printf("def %s(self, value: Any) -> None:\n", *PropName.ToLower());
            Code += GetIndent(2) + "# TODO: Implement property setter\n";
            Code += GetIndent(2) + "pass\n\n";
        }
    }
    
    // Functions
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += GetIndent() + CString::Printf("def %s(self", *FuncName.ToLower());
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            Code += ", " + FuncMeta.GetParameterName(i).ToLower() + ": Any";
        }
        Code += ") -> Any:\n";
        
        if (bGenerateComments && !FuncMeta.Description.IsEmpty())
        {
            Code += GetIndent(2) + "\"\"\"" + FuncMeta.Description + "\"\"\"\n";
        }
        
        Code += GetIndent(2) + "# TODO: Implement function\n";
        Code += GetIndent(2) + "return None\n\n";
    }
    
    return Code;
}

CString NScriptBindingGenerator::GenerateTypeScriptClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    // Imports
    Code += "import { CObject } from './CObject';\n\n";
    
    // Interface definition
    Code += CString::Printf("interface I%s {\n", *ClassName);
    
    // Properties in interface
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        if (PropMeta.IsWritable())
        {
            Code += GetIndent() + PropName.ToLower() + ": any;\n";
        }
        else
        {
            Code += GetIndent() + "readonly " + PropName.ToLower() + ": any;\n";
        }
    }
    
    // Functions in interface
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += GetIndent() + FuncName.ToLower() + "(";
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            if (i > 0) Code += ", ";
            Code += FuncMeta.GetParameterName(i).ToLower() + ": any";
        }
        Code += "): any;\n";
    }
    
    Code += "}\n\n";
    
    // Class implementation
    if (bGenerateComments && !Meta.MetaInfo.Description.IsEmpty())
    {
        Code += "/**\n";
        Code += " * " + Meta.MetaInfo.Description + "\n";
        Code += " * Category: " + Meta.MetaInfo.Category + "\n";
        Code += " */\n";
    }
    
    Code += CString::Printf("export class %s extends CObject implements I%s {\n", *ClassName, *ClassName);
    
    // Properties
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        Code += GetIndent() + "private _" + PropName.ToLower() + ": any = null;\n";
    }
    
    Code += "\n";
    
    // Constructor
    Code += GetIndent() + "constructor() {\n";
    Code += GetIndent(2) + "super();\n";
    Code += GetIndent() + "}\n\n";
    
    // Property accessors
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        Code += GetIndent() + "get " + PropName.ToLower() + "(): any {\n";
        Code += GetIndent(2) + "return this._" + PropName.ToLower() + ";\n";
        Code += GetIndent() + "}\n\n";
        
        if (PropMeta.IsWritable())
        {
            Code += GetIndent() + "set " + PropName.ToLower() + "(value: any) {\n";
            Code += GetIndent(2) + "this._" + PropName.ToLower() + " = value;\n";
            Code += GetIndent() + "}\n\n";
        }
    }
    
    // Functions
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += GetIndent() + FuncName.ToLower() + "(";
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            if (i > 0) Code += ", ";
            Code += FuncMeta.GetParameterName(i).ToLower() + ": any";
        }
        Code += "): any {\n";
        
        Code += GetIndent(2) + "// TODO: Implement function\n";
        Code += GetIndent(2) + "return null;\n";
        Code += GetIndent() + "}\n\n";
    }
    
    Code += "}\n";
    return Code;
}

CString NScriptBindingGenerator::GenerateCSharpClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    // Usings
    Code += "using System;\n";
    Code += "using NLib.Scripting;\n\n";
    
    Code += "namespace NLib.Generated\n{\n";
    
    // Class definition
    if (bGenerateComments && !Meta.MetaInfo.Description.IsEmpty())
    {
        Code += GetIndent() + "/// <summary>\n";
        Code += GetIndent() + "/// " + Meta.MetaInfo.Description + "\n";
        Code += GetIndent() + "/// </summary>\n";
    }
    
    Code += GetIndent() + CString::Printf("public class %s : CObject\n", *ClassName);
    Code += GetIndent() + "{\n";
    
    // Properties
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        Code += GetIndent(2) + "private object _" + PropName.ToLower() + ";\n\n";
        
        if (bGenerateComments && !PropMeta.Description.IsEmpty())
        {
            Code += GetIndent(2) + "/// <summary>\n";
            Code += GetIndent(2) + "/// " + PropMeta.Description + "\n";
            Code += GetIndent(2) + "/// </summary>\n";
        }
        
        Code += GetIndent(2) + "public object " + PropName + "\n";
        Code += GetIndent(2) + "{\n";
        Code += GetIndent(3) + "get => _" + PropName.ToLower() + ";\n";
        
        if (PropMeta.IsWritable())
        {
            Code += GetIndent(3) + "set => _" + PropName.ToLower() + " = value;\n";
        }
        
        Code += GetIndent(2) + "}\n\n";
    }
    
    // Functions
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        if (bGenerateComments && !FuncMeta.Description.IsEmpty())
        {
            Code += GetIndent(2) + "/// <summary>\n";
            Code += GetIndent(2) + "/// " + FuncMeta.Description + "\n";
            Code += GetIndent(2) + "/// </summary>\n";
        }
        
        Code += GetIndent(2) + "public object " + FuncName + "(";
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            if (i > 0) Code += ", ";
            Code += "object " + FuncMeta.GetParameterName(i).ToLower();
        }
        Code += ")\n";
        Code += GetIndent(2) + "{\n";
        Code += GetIndent(3) + "// TODO: Implement function\n";
        Code += GetIndent(3) + "return null;\n";
        Code += GetIndent(2) + "}\n\n";
    }
    
    Code += GetIndent() + "}\n";
    Code += "}\n";
    
    return Code;
}

CString NScriptBindingGenerator::GenerateNBPClass(const CString& ClassName, const NScriptClassMeta& Meta)
{
    CString Code;
    
    // NBP class definition
    Code += "class " + ClassName + " {\n";
    
    if (bGenerateComments && !Meta.MetaInfo.Description.IsEmpty())
    {
        Code += GetIndent() + "// " + Meta.MetaInfo.Description + "\n";
        Code += GetIndent() + "// Category: " + Meta.MetaInfo.Category + "\n\n";
    }
    
    // Properties
    Code += GetIndent() + "properties {\n";
    for (const auto& PropPair : Meta.Properties)
    {
        const CString& PropName = PropPair.Key;
        const NScriptPropertyMeta& PropMeta = PropPair.Value;
        
        Code += GetIndent(2) + "var " + PropName.ToLower() + ": any";
        if (PropMeta.IsReadable() && !PropMeta.IsWritable())
        {
            Code += " [readonly]";
        }
        Code += ";\n";
    }
    Code += GetIndent() + "}\n\n";
    
    // Functions
    Code += GetIndent() + "functions {\n";
    for (const auto& FuncPair : Meta.Functions)
    {
        const CString& FuncName = FuncPair.Key;
        const NScriptFunctionMeta& FuncMeta = FuncPair.Value;
        
        Code += GetIndent(2) + "function " + FuncName.ToLower() + "(";
        for (int32_t i = 0; i < FuncMeta.GetParameterCount(); ++i)
        {
            if (i > 0) Code += ", ";
            Code += FuncMeta.GetParameterName(i).ToLower() + ": any";
        }
        Code += "): any;\n";
    }
    Code += GetIndent() + "}\n";
    
    Code += "}\n";
    return Code;
}

CString NScriptBindingGenerator::GenerateLuaModule(const CArray<CString>& ClassNames)
{
    CString Code;
    Code += "-- NLib Lua Module\n";
    Code += "-- Auto-generated script bindings\n\n";
    
    Code += "local NLib = {}\n\n";
    
    for (const CString& ClassName : ClassNames)
    {
        Code += CString::Printf("NLib.%s = require('%s')\n", *ClassName, *ClassName);
    }
    
    Code += "\nreturn NLib\n";
    return Code;
}

CString NScriptBindingGenerator::GeneratePythonModule(const CArray<CString>& ClassNames)
{
    CString Code;
    Code += "\"\"\"NLib Python Module\n";
    Code += "Auto-generated script bindings\n";
    Code += "\"\"\"\n\n";
    
    for (const CString& ClassName : ClassNames)
    {
        Code += CString::Printf("from .%s import %s\n", *ClassName.ToLower(), *ClassName);
    }
    
    Code += "\n__all__ = [\n";
    for (int32_t i = 0; i < ClassNames.Num(); ++i)
    {
        Code += CString::Printf("    '%s'", *ClassNames[i]);
        if (i < ClassNames.Num() - 1) Code += ",";
        Code += "\n";
    }
    Code += "]\n";
    
    return Code;
}

CString NScriptBindingGenerator::GenerateTypeScriptModule(const CArray<CString>& ClassNames)
{
    CString Code;
    Code += "// NLib TypeScript Module\n";
    Code += "// Auto-generated script bindings\n\n";
    
    for (const CString& ClassName : ClassNames)
    {
        Code += CString::Printf("export { %s } from './%s';\n", *ClassName, *ClassName);
    }
    
    return Code;
}

CString NScriptBindingGenerator::GenerateCSharpModule(const CArray<CString>& ClassNames)
{
    CString Code;
    Code += "// NLib C# Module\n";
    Code += "// Auto-generated script bindings\n\n";
    
    Code += "using System;\n";
    Code += "using NLib.Scripting;\n\n";
    
    Code += "namespace NLib.Generated\n{\n";
    Code += GetIndent() + "public static class CLibModule\n";
    Code += GetIndent() + "{\n";
    Code += GetIndent(2) + "public static void Initialize()\n";
    Code += GetIndent(2) + "{\n";
    Code += GetIndent(3) + "// Initialize all generated classes\n";
    Code += GetIndent(2) + "}\n";
    Code += GetIndent() + "}\n";
    Code += "}\n";
    
    return Code;
}

CString NScriptBindingGenerator::GenerateNBPModule(const CArray<CString>& ClassNames)
{
    CString Code;
    Code += "// NLib NBP Module\n";
    Code += "// Auto-generated script bindings\n\n";
    
    Code += "module NLib {\n";
    
    for (const CString& ClassName : ClassNames)
    {
        Code += GetIndent() + "import " + ClassName + ";\n";
    }
    
    Code += "}\n";
    return Code;
}

CString NScriptBindingGenerator::GetIndent(int32_t Level) const
{
    CString Indent;
    for (int32_t i = 0; i < Level * IndentSize; ++i)
    {
        Indent += " ";
    }
    return Indent;
}

} // namespace NLib