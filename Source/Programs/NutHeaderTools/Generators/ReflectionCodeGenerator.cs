using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NutBuildSystem.Logging;
using NutHeaderTools.Validators;
using NutHeaderTools.Parsers;
using NutHeaderTools;

namespace NutHeaderTools.Generators
{
    /// <summary>
    /// 反射代码生成器 - 生成完整的反射代码
    /// </summary>
    public class ReflectionCodeGenerator
    {
        private readonly ILogger logger;
        private readonly AdvancedMacroParser macroParser;

        public ReflectionCodeGenerator(ILogger logger)
        {
            this.logger = logger;
            this.macroParser = new AdvancedMacroParser(logger);
        }

        /// <summary>
        /// 生成类的完整反射代码
        /// </summary>
        public string GenerateClassReflectionCode(ClassInfo classInfo)
        {
            var code = new StringBuilder();

            // 生成属性反射数组
            if (classInfo.Properties.Count > 0)
            {
                code.AppendLine(GeneratePropertyReflectionArray(classInfo));
                code.AppendLine();
            }

            // 生成函数反射数组
            if (classInfo.Functions.Count > 0)
            {
                code.AppendLine(GenerateFunctionReflectionArray(classInfo));
                code.AppendLine();
            }

            // 生成类反射结构
            code.AppendLine(GenerateClassReflectionStruct(classInfo));
            code.AppendLine();

            // 生成静态函数声明
            code.AppendLine(GenerateStaticFunctionDeclarations(classInfo));
            code.AppendLine();

            // 生成反射注册代码
            code.AppendLine(GenerateReflectionRegistration(classInfo));

            return code.ToString();
        }

        /// <summary>
        /// 生成属性反射数组
        /// </summary>
        private string GeneratePropertyReflectionArray(ClassInfo classInfo)
        {
            return GeneratePropertyReflectionArrayInternal(classInfo.Name, classInfo.Properties);
        }

        /// <summary>
        /// 生成结构体属性反射数组
        /// </summary>
        private string GeneratePropertyReflectionArrayForStruct(StructInfo structInfo)
        {
            return GeneratePropertyReflectionArrayInternal(structInfo.Name, structInfo.Properties);
        }

        /// <summary>
        /// 生成属性反射数组的内部实现
        /// </summary>
        private string GeneratePropertyReflectionArrayInternal(string typeName, List<PropertyInfo> properties)
        {
            var code = new StringBuilder();
            code.AppendLine($"// {typeName} 属性反射数组");
            code.AppendLine($"static const SPropertyReflection {typeName}_Properties[] = {{");

            for (int i = 0; i < properties.Count; i++)
            {
                var property = properties[i];
                var normalizedType = macroParser.NormalizeTypeName(property.Type);

                code.AppendLine("    {");
                code.AppendLine($"        \"{property.Name}\",           // Name");
                code.AppendLine($"        \"{normalizedType}\",           // TypeName");
                code.AppendLine($"        offsetof({typeName}, {property.Name}), // Offset");
                code.AppendLine($"        sizeof({normalizedType}),      // Size");
                code.AppendLine($"        &typeid({normalizedType}),     // TypeInfo");
                code.AppendLine($"        {GeneratePropertyFlags(property)}, // Flags");
                code.AppendLine("        nullptr,                      // Category");
                code.AppendLine("        nullptr,                      // DisplayName");
                code.AppendLine("        nullptr,                      // ToolTip");
                code.AppendLine("        std::any{},                   // DefaultValue");
                code.AppendLine($"        {GeneratePropertyGetter(typeName, property)}, // Getter");
                code.AppendLine($"        {GeneratePropertySetter(typeName, property)}  // Setter");
                code.Append("    }");
                if (i < properties.Count - 1)
                    code.AppendLine(",");
                else
                    code.AppendLine();
            }

            code.AppendLine("};");
            return code.ToString();
        }

        /// <summary>
        /// 生成函数反射数组
        /// </summary>
        private string GenerateFunctionReflectionArray(ClassInfo classInfo)
        {
            var code = new StringBuilder();
            code.AppendLine($"// {classInfo.Name} 函数反射数组");
            code.AppendLine($"static const SFunctionReflection {classInfo.Name}_Functions[] = {{");

            for (int i = 0; i < classInfo.Functions.Count; i++)
            {
                var function = classInfo.Functions[i];
                var normalizedReturnType = macroParser.NormalizeTypeName(function.ReturnType);

                code.AppendLine("    {");
                code.AppendLine($"        \"{function.Name}\",          // Name");
                code.AppendLine($"        \"{normalizedReturnType}\",    // ReturnTypeName");
                code.AppendLine($"        &typeid({normalizedReturnType}), // ReturnTypeInfo");
                code.AppendLine($"        {GenerateFunctionFlags(function)}, // Flags");
                code.AppendLine("        nullptr,                      // Category");
                code.AppendLine("        nullptr,                      // DisplayName");
                code.AppendLine("        nullptr,                      // ToolTip");
                code.AppendLine("        {},                           // Parameters");
                code.AppendLine($"        {GenerateFunctionInvoker(classInfo.Name, function)}  // Invoker");
                code.Append("    }");
                if (i < classInfo.Functions.Count - 1)
                    code.AppendLine(",");
                else
                    code.AppendLine();
            }

            code.AppendLine("};");
            return code.ToString();
        }

        /// <summary>
        /// 生成类反射结构
        /// </summary>
        private string GenerateClassReflectionStruct(ClassInfo classInfo)
        {
            var code = new StringBuilder();
            code.AppendLine($"// {classInfo.Name} 类反射结构");
            code.AppendLine($"static const SClassReflection {classInfo.Name}_ClassReflection = {{");
            code.AppendLine($"    \"{classInfo.Name}\",              // Name");
            code.AppendLine($"    \"{classInfo.BaseClass}\",         // BaseClassName");
            code.AppendLine($"    sizeof({classInfo.Name}),          // Size");
            code.AppendLine($"    &typeid({classInfo.Name}),         // TypeInfo");
            code.AppendLine($"    {GenerateClassFlags(classInfo)},   // Flags");
            code.AppendLine("    nullptr,                           // Category");
            code.AppendLine("    nullptr,                           // DisplayName");
            code.AppendLine("    nullptr,                           // ToolTip");

            if (classInfo.Properties.Count > 0)
            {
                code.AppendLine($"    {classInfo.Name}_Properties,       // Properties");
                code.AppendLine($"    {classInfo.Properties.Count},        // PropertyCount");
            }
            else
            {
                code.AppendLine("    nullptr,                           // Properties");
                code.AppendLine("    0,                                 // PropertyCount");
            }

            if (classInfo.Functions.Count > 0)
            {
                code.AppendLine($"    {classInfo.Name}_Functions,        // Functions");
                code.AppendLine($"    {classInfo.Functions.Count},         // FunctionCount");
            }
            else
            {
                code.AppendLine("    nullptr,                           // Functions");
                code.AppendLine("    0,                                 // FunctionCount");
            }

            code.AppendLine($"    {GenerateConstructor(classInfo.Name)} // Constructor");
            code.AppendLine("};");

            return code.ToString();
        }

        /// <summary>
        /// 生成静态函数声明
        /// </summary>
        private string GenerateStaticFunctionDeclarations(ClassInfo classInfo)
        {
            var code = new StringBuilder();
            code.AppendLine($"// {classInfo.Name} 静态函数声明");
            code.AppendLine($"const char* {classInfo.Name}::GetStaticTypeName() {{ return \"{classInfo.Name}\"; }}");
            code.AppendLine($"const SClassReflection* {classInfo.Name}::GetStaticClassReflection() {{ return &{classInfo.Name}_ClassReflection; }}");
            code.AppendLine($"NObject* {classInfo.Name}::CreateDefaultObject() {{ return new {classInfo.Name}(); }}");

            return code.ToString();
        }

        /// <summary>
        /// 生成反射注册代码
        /// </summary>
        private string GenerateReflectionRegistration(ClassInfo classInfo)
        {
            var code = new StringBuilder();
            code.AppendLine($"// {classInfo.Name} 反射注册");
            code.AppendLine($"REGISTER_NCLASS_REFLECTION({classInfo.Name})");

            return code.ToString();
        }

        /// <summary>
        /// 生成属性标志
        /// </summary>
        private string GeneratePropertyFlags(PropertyInfo property)
        {
            var flags = new List<string>();

            // 根据属性名称和类型推断标志
            if (property.Name.Contains("Edit") || property.Name.Contains("Visible"))
                flags.Add("EPropertyFlags::EditAnywhere");

            if (property.Name.Contains("ReadOnly"))
                flags.Add("EPropertyFlags::BlueprintReadOnly");
            else
                flags.Add("EPropertyFlags::BlueprintReadWrite");

            if (property.Name.Contains("Save"))
                flags.Add("EPropertyFlags::SaveGame");

            if (property.Name.Contains("Temp") || property.Name.Contains("Transient"))
                flags.Add("EPropertyFlags::Transient");

            return flags.Count > 0 ? string.Join(" | ", flags) : "EPropertyFlags::None";
        }

        /// <summary>
        /// 生成函数标志
        /// </summary>
        private string GenerateFunctionFlags(FunctionInfo function)
        {
            var flags = new List<string>();

            // 根据函数名称推断标志
            if (function.Name.Contains("Get") || function.Name.Contains("Is"))
                flags.Add("EFunctionFlags::Pure");

            if (function.Name.Contains("Callable") || function.Name.Contains("Call"))
                flags.Add("EFunctionFlags::BlueprintCallable");

            if (function.Name.Contains("Static") || function.Name.StartsWith("Create"))
                flags.Add("EFunctionFlags::Static");

            if (function.Name.Contains("Const") || function.Name.EndsWith("() const"))
                flags.Add("EFunctionFlags::Const");

            return flags.Count > 0 ? string.Join(" | ", flags) : "EFunctionFlags::None";
        }

        /// <summary>
        /// 生成类标志
        /// </summary>
        private string GenerateClassFlags(ClassInfo classInfo)
        {
            var flags = new List<string>();

            // 根据类名和基类推断标志
            if (classInfo.Name.Contains("Abstract") || classInfo.Name.Contains("Interface"))
                flags.Add("EClassFlags::Abstract");

            if (classInfo.Name.Contains("Blueprint") || classInfo.Name.Contains("Script"))
                flags.Add("EClassFlags::BlueprintType");

            return flags.Count > 0 ? string.Join(" | ", flags) : "EClassFlags::None";
        }

        /// <summary>
        /// 生成属性获取器
        /// </summary>
        private string GeneratePropertyGetter(string className, PropertyInfo property)
        {
            var normalizedType = macroParser.NormalizeTypeName(property.Type);
            
            return $"[&](const NObject* obj) -> std::any {{ " +
                   $"const {className}* instance = static_cast<const {className}*>(obj); " +
                   $"return std::make_any<{normalizedType}>(instance->{property.Name}); }}";
        }

        /// <summary>
        /// 生成属性设置器
        /// </summary>
        private string GeneratePropertySetter(string className, PropertyInfo property)
        {
            var normalizedType = macroParser.NormalizeTypeName(property.Type);
            
            return $"[&](NObject* obj, const std::any& value) {{ " +
                   $"{className}* instance = static_cast<{className}*>(obj); " +
                   $"instance->{property.Name} = std::any_cast<{normalizedType}>(value); }}";
        }

        /// <summary>
        /// 生成函数调用器
        /// </summary>
        private string GenerateFunctionInvoker(string className, FunctionInfo function)
        {
            var normalizedReturnType = macroParser.NormalizeTypeName(function.ReturnType);
            
            if (normalizedReturnType == "void")
            {
                return $"[&](NObject* obj, const std::vector<std::any>& args) -> std::any {{ " +
                       $"{className}* instance = static_cast<{className}*>(obj); " +
                       $"instance->{function.Name}(); " +
                       $"return std::any(); }}";
            }
            else
            {
                return $"[&](NObject* obj, const std::vector<std::any>& args) -> std::any {{ " +
                       $"{className}* instance = static_cast<{className}*>(obj); " +
                       $"return std::make_any<{normalizedReturnType}>(instance->{function.Name}()); }}";
            }
        }

        /// <summary>
        /// 生成构造函数
        /// </summary>
        private string GenerateConstructor(string className)
        {
            return $"[&]() -> NObject* {{ return new {className}(); }}";
        }

        /// <summary>
        /// 生成枚举反射代码
        /// </summary>
        public string GenerateEnumReflectionCode(EnumInfo enumInfo)
        {
            var code = new StringBuilder();

            // 生成枚举值反射数组
            code.AppendLine($"// {enumInfo.Name} 枚举值反射数组");
            code.AppendLine($"static const SEnumValueReflection {enumInfo.Name}_Values[] = {{");

            for (int i = 0; i < enumInfo.Values.Count; i++)
            {
                var value = enumInfo.Values[i];
                code.AppendLine("    {");
                code.AppendLine($"        \"{value.Name}\",        // Name");
                code.AppendLine($"        {value.Value},           // Value");
                code.AppendLine($"        \"{value.Name}\",        // DisplayName");
                code.AppendLine("        nullptr                   // ToolTip");
                code.Append("    }");
                if (i < enumInfo.Values.Count - 1)
                    code.AppendLine(",");
                else
                    code.AppendLine();
            }

            code.AppendLine("};");
            code.AppendLine();

            // 生成枚举反射结构
            code.AppendLine($"// {enumInfo.Name} 枚举反射结构");
            code.AppendLine($"static const SEnumReflection {enumInfo.Name}_EnumReflection = {{");
            code.AppendLine($"    \"{enumInfo.Name}\",               // Name");
            code.AppendLine($"    &typeid({enumInfo.Name}),          // TypeInfo");
            code.AppendLine("    nullptr,                           // Category");
            code.AppendLine($"    \"{enumInfo.Name}\",               // DisplayName");
            code.AppendLine("    nullptr,                           // ToolTip");
            code.AppendLine($"    {enumInfo.Name}_Values,            // Values");
            code.AppendLine($"    {enumInfo.Values.Count}            // ValueCount");
            code.AppendLine("};");

            return code.ToString();
        }

        /// <summary>
        /// 生成结构体反射代码
        /// </summary>
        public string GenerateStructReflectionCode(StructInfo structInfo)
        {
            var code = new StringBuilder();

            // 生成属性反射数组（与类相同）
            if (structInfo.Properties.Count > 0)
            {
                code.AppendLine(GeneratePropertyReflectionArrayForStruct(structInfo));
                code.AppendLine();
            }

            // 生成结构体反射结构
            code.AppendLine($"// {structInfo.Name} 结构体反射结构");
            code.AppendLine($"static const SStructReflection {structInfo.Name}_StructReflection = {{");
            code.AppendLine($"    \"{structInfo.Name}\",              // Name");
            code.AppendLine($"    sizeof({structInfo.Name}),          // Size");
            code.AppendLine($"    &typeid({structInfo.Name}),         // TypeInfo");
            code.AppendLine("    nullptr,                           // Category");
            code.AppendLine("    nullptr,                           // DisplayName");
            code.AppendLine("    nullptr,                           // ToolTip");

            if (structInfo.Properties.Count > 0)
            {
                code.AppendLine($"    {structInfo.Name}_Properties,       // Properties");
                code.AppendLine($"    {structInfo.Properties.Count}        // PropertyCount");
            }
            else
            {
                code.AppendLine("    nullptr,                           // Properties");
                code.AppendLine("    0,                                 // PropertyCount");
            }

            code.AppendLine("};");

            return code.ToString();
        }
    }

    // === 扩展数据结构 ===

    public class StructInfo
    {
        public string Name { get; set; } = "";
        public List<PropertyInfo> Properties { get; set; } = new();
    }
}
