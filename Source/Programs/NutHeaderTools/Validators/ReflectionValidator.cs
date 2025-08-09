using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using NutBuildSystem.Logging;
using NutHeaderTools.Parsers;

namespace NutHeaderTools.Validators
{
    /// <summary>
    /// 反射验证器 - 验证解析的反射信息的正确性
    /// </summary>
    public class ReflectionValidator
    {
        private readonly ILogger logger;
        private readonly AdvancedMacroParser macroParser;

        // 预定义的类型和关键字
        private static readonly HashSet<string> ValidTypes = new(StringComparer.OrdinalIgnoreCase)
        {
            // 基本类型
            "void", "bool", "char", "int", "float", "double", "long", "short", "unsigned", "signed",
            "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
            "size_t", "ptrdiff_t", "nullptr_t",
            
            // 标准库类型
            "string", "wstring", "vector", "map", "unordered_map", "set", "unordered_set",
            "list", "deque", "queue", "stack", "array", "tuple", "pair", "optional", "variant",
            
            // Nut引擎类型
            "NObject", "CString", "TString", "TArray", "THashMap", "TQueue", "FVector2", "FVector3", "FVector4",
            "FMatrix", "FQuaternion", "FTransform", "FColor", "FLinearColor", "FBox", "FSphere", "FPlane"
        };

        private static readonly HashSet<string> ReservedKeywords = new(StringComparer.OrdinalIgnoreCase)
        {
            "auto", "break", "case", "catch", "class", "const", "constexpr", "continue", "default", "delete",
            "do", "else", "enum", "explicit", "export", "extern", "false", "for", "friend", "goto", "if",
            "inline", "mutable", "namespace", "new", "noexcept", "nullptr", "operator", "private", "protected",
            "public", "register", "return", "sizeof", "static", "struct", "switch", "template", "this", "throw",
            "true", "try", "typedef", "typename", "union", "using", "virtual", "volatile", "while"
        };

        public ReflectionValidator(ILogger logger)
        {
            this.logger = logger;
            this.macroParser = new AdvancedMacroParser(logger);
        }

        /// <summary>
        /// 验证类信息
        /// </summary>
        public ValidationResult ValidateClass(ClassInfo classInfo)
        {
            var result = new ValidationResult();

            // 验证类名
            if (!ValidateClassName(classInfo.Name))
            {
                result.AddError($"无效的类名: {classInfo.Name}");
            }

            // 验证基类
            if (!ValidateBaseClass(classInfo.BaseClass))
            {
                result.AddWarning($"可能无效的基类: {classInfo.BaseClass}");
            }

            // 验证属性
            foreach (var property in classInfo.Properties)
            {
                var propertyResult = ValidateProperty(property);
                result.Merge(propertyResult);
            }

            // 验证函数
            foreach (var function in classInfo.Functions)
            {
                var functionResult = ValidateFunction(function);
                result.Merge(functionResult);
            }

            // 检查重复名称
            CheckDuplicateNames(classInfo, result);

            return result;
        }

        /// <summary>
        /// 验证属性信息
        /// </summary>
        public ValidationResult ValidateProperty(PropertyInfo property)
        {
            var result = new ValidationResult();

            // 验证属性名
            if (!ValidatePropertyName(property.Name))
            {
                result.AddError($"无效的属性名: {property.Name}");
            }

            // 验证类型
            if (!ValidatePropertyType(property.Type))
            {
                result.AddError($"无效的属性类型: {property.Type}");
            }

            return result;
        }

        /// <summary>
        /// 验证函数信息
        /// </summary>
        public ValidationResult ValidateFunction(FunctionInfo function)
        {
            var result = new ValidationResult();

            // 验证函数名
            if (!ValidateFunctionName(function.Name))
            {
                result.AddError($"无效的函数名: {function.Name}");
            }

            // 验证返回类型
            if (!ValidateReturnType(function.ReturnType))
            {
                result.AddError($"无效的返回类型: {function.ReturnType}");
            }

            return result;
        }

        /// <summary>
        /// 验证枚举信息
        /// </summary>
        public ValidationResult ValidateEnum(EnumInfo enumInfo)
        {
            var result = new ValidationResult();

            // 验证枚举名
            if (!ValidateEnumName(enumInfo.Name))
            {
                result.AddError($"无效的枚举名: {enumInfo.Name}");
            }

            // 验证底层类型
            if (!ValidateUnderlyingType(enumInfo.UnderlyingType))
            {
                result.AddError($"无效的底层类型: {enumInfo.UnderlyingType}");
            }

            // 验证枚举值
            foreach (var value in enumInfo.Values)
            {
                if (!ValidateEnumValue(value))
                {
                    result.AddError($"无效的枚举值: {value.Name}");
                }
            }

            return result;
        }

        /// <summary>
        /// 验证类名
        /// </summary>
        private bool ValidateClassName(string className)
        {
            if (string.IsNullOrWhiteSpace(className))
                return false;

            // 检查是否为保留关键字
            if (ReservedKeywords.Contains(className))
                return false;

            // 检查命名规范（通常以大写字母开头）
            if (!char.IsUpper(className[0]))
                return false;

            // 检查字符有效性
            return Regex.IsMatch(className, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证基类
        /// </summary>
        private bool ValidateBaseClass(string baseClass)
        {
            if (string.IsNullOrWhiteSpace(baseClass))
                return false;

            // 检查是否为已知类型
            if (ValidTypes.Contains(baseClass))
                return true;

            // 检查是否为有效的标识符
            return Regex.IsMatch(baseClass, @"^[A-Za-z_][A-Za-z0-9_:<>]*$");
        }

        /// <summary>
        /// 验证属性名
        /// </summary>
        private bool ValidatePropertyName(string propertyName)
        {
            if (string.IsNullOrWhiteSpace(propertyName))
                return false;

            // 检查是否为保留关键字
            if (ReservedKeywords.Contains(propertyName))
                return false;

            // 检查字符有效性
            return Regex.IsMatch(propertyName, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证属性类型
        /// </summary>
        private bool ValidatePropertyType(string typeName)
        {
            if (string.IsNullOrWhiteSpace(typeName))
                return false;

            // 检查是否为已知类型
            if (ValidTypes.Contains(typeName))
                return true;

            // 检查是否为模板类型
            if (typeName.Contains('<') && typeName.Contains('>'))
            {
                return macroParser.IsValidTypeName(typeName);
            }

            // 检查是否为有效的标识符
            return Regex.IsMatch(typeName, @"^[A-Za-z_][A-Za-z0-9_:<>]*$");
        }

        /// <summary>
        /// 验证函数名
        /// </summary>
        private bool ValidateFunctionName(string functionName)
        {
            if (string.IsNullOrWhiteSpace(functionName))
                return false;

            // 检查是否为保留关键字
            if (ReservedKeywords.Contains(functionName))
                return false;

            // 检查字符有效性
            return Regex.IsMatch(functionName, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证返回类型
        /// </summary>
        private bool ValidateReturnType(string returnType)
        {
            if (string.IsNullOrWhiteSpace(returnType))
                return false;

            // 检查是否为已知类型
            if (ValidTypes.Contains(returnType))
                return true;

            // 检查是否为模板类型
            if (returnType.Contains('<') && returnType.Contains('>'))
            {
                return macroParser.IsValidTypeName(returnType);
            }

            // 检查是否为有效的标识符
            return Regex.IsMatch(returnType, @"^[A-Za-z_][A-Za-z0-9_:<>]*$");
        }

        /// <summary>
        /// 验证参数
        /// </summary>
        private bool ValidateParameter(string parameter)
        {
            if (string.IsNullOrWhiteSpace(parameter))
                return false;

            // 简单的参数验证
            return Regex.IsMatch(parameter, @"^[A-Za-z_][A-Za-z0-9_]*\s+[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证枚举名
        /// </summary>
        private bool ValidateEnumName(string enumName)
        {
            if (string.IsNullOrWhiteSpace(enumName))
                return false;

            // 检查是否为保留关键字
            if (ReservedKeywords.Contains(enumName))
                return false;

            // 检查命名规范（通常以E开头）
            if (!enumName.StartsWith("E"))
                return false;

            // 检查字符有效性
            return Regex.IsMatch(enumName, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证底层类型
        /// </summary>
        private bool ValidateUnderlyingType(string underlyingType)
        {
            var validTypes = new[] { "int", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t" };
            return validTypes.Contains(underlyingType);
        }

        /// <summary>
        /// 验证枚举值
        /// </summary>
        private bool ValidateEnumValue(EnumValueInfo enumValue)
        {
            if (string.IsNullOrWhiteSpace(enumValue.Name))
                return false;

            // 检查是否为保留关键字
            if (ReservedKeywords.Contains(enumValue.Name))
                return false;

            // 检查字符有效性
            return Regex.IsMatch(enumValue.Name, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 验证属性标志
        /// </summary>
        private bool ValidatePropertyAttributes(string attributes)
        {
            if (string.IsNullOrWhiteSpace(attributes))
                return true;

            var validFlags = new[] { "EditAnywhere", "EditDefaultsOnly", "EditInstanceOnly", "VisibleAnywhere", 
                                   "VisibleDefaultsOnly", "VisibleInstanceOnly", "BlueprintReadOnly", "BlueprintReadWrite",
                                   "SaveGame", "Transient", "Config", "Replicated", "ReplicatedUsing", "NotReplicated",
                                   "Interp", "NonTransactional" };

            var flags = attributes.Split(',', StringSplitOptions.RemoveEmptyEntries);
            return flags.All(flag => validFlags.Contains(flag.Trim()));
        }

        /// <summary>
        /// 验证函数标志
        /// </summary>
        private bool ValidateFunctionAttributes(string attributes)
        {
            if (string.IsNullOrWhiteSpace(attributes))
                return true;

            var validFlags = new[] { "BlueprintCallable", "BlueprintImplementableEvent", "BlueprintNativeEvent",
                                   "CallInEditor", "Exec", "Server", "Client", "NetMulticast", "Reliable",
                                   "Unreliable", "WithValidation", "Pure", "Const", "Static", "Final", "Override" };

            var flags = attributes.Split(',', StringSplitOptions.RemoveEmptyEntries);
            return flags.All(flag => validFlags.Contains(flag.Trim()));
        }

        /// <summary>
        /// 检查重复名称
        /// </summary>
        private void CheckDuplicateNames(ClassInfo classInfo, ValidationResult result)
        {
            var allNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            // 检查属性名重复
            foreach (var property in classInfo.Properties)
            {
                if (!allNames.Add(property.Name))
                {
                    result.AddError($"重复的属性名: {property.Name}");
                }
            }

            // 检查函数名重复
            foreach (var function in classInfo.Functions)
            {
                if (!allNames.Add(function.Name))
                {
                    result.AddError($"重复的函数名: {function.Name}");
                }
            }
        }

        /// <summary>
        /// 解析函数参数
        /// </summary>
        private List<string> ParseFunctionParameters(string parametersString)
        {
            var parameters = new List<string>();
            
            if (string.IsNullOrWhiteSpace(parametersString) || parametersString.Trim() == "void")
                return parameters;

            // 简单的参数分割（不考虑嵌套括号）
            var parts = parametersString.Split(',');
            foreach (var part in parts)
            {
                var trimmed = part.Trim();
                if (!string.IsNullOrEmpty(trimmed))
                {
                    parameters.Add(trimmed);
                }
            }

            return parameters;
        }

        /// <summary>
        /// 验证继承关系
        /// </summary>
        public bool ValidateInheritance(ClassInfo classInfo, List<ClassInfo> allClasses)
        {
            if (classInfo.BaseClass == "NObject")
                return true;

            // 查找基类
            var baseClass = allClasses.FirstOrDefault(c => c.Name == classInfo.BaseClass);
            if (baseClass == null)
            {
                logger.Warning($"未找到基类: {classInfo.BaseClass}");
                return false;
            }

            // 检查循环继承
            return !HasCircularInheritance(classInfo, baseClass, allClasses);
        }

        /// <summary>
        /// 检查循环继承
        /// </summary>
        private bool HasCircularInheritance(ClassInfo originalClass, ClassInfo currentClass, List<ClassInfo> allClasses)
        {
            if (currentClass.BaseClass == originalClass.Name)
                return true;

            if (currentClass.BaseClass == "NObject")
                return false;

            var nextBaseClass = allClasses.FirstOrDefault(c => c.Name == currentClass.BaseClass);
            if (nextBaseClass == null)
                return false;

            return HasCircularInheritance(originalClass, nextBaseClass, allClasses);
        }
    }

    /// <summary>
    /// 验证结果
    /// </summary>
    public class ValidationResult
    {
        public List<string> Errors { get; } = new();
        public List<string> Warnings { get; } = new();

        public bool IsValid => Errors.Count == 0;
        public bool HasWarnings => Warnings.Count > 0;

        public void AddError(string error)
        {
            Errors.Add(error);
        }

        public void AddWarning(string warning)
        {
            Warnings.Add(warning);
        }

        public void Merge(ValidationResult other)
        {
            Errors.AddRange(other.Errors);
            Warnings.AddRange(other.Warnings);
        }

        public override string ToString()
        {
            var result = new System.Text.StringBuilder();
            
            if (Errors.Count > 0)
            {
                result.AppendLine($"发现 {Errors.Count} 个错误:");
                foreach (var error in Errors)
                {
                    result.AppendLine($"  ❌ {error}");
                }
            }

            if (Warnings.Count > 0)
            {
                result.AppendLine($"发现 {Warnings.Count} 个警告:");
                foreach (var warning in Warnings)
                {
                    result.AppendLine($"  ⚠️ {warning}");
                }
            }

            return result.ToString();
        }
    }
}
