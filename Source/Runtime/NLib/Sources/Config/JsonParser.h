#pragma once

#include "ConfigValue.h"
#include "Core/Object.h"
#include "Logging/LogCategory.h"

namespace NLib
{
/**
 * @brief JSON解析错误信息
 */
struct SJsonParseError
{
	CString Message;      // 错误消息
	int32_t Line = 0;     // 行号
	int32_t Column = 0;   // 列号
	int32_t Position = 0; // 字符位置

	SJsonParseError() = default;

	SJsonParseError(const CString& InMessage, int32_t InLine = 0, int32_t InColumn = 0, int32_t InPosition = 0)
	    : Message(InMessage),
	      Line(InLine),
	      Column(InColumn),
	      Position(InPosition)
	{}

	CString ToString() const
	{
		if (Line > 0 && Column > 0)
		{
			return CString("JSON Parse Error at line ") + CString::FromInt(Line) + CString(", column ") +
			       CString::FromInt(Column) + CString(": ") + Message;
		}
		else
		{
			return CString("JSON Parse Error: ") + Message;
		}
	}
};

/**
 * @brief JSON解析结果
 */
struct SJsonParseResult
{
	bool bSuccess = false; // 是否成功
	CConfigValue Value;    // 解析结果
	SJsonParseError Error; // 错误信息

	SJsonParseResult() = default;

	explicit SJsonParseResult(const CConfigValue& InValue)
	    : bSuccess(true),
	      Value(InValue)
	{}

	explicit SJsonParseResult(const SJsonParseError& InError)
	    : bSuccess(false),
	      Error(InError)
	{}

	operator bool() const
	{
		return bSuccess;
	}
};

/**
 * @brief JSON解析器
 *
 * 提供JSON字符串到ConfigValue的解析功能：
 * - 标准JSON格式支持
 * - 详细的错误报告
 * - 位置信息跟踪
 * - 注释支持（可选）
 */
class CJsonParser
{
public:
	/**
	 * @brief 解析选项
	 */
	struct SParseOptions
	{
		bool bAllowComments;       // 是否允许注释
		bool bAllowTrailingCommas; // 是否允许尾随逗号
		bool bAllowUnquotedKeys;  // 是否允许不带引号的键
		int32_t MaxDepth;          // 最大嵌套深度
		
		SParseOptions() 
		    : bAllowComments(true),
		      bAllowTrailingCommas(true),
		      bAllowUnquotedKeys(false),
		      MaxDepth(1000)
		{}
	};

public:
	/**
	 * @brief 解析JSON字符串
	 */
	static SJsonParseResult Parse(const CString& JsonString, const SParseOptions& Options = SParseOptions());

	/**
	 * @brief 解析JSON文件
	 */
	static SJsonParseResult ParseFile(const CString& FilePath, const SParseOptions& Options = SParseOptions());

private:
	// === 解析器实现类 ===

	class CJsonParserImpl
	{
	public:
		explicit CJsonParserImpl(const CString& InJson, const SParseOptions& InOptions);

		SJsonParseResult Parse();

	private:
		// === 字符处理 ===

		char CurrentChar() const;
		char PeekChar(int32_t Offset = 1) const;
		void AdvanceChar();
		bool IsAtEnd() const;

		void SkipWhitespace();
		void SkipComment();

		// === 位置跟踪 ===

		void UpdatePosition(char Ch);
		SJsonParseError CreateError(const CString& Message) const;

		// === 解析方法 ===

		CConfigValue ParseValue();
		CConfigValue ParseNull();
		CConfigValue ParseBool();
		CConfigValue ParseNumber();
		CConfigValue ParseString();
		CConfigValue ParseArray();
		CConfigValue ParseObject();

		CString ParseStringLiteral();
		CString UnescapeString(const CString& Str);

		// === 验证方法 ===

		bool ExpectChar(char Expected);
		bool MatchKeyword(const CString& Keyword);

	private:
		const CString& Json;          // JSON字符串
		const SParseOptions& Options; // 解析选项

		int32_t Position = 0; // 当前位置
		int32_t Line = 1;     // 当前行
		int32_t Column = 1;   // 当前列
		int32_t Depth = 0;    // 当前嵌套深度

		SJsonParseError LastError; // 最后的错误
	};
};

/**
 * @brief JSON生成器选项
 */
struct SJsonGenerateOptions
{
	bool bPrettyPrint;    // 是否格式化输出
	int32_t IndentSize;      // 缩进大小
	bool bSortKeys;      // 是否排序键
	bool bEscapeUnicode; // 是否转义Unicode字符
	
	SJsonGenerateOptions()
	    : bPrettyPrint(true),
	      IndentSize(2),
	      bSortKeys(false),
	      bEscapeUnicode(false)
	{}
};

/**
 * @brief JSON生成器
 *
 * 提供ConfigValue到JSON字符串的生成功能
 */
class CJsonGenerator
{
public:
	/**
	 * @brief 生成JSON字符串
	 */
	static CString Generate(const CConfigValue& Value, const SJsonGenerateOptions& Options = SJsonGenerateOptions());

	/**
	 * @brief 写入JSON文件
	 */
	static bool WriteToFile(const CConfigValue& Value,
	                        const CString& FilePath,
	                        const SJsonGenerateOptions& Options = SJsonGenerateOptions());

private:
	class CJsonGeneratorImpl
	{
	public:
		explicit CJsonGeneratorImpl(const SJsonGenerateOptions& InOptions);

		CString Generate(const CConfigValue& Value);

	private:
		CString GenerateValue(const CConfigValue& Value, int32_t Indent = 0);
		CString GenerateString(const CString& Str);
		CString GenerateArray(const CConfigArray& Array, int32_t Indent = 0);
		CString GenerateObject(const CConfigObject& Object, int32_t Indent = 0);

		CString GetIndent(int32_t Level) const;
		CString EscapeString(const CString& Str) const;

	private:
		const SJsonGenerateOptions& Options;
	};
};

// === 便捷函数 ===

/**
 * @brief 解析JSON字符串的便捷函数
 */
inline SJsonParseResult ParseJson(const CString& JsonString, bool bAllowComments = true)
{
	CJsonParser::SParseOptions Options;
	Options.bAllowComments = bAllowComments;
	return CJsonParser::Parse(JsonString, Options);
}

/**
 * @brief 解析JSON文件的便捷函数
 */
inline SJsonParseResult ParseJsonFile(const CString& FilePath, bool bAllowComments = true)
{
	CJsonParser::SParseOptions Options;
	Options.bAllowComments = bAllowComments;
	return CJsonParser::ParseFile(FilePath, Options);
}

/**
 * @brief 生成JSON字符串的便捷函数
 */
inline CString ToJson(const CConfigValue& Value, bool bPrettyPrint = true)
{
	SJsonGenerateOptions Options;
	Options.bPrettyPrint = bPrettyPrint;
	return CJsonGenerator::Generate(Value, Options);
}

/**
 * @brief 写入JSON文件的便捷函数
 */
inline bool WriteJsonFile(const CConfigValue& Value, const CString& FilePath, bool bPrettyPrint = true)
{
	SJsonGenerateOptions Options;
	Options.bPrettyPrint = bPrettyPrint;
	return CJsonGenerator::WriteToFile(Value, FilePath, Options);
}

// === 类型别名 ===
using FJsonParser = CJsonParser;
using FJsonGenerator = CJsonGenerator;
using FJsonParseResult = SJsonParseResult;
using FJsonParseError = SJsonParseError;

} // namespace NLib
