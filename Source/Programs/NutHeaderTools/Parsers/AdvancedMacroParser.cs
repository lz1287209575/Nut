using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using NutBuildSystem.Logging;

namespace NutHeaderTools.Parsers
{
    /// <summary>
    /// 高级宏解析器 - 支持复杂的C++类型和语法解析
    /// </summary>
    public class AdvancedMacroParser
    {
        private readonly ILogger logger;
        
        // 正则表达式模式
        private static readonly Regex TemplateTypeRegex = new Regex(
            @"([A-Za-z_]\w*)\s*<([^<>]*(?:<[^<>]*>[^<>]*)*)>",
            RegexOptions.Compiled);
            
        private static readonly Regex FunctionSignatureRegex = new Regex(
            @"([A-Za-z_]\w*(?:<[^<>]*>)?(?:::[A-Za-z_]\w*)*)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)",
            RegexOptions.Compiled);
            
        private static readonly Regex ParameterRegex = new Regex(
            @"(?:const\s+)?([A-Za-z_]\w*(?:<[^<>]*>)?(?:::[A-Za-z_]\w*)*)\s*(?:&\s*|\*\s*)?([A-Za-z_]\w*)?\s*(?:=\s*([^,)]+))?",
            RegexOptions.Compiled);
            
        private static readonly Regex EnumRegex = new Regex(
            @"(?:enum\s+class\s+)?([A-Za-z_]\w*)\s*:\s*([A-Za-z_]\w*)\s*\{([^}]+)\}",
            RegexOptions.Compiled);
            
        private static readonly Regex EnumValueRegex = new Regex(
            @"([A-Za-z_]\w*)(?:\s*=\s*([^,\s]+))?",
            RegexOptions.Compiled);

        public AdvancedMacroParser(ILogger logger)
        {
            this.logger = logger;
        }

        /// <summary>
        /// 解析模板类型
        /// </summary>
        public TemplateTypeInfo? ParseTemplateType(string typeName)
        {
            try
            {
                var match = TemplateTypeRegex.Match(typeName);
                if (!match.Success)
                    return null;

                var templateName = match.Groups[1].Value;
                var templateArgs = ParseTemplateArguments(match.Groups[2].Value);

                return new TemplateTypeInfo
                {
                    TemplateName = templateName,
                    TemplateArguments = templateArgs,
                    FullTypeName = typeName
                };
            }
            catch (Exception ex)
            {
                logger.Warning($"解析模板类型失败: {typeName}, 错误: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 解析模板参数
        /// </summary>
        private List<string> ParseTemplateArguments(string argsString)
        {
            var args = new List<string>();
            var currentArg = new StringBuilder();
            var bracketCount = 0;

            foreach (char c in argsString)
            {
                if (c == '<')
                {
                    bracketCount++;
                    currentArg.Append(c);
                }
                else if (c == '>')
                {
                    bracketCount--;
                    currentArg.Append(c);
                }
                else if (c == ',' && bracketCount == 0)
                {
                    args.Add(currentArg.ToString().Trim());
                    currentArg.Clear();
                }
                else
                {
                    currentArg.Append(c);
                }
            }

            if (currentArg.Length > 0)
            {
                args.Add(currentArg.ToString().Trim());
            }

            return args;
        }

        /// <summary>
        /// 解析函数签名
        /// </summary>
        public FunctionSignatureInfo? ParseFunctionSignature(string signature)
        {
            try
            {
                var match = FunctionSignatureRegex.Match(signature);
                if (!match.Success)
                    return null;

                var returnType = match.Groups[1].Value.Trim();
                var functionName = match.Groups[2].Value.Trim();
                var parametersString = match.Groups[3].Value.Trim();

                var parameters = ParseFunctionParameters(parametersString);

                return new FunctionSignatureInfo
                {
                    ReturnType = returnType,
                    FunctionName = functionName,
                    Parameters = parameters,
                    FullSignature = signature
                };
            }
            catch (Exception ex)
            {
                logger.Warning($"解析函数签名失败: {signature}, 错误: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 解析函数参数
        /// </summary>
        public List<ParameterInfo> ParseFunctionParameters(string parametersString)
        {
            var parameters = new List<ParameterInfo>();

            if (string.IsNullOrWhiteSpace(parametersString))
                return parameters;

            // 处理 void 参数
            if (parametersString.Trim() == "void")
                return parameters;

            var paramMatches = ParameterRegex.Matches(parametersString);
            foreach (Match match in paramMatches)
            {
                var paramType = match.Groups[1].Value.Trim();
                var paramName = match.Groups[2].Success ? match.Groups[2].Value.Trim() : "";
                var defaultValue = match.Groups[3].Success ? match.Groups[3].Value.Trim() : "";

                // 分析参数修饰符
                var modifiers = AnalyzeParameterModifiers(match.Value);

                parameters.Add(new ParameterInfo
                {
                    Type = paramType,
                    Name = paramName,
                    DefaultValue = defaultValue,
                    IsConst = modifiers.IsConst,
                    IsReference = modifiers.IsReference,
                    IsPointer = modifiers.IsPointer
                });
            }

            return parameters;
        }

        /// <summary>
        /// 分析参数修饰符
        /// </summary>
        private ParameterModifiers AnalyzeParameterModifiers(string parameterString)
        {
            return new ParameterModifiers
            {
                IsConst = parameterString.Contains("const"),
                IsReference = parameterString.Contains("&"),
                IsPointer = parameterString.Contains("*")
            };
        }

        /// <summary>
        /// 解析枚举定义
        /// </summary>
        public EnumInfo? ParseEnum(string enumContent)
        {
            try
            {
                var match = EnumRegex.Match(enumContent);
                if (!match.Success)
                    return null;

                var enumName = match.Groups[1].Value.Trim();
                var underlyingType = match.Groups[2].Success ? match.Groups[2].Value.Trim() : "int";
                var valuesString = match.Groups[3].Value.Trim();

                var values = ParseEnumValues(valuesString);

                return new EnumInfo
                {
                    Name = enumName,
                    UnderlyingType = underlyingType,
                    Values = values,
                    FullContent = enumContent
                };
            }
            catch (Exception ex)
            {
                logger.Warning($"解析枚举失败: {enumContent}, 错误: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// 解析枚举值
        /// </summary>
        private List<EnumValueInfo> ParseEnumValues(string valuesString)
        {
            var values = new List<EnumValueInfo>();
            var valueMatches = EnumValueRegex.Matches(valuesString);

            foreach (Match match in valueMatches)
            {
                var valueName = match.Groups[1].Value.Trim();
                var valueNumber = match.Groups[2].Success ? match.Groups[2].Value.Trim() : "";

                values.Add(new EnumValueInfo
                {
                    Name = valueName,
                    Value = valueNumber
                });
            }

            return values;
        }

        /// <summary>
        /// 验证类型名称是否有效
        /// </summary>
        public bool IsValidTypeName(string typeName)
        {
            if (string.IsNullOrWhiteSpace(typeName))
                return false;

            // 基本类型检查
            var basicTypes = new[] { "void", "bool", "char", "int", "float", "double", "long", "short", "unsigned", "signed" };
            if (basicTypes.Contains(typeName))
                return true;

            // 检查是否为有效的标识符
            if (!Regex.IsMatch(typeName, @"^[A-Za-z_][A-Za-z0-9_:<>]*$"))
                return false;

            // 检查模板语法
            var bracketCount = 0;
            foreach (char c in typeName)
            {
                if (c == '<') bracketCount++;
                else if (c == '>') bracketCount--;
                
                if (bracketCount < 0) return false;
            }

            return bracketCount == 0;
        }

        /// <summary>
        /// 验证函数名是否有效
        /// </summary>
        public bool IsValidFunctionName(string functionName)
        {
            if (string.IsNullOrWhiteSpace(functionName))
                return false;

            return Regex.IsMatch(functionName, @"^[A-Za-z_][A-Za-z0-9_]*$");
        }

        /// <summary>
        /// 清理和标准化类型名称
        /// </summary>
        public string NormalizeTypeName(string typeName)
        {
            if (string.IsNullOrWhiteSpace(typeName))
                return typeName;

            // 移除多余的空格
            var normalized = Regex.Replace(typeName, @"\s+", " ").Trim();

            // 标准化模板语法
            normalized = Regex.Replace(normalized, @"\s*<\s*", "<");
            normalized = Regex.Replace(normalized, @"\s*>\s*", ">");
            normalized = Regex.Replace(normalized, @"\s*,\s*", ", ");

            return normalized;
        }
    }

    // === 数据结构定义 ===

    public class TemplateTypeInfo
    {
        public string TemplateName { get; set; } = "";
        public List<string> TemplateArguments { get; set; } = new();
        public string FullTypeName { get; set; } = "";
    }

    public class FunctionSignatureInfo
    {
        public string ReturnType { get; set; } = "";
        public string FunctionName { get; set; } = "";
        public List<ParameterInfo> Parameters { get; set; } = new();
        public string FullSignature { get; set; } = "";
    }

    public class ParameterInfo
    {
        public string Type { get; set; } = "";
        public string Name { get; set; } = "";
        public string DefaultValue { get; set; } = "";
        public bool IsConst { get; set; }
        public bool IsReference { get; set; }
        public bool IsPointer { get; set; }
    }

    public class ParameterModifiers
    {
        public bool IsConst { get; set; }
        public bool IsReference { get; set; }
        public bool IsPointer { get; set; }
    }

    public class EnumInfo
    {
        public string Name { get; set; } = "";
        public string UnderlyingType { get; set; } = "int";
        public List<EnumValueInfo> Values { get; set; } = new();
        public string FullContent { get; set; } = "";
    }

    public class EnumValueInfo
    {
        public string Name { get; set; } = "";
        public string Value { get; set; } = "";
    }
}
