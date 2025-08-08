#include "JsonParser.h"

#include "Logging/LogCategory.h"

#include <cctype>
#include <cmath>
#include <fstream>

namespace NLib
{
// === CJsonParserImpl 实现 ===

CJsonParser::CJsonParserImpl::CJsonParserImpl(const CString& InJson, const SParseOptions& InOptions)
    : Json(InJson),
      Options(InOptions)
{}

SJsonParseResult CJsonParser::CJsonParserImpl::Parse()
{
	try
	{
		SkipWhitespace();

		if (IsAtEnd())
		{
			return SJsonParseResult(CreateError("Empty JSON input"));
		}

		CConfigValue Result = ParseValue();

		SkipWhitespace();
		if (!IsAtEnd())
		{
			return SJsonParseResult(CreateError("Unexpected characters after JSON value"));
		}

		return SJsonParseResult(Result);
	}
	catch (const SJsonParseError& Error)
	{
		return SJsonParseResult(Error);
	}
	catch (...)
	{
		return SJsonParseResult(CreateError("Unknown parsing error"));
	}
}

char CJsonParser::CJsonParserImpl::CurrentChar() const
{
	if (IsAtEnd())
	{
		return '\0';
	}
	return Json[Position];
}

char CJsonParser::CJsonParserImpl::PeekChar(int32_t Offset) const
{
	int32_t PeekPos = Position + Offset;
	if (PeekPos >= static_cast<int32_t>(Json.Size()))
	{
		return '\0';
	}
	return Json[PeekPos];
}

void CJsonParser::CJsonParserImpl::AdvanceChar()
{
	if (!IsAtEnd())
	{
		UpdatePosition(CurrentChar());
		Position++;
	}
}

bool CJsonParser::CJsonParserImpl::IsAtEnd() const
{
	return Position >= static_cast<int32_t>(Json.Size());
}

void CJsonParser::CJsonParserImpl::SkipWhitespace()
{
	while (!IsAtEnd())
	{
		char Ch = CurrentChar();
		if (Ch == ' ' || Ch == '\t' || Ch == '\r' || Ch == '\n')
		{
			AdvanceChar();
		}
		else if (Options.bAllowComments && Ch == '/')
		{
			if (PeekChar() == '/' || PeekChar() == '*')
			{
				SkipComment();
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
}

void CJsonParser::CJsonParserImpl::SkipComment()
{
	char Ch = CurrentChar();
	if (Ch == '/' && PeekChar() == '/')
	{
		// 单行注释
		AdvanceChar(); // 跳过第一个 '/'
		AdvanceChar(); // 跳过第二个 '/'

		while (!IsAtEnd() && CurrentChar() != '\n')
		{
			AdvanceChar();
		}

		if (CurrentChar() == '\n')
		{
			AdvanceChar();
		}
	}
	else if (Ch == '/' && PeekChar() == '*')
	{
		// 多行注释
		AdvanceChar(); // 跳过 '/'
		AdvanceChar(); // 跳过 '*'

		while (!IsAtEnd())
		{
			if (CurrentChar() == '*' && PeekChar() == '/')
			{
				AdvanceChar(); // 跳过 '*'
				AdvanceChar(); // 跳过 '/'
				break;
			}
			AdvanceChar();
		}
	}
}

void CJsonParser::CJsonParserImpl::UpdatePosition(char Ch)
{
	if (Ch == '\n')
	{
		Line++;
		Column = 1;
	}
	else
	{
		Column++;
	}
}

SJsonParseError CJsonParser::CJsonParserImpl::CreateError(const CString& Message) const
{
	return SJsonParseError(Message, Line, Column, Position);
}

CConfigValue CJsonParser::CJsonParserImpl::ParseValue()
{
	if (Depth >= Options.MaxDepth)
	{
		throw CreateError("Maximum nesting depth exceeded");
	}

	SkipWhitespace();

	char Ch = CurrentChar();
	switch (Ch)
	{
	case 'n':
		return ParseNull();
	case 't':
	case 'f':
		return ParseBool();
	case '"':
		return ParseString();
	case '[':
		return ParseArray();
	case '{':
		return ParseObject();
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return ParseNumber();
	default:
		throw CreateError(CString("Unexpected character: ") + CString(&Ch, 1));
	}
}

CConfigValue CJsonParser::CJsonParserImpl::ParseNull()
{
	if (!MatchKeyword("null"))
	{
		throw CreateError("Invalid null value");
	}
	return CConfigValue();
}

CConfigValue CJsonParser::CJsonParserImpl::ParseBool()
{
	if (MatchKeyword("true"))
	{
		return CConfigValue(true);
	}
	else if (MatchKeyword("false"))
	{
		return CConfigValue(false);
	}
	else
	{
		throw CreateError("Invalid boolean value");
	}
}

CConfigValue CJsonParser::CJsonParserImpl::ParseNumber()
{
	int32_t StartPos = Position;
	// bool bIsNegative = false; // 未使用的变量
	bool bHasDecimal = false;
	bool bHasExponent = false;

	// 处理负号
	if (CurrentChar() == '-')
	{
		// bIsNegative = true;
		AdvanceChar();
	}

	// 整数部分
	if (CurrentChar() == '0')
	{
		AdvanceChar();
	}
	else if (CurrentChar() >= '1' && CurrentChar() <= '9')
	{
		while (CurrentChar() >= '0' && CurrentChar() <= '9')
		{
			AdvanceChar();
		}
	}
	else
	{
		throw CreateError("Invalid number format");
	}

	// 小数部分
	if (CurrentChar() == '.')
	{
		bHasDecimal = true;
		AdvanceChar();

		if (CurrentChar() < '0' || CurrentChar() > '9')
		{
			throw CreateError("Invalid decimal number format");
		}

		while (CurrentChar() >= '0' && CurrentChar() <= '9')
		{
			AdvanceChar();
		}
	}

	// 指数部分
	if (CurrentChar() == 'e' || CurrentChar() == 'E')
	{
		bHasExponent = true;
		AdvanceChar();

		if (CurrentChar() == '+' || CurrentChar() == '-')
		{
			AdvanceChar();
		}

		if (CurrentChar() < '0' || CurrentChar() > '9')
		{
			throw CreateError("Invalid exponent format");
		}

		while (CurrentChar() >= '0' && CurrentChar() <= '9')
		{
			AdvanceChar();
		}
	}

	// 提取数字字符串
	CString NumberStr = Json.SubString(StartPos, Position - StartPos);

	try
	{
		if (bHasDecimal || bHasExponent)
		{
			double Value = std::stod(NumberStr.GetData());
			return CConfigValue(Value);
		}
		else
		{
			// 尝试解析为int32，如果溢出则使用int64
			try
			{
				long long Value = std::stoll(NumberStr.GetData());
				if (Value >= INT32_MIN && Value <= INT32_MAX)
				{
					return CConfigValue(static_cast<int32_t>(Value));
				}
				else
				{
					return CConfigValue(static_cast<int64_t>(Value));
				}
			}
			catch (...)
			{
				// 如果解析失败，尝试作为double
				double Value = std::stod(NumberStr.GetData());
				return CConfigValue(Value);
			}
		}
	}
	catch (...)
	{
		throw CreateError("Invalid number value: " + NumberStr);
	}
}

CConfigValue CJsonParser::CJsonParserImpl::ParseString()
{
	CString Str = ParseStringLiteral();
	return CConfigValue(UnescapeString(Str));
}

CConfigValue CJsonParser::CJsonParserImpl::ParseArray()
{
	if (!ExpectChar('['))
	{
		throw CreateError("Expected '['");
	}

	Depth++;

	CConfigArray Array;
	SkipWhitespace();

	// 空数组
	if (CurrentChar() == ']')
	{
		AdvanceChar();
		Depth--;
		return CConfigValue(Array);
	}

	while (true)
	{
		Array.PushBack(ParseValue());

		SkipWhitespace();

		if (CurrentChar() == ']')
		{
			AdvanceChar();
			break;
		}
		else if (CurrentChar() == ',')
		{
			AdvanceChar();
			SkipWhitespace();

			// 允许尾随逗号
			if (Options.bAllowTrailingCommas && CurrentChar() == ']')
			{
				AdvanceChar();
				break;
			}
		}
		else
		{
			throw CreateError("Expected ',' or ']' in array");
		}
	}

	Depth--;
	return CConfigValue(Array);
}

CConfigValue CJsonParser::CJsonParserImpl::ParseObject()
{
	if (!ExpectChar('{'))
	{
		throw CreateError("Expected '{'");
	}

	Depth++;

	CConfigObject Object;
	SkipWhitespace();

	// 空对象
	if (CurrentChar() == '}')
	{
		AdvanceChar();
		Depth--;
		return CConfigValue(Object);
	}

	while (true)
	{
		SkipWhitespace();

		// 解析键
		CString Key;
		if (CurrentChar() == '"')
		{
			Key = UnescapeString(ParseStringLiteral());
		}
		else if (Options.bAllowUnquotedKeys && (std::isalpha(CurrentChar()) || CurrentChar() == '_'))
		{
			// 不带引号的键
			int32_t StartPos = Position;
			while (std::isalnum(CurrentChar()) || CurrentChar() == '_')
			{
				AdvanceChar();
			}
			Key = Json.SubString(StartPos, Position - StartPos);
		}
		else
		{
			throw CreateError("Expected object key");
		}

		SkipWhitespace();

		if (!ExpectChar(':'))
		{
			throw CreateError("Expected ':' after object key");
		}

		SkipWhitespace();

		// 解析值
		CConfigValue Value = ParseValue();
		Object.Insert(Key, Value);

		SkipWhitespace();

		if (CurrentChar() == '}')
		{
			AdvanceChar();
			break;
		}
		else if (CurrentChar() == ',')
		{
			AdvanceChar();
			SkipWhitespace();

			// 允许尾随逗号
			if (Options.bAllowTrailingCommas && CurrentChar() == '}')
			{
				AdvanceChar();
				break;
			}
		}
		else
		{
			throw CreateError("Expected ',' or '}' in object");
		}
	}

	Depth--;
	return CConfigValue(Object);
}

CString CJsonParser::CJsonParserImpl::ParseStringLiteral()
{
	if (!ExpectChar('"'))
	{
		throw CreateError("Expected '\"'");
	}

	CString Result;

	while (!IsAtEnd() && CurrentChar() != '"')
	{
		char Ch = CurrentChar();

		if (Ch == '\\')
		{
			AdvanceChar();
			if (IsAtEnd())
			{
				throw CreateError("Unterminated string literal");
			}

			char EscapeChar = CurrentChar();
			switch (EscapeChar)
			{
			case '"':
				Result += '"';
				break;
			case '\\':
				Result += '\\';
				break;
			case '/':
				Result += '/';
				break;
			case 'b':
				Result += '\b';
				break;
			case 'f':
				Result += '\f';
				break;
			case 'n':
				Result += '\n';
				break;
			case 'r':
				Result += '\r';
				break;
			case 't':
				Result += '\t';
				break;
			case 'u': {
				// Unicode转义
				AdvanceChar();
				CString HexStr;
				for (int i = 0; i < 4; ++i)
				{
					if (IsAtEnd() || !std::isxdigit(CurrentChar()))
					{
						throw CreateError("Invalid unicode escape sequence");
					}
					HexStr += CurrentChar();
					AdvanceChar();
				}

				try
				{
					int CodePoint = std::stoi(HexStr.GetData(), nullptr, 16);
					// 简化实现：只处理ASCII范围的字符
					if (CodePoint < 128)
					{
						Result += static_cast<char>(CodePoint);
					}
					else
					{
						Result += '?'; // 非ASCII字符用?代替
					}
				}
				catch (...)
				{
					throw CreateError("Invalid unicode escape sequence");
				}
				continue; // 不要再次前进
			}
			default:
				throw CreateError(CString("Invalid escape sequence: \\") + CString(&EscapeChar, 1));
			}
			AdvanceChar();
		}
		else if (Ch < 32)
		{
			throw CreateError("Control character in string literal");
		}
		else
		{
			Result += Ch;
			AdvanceChar();
		}
	}

	if (!ExpectChar('"'))
	{
		throw CreateError("Unterminated string literal");
	}

	return Result;
}

CString CJsonParser::CJsonParserImpl::UnescapeString(const CString& Str)
{
	// 已经在ParseStringLiteral中处理了转义，这里直接返回
	return Str;
}

bool CJsonParser::CJsonParserImpl::ExpectChar(char Expected)
{
	if (CurrentChar() == Expected)
	{
		AdvanceChar();
		return true;
	}
	return false;
}

bool CJsonParser::CJsonParserImpl::MatchKeyword(const CString& Keyword)
{
	if (Position + static_cast<int32_t>(Keyword.Size()) > static_cast<int32_t>(Json.Size()))
	{
		return false;
	}

	for (int32_t i = 0; i < static_cast<int32_t>(Keyword.Size()); ++i)
	{
		if (Json[Position + i] != Keyword[i])
		{
			return false;
		}
	}

	// 确保关键字后面没有标识符字符
	if (Position + static_cast<int32_t>(Keyword.Size()) < static_cast<int32_t>(Json.Size()))
	{
		char NextChar = Json[Position + static_cast<int32_t>(Keyword.Size())];
		if (std::isalnum(NextChar) || NextChar == '_')
		{
			return false;
		}
	}

	// 前进到关键字后面
	for (int32_t i = 0; i < static_cast<int32_t>(Keyword.Size()); ++i)
	{
		AdvanceChar();
	}

	return true;
}

// === CJsonParser 静态方法实现 ===

SJsonParseResult CJsonParser::Parse(const CString& JsonString, const SParseOptions& /*Options*/)
{
	NLOG_CONFIG(Debug, "Parsing JSON string of length {}", JsonString.Size());

	try
	{
		// 使用nlohmann::json直接解析
		std::string StdJsonString(JsonString.GetData(), JsonString.Size());
		nlohmann::json ParsedJson = nlohmann::json::parse(StdJsonString);
		
		// 创建 CConfigValue 对象
		CConfigValue Result(ParsedJson);
		
		NLOG_CONFIG(Debug, "JSON parsing successful");
		return SJsonParseResult(Result);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		NLOG_CONFIG(Error, "JSON parsing failed: {}", e.what());
		return SJsonParseResult(SJsonParseError(CString(e.what())));
	}
	catch (const std::exception& e)
	{
		NLOG_CONFIG(Error, "JSON parsing failed: {}", e.what());
		return SJsonParseResult(SJsonParseError(CString(e.what())));
	}
}

SJsonParseResult CJsonParser::ParseFile(const CString& FilePath, const SParseOptions& Options)
{
	NLOG_CONFIG(Debug, "Parsing JSON file: {}", FilePath.GetData());

	std::ifstream File(FilePath.GetData());
	if (!File.is_open())
	{
		NLOG_CONFIG(Error, "Failed to open JSON file: {}", FilePath.GetData());
		return SJsonParseResult(SJsonParseError("Failed to open file: " + FilePath));
	}

	// 读取文件内容
	CString JsonContent;
	std::string Line;
	while (std::getline(File, Line))
	{
		JsonContent += CString(Line.c_str());
		JsonContent += "\n";
	}

	File.close();

	if (JsonContent.IsEmpty())
	{
		NLOG_CONFIG(Warning, "JSON file is empty: {}", FilePath.GetData());
		return SJsonParseResult(SJsonParseError("File is empty: " + FilePath));
	}

	return Parse(JsonContent, Options);
}

// === CJsonGeneratorImpl 实现 ===

CJsonGenerator::CJsonGeneratorImpl::CJsonGeneratorImpl(const SJsonGenerateOptions& InOptions)
    : Options(InOptions)
{}

CString CJsonGenerator::CJsonGeneratorImpl::Generate(const CConfigValue& Value)
{
	return GenerateValue(Value, 0);
}

CString CJsonGenerator::CJsonGeneratorImpl::GenerateValue(const CConfigValue& Value, int32_t Indent)
{
	switch (Value.GetType())
	{
	case EConfigValueType::Null:
		return CString("null");

	case EConfigValueType::Bool:
		return Value.AsBool() ? CString("true") : CString("false");

	case EConfigValueType::Int32:
		return CString::FromInt(Value.AsInt32());

	case EConfigValueType::Int64:
		return CString::FromInt64(Value.AsInt64());

	case EConfigValueType::Float:
		return CString::FromFloat(Value.AsFloat());

	case EConfigValueType::Double:
		return CString::FromDouble(Value.AsDouble());

	case EConfigValueType::String:
		return GenerateString(Value.AsString());

	case EConfigValueType::Array:
		return GenerateArray(Value.AsArray(), Indent);

	case EConfigValueType::Object:
		return GenerateObject(Value.AsObject(), Indent);

	default:
		return CString("null");
	}
}

CString CJsonGenerator::CJsonGeneratorImpl::GenerateString(const CString& Str)
{
	return CString("\"") + EscapeString(Str) + CString("\"");
}

CString CJsonGenerator::CJsonGeneratorImpl::GenerateArray(const CConfigArray& Array, int32_t Indent)
{
	if (Array.IsEmpty())
	{
		return CString("[]");
	}

	CString Result = CString("[");

	if (Options.bPrettyPrint)
	{
		Result += "\n";

		for (size_t i = 0; i < Array.Size(); ++i)
		{
			Result += GetIndent(Indent + 1);
			Result += GenerateValue(Array[i], Indent + 1);

			if (i < Array.Size() - 1)
			{
				Result += ",";
			}

			Result += "\n";
		}

		Result += GetIndent(Indent);
	}
	else
	{
		for (size_t i = 0; i < Array.Size(); ++i)
		{
			Result += GenerateValue(Array[i], Indent + 1);

			if (i < Array.Size() - 1)
			{
				Result += ",";
			}
		}
	}

	Result += "]";
	return Result;
}

CString CJsonGenerator::CJsonGeneratorImpl::GenerateObject(const CConfigObject& Object, int32_t Indent)
{
	if (Object.IsEmpty())
	{
		return CString("{}");
	}

	CString Result = CString("{");

	// 获取所有键并可选地排序
	TArray<CString, CMemoryManager> Keys;
	Keys.Reserve(Object.Size());

	for (const auto& Pair : Object)
	{
		Keys.PushBack(Pair.first);
	}

	if (Options.bSortKeys)
	{
		// 简单的字符串排序
		for (int32_t i = 0; i < Keys.Size() - 1; ++i)
		{
			for (int32_t j = i + 1; j < Keys.Size(); ++j)
			{
				if (Keys[i] > Keys[j])
				{
					CString Temp = Keys[i];
					Keys[i] = Keys[j];
					Keys[j] = Temp;
				}
			}
		}
	}

	if (Options.bPrettyPrint)
	{
		Result += "\n";

		for (int32_t i = 0; i < Keys.Size(); ++i)
		{
			const CString& Key = Keys[i];
			const CConfigValue* Value = Object.Find(Key);

			Result += GetIndent(Indent + 1);
			Result += GenerateString(Key);
			Result += ": ";
			Result += GenerateValue(*Value, Indent + 1);

			if (i < Keys.Size() - 1)
			{
				Result += ",";
			}

			Result += "\n";
		}

		Result += GetIndent(Indent);
	}
	else
	{
		for (int32_t i = 0; i < Keys.Size(); ++i)
		{
			const CString& Key = Keys[i];
			const CConfigValue* Value = Object.Find(Key);

			Result += GenerateString(Key);
			Result += ":";
			Result += GenerateValue(*Value, Indent + 1);

			if (i < Keys.Size() - 1)
			{
				Result += ",";
			}
		}
	}

	Result += "}";
	return Result;
}

CString CJsonGenerator::CJsonGeneratorImpl::GetIndent(int32_t Level) const
{
	if (!Options.bPrettyPrint || Level <= 0)
	{
		return CString();
	}

	CString Result;
	for (int32_t i = 0; i < Level * Options.IndentSize; ++i)
	{
		Result += " ";
	}
	return Result;
}

CString CJsonGenerator::CJsonGeneratorImpl::EscapeString(const CString& Str) const
{
	CString Result;
	Result.Reserve(Str.Size() * 2);

	for (int32_t i = 0; i < Str.Size(); ++i)
	{
		char Ch = Str[i];
		switch (Ch)
		{
		case '"':
			Result += "\\\"";
			break;
		case '\\':
			Result += "\\\\";
			break;
		case '\b':
			Result += "\\b";
			break;
		case '\f':
			Result += "\\f";
			break;
		case '\n':
			Result += "\\n";
			break;
		case '\r':
			Result += "\\r";
			break;
		case '\t':
			Result += "\\t";
			break;
		default:
			if (Ch < 32 || (Options.bEscapeUnicode && Ch > 126))
			{
				char Buffer[8];
				snprintf(Buffer, sizeof(Buffer), "\\u%04x", static_cast<unsigned char>(Ch));
				Result += Buffer;
			}
			else
			{
				Result += Ch;
			}
			break;
		}
	}

	return Result;
}

// === CJsonGenerator 静态方法实现 ===

CString CJsonGenerator::Generate(const CConfigValue& Value, const SJsonGenerateOptions& Options)
{
	NLOG_CONFIG(Debug, "Generating JSON string");

	try
	{
		// 直接使用nlohmann::json生成
		std::string JsonString;
		if (Options.bPrettyPrint)
		{
			JsonString = Value.GetInternalJson().dump(Options.IndentSize);
		}
		else
		{
			JsonString = Value.GetInternalJson().dump();
		}
		
		CString Result(JsonString.c_str());
		NLOG_CONFIG(Debug, "JSON generation completed, length: {}", Result.Size());
		return Result;
	}
	catch (const std::exception& e)
	{
		NLOG_CONFIG(Error, "JSON generation failed: {}", e.what());
		return CString();
	}
}

bool CJsonGenerator::WriteToFile(const CConfigValue& Value,
                                 const CString& FilePath,
                                 const SJsonGenerateOptions& Options)
{
	NLOG_CONFIG(Debug, "Writing JSON to file: {}", FilePath.GetData());

	CString JsonString = Generate(Value, Options);

	std::ofstream File(FilePath.GetData());
	if (!File.is_open())
	{
		NLOG_CONFIG(Error, "Failed to open file for writing: {}", FilePath.GetData());
		return false;
	}

	File << JsonString.GetData();
	File.close();

	if (File.fail())
	{
		NLOG_CONFIG(Error, "Failed to write JSON to file: {}", FilePath.GetData());
		return false;
	}

	NLOG_CONFIG(Debug, "JSON successfully written to file: {}", FilePath.GetData());
	return true;
}

} // namespace NLib