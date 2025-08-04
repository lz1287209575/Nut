#include "ConfigValue.h"

#include "Logging/LogCategory.h"

#include <sstream>

namespace NLib
{
// === 路径解析辅助函数 ===

/**
 * @brief 解析路径分段
 */
struct SPathSegment
{
	TString Key;
	bool bIsArrayIndex = false;
	size_t ArrayIndex = 0;

	SPathSegment() = default;
	SPathSegment(const TString& InKey)
	    : Key(InKey)
	{}
	SPathSegment(size_t InIndex)
	    : bIsArrayIndex(true),
	      ArrayIndex(InIndex)
	{}
};

/**
 * @brief 解析路径字符串
 */
static TArray<SPathSegment, CMemoryManager> ParsePath(const TString& Path)
{
	TArray<SPathSegment, CMemoryManager> Segments;

	if (Path.IsEmpty())
	{
		return Segments;
	}

	const char* Data = Path.GetData();
	const char* Current = Data;
	const char* End = Data + Path.Length();

	while (Current < End)
	{
		// 跳过分隔符
		while (Current < End && (*Current == '.' || *Current == '/'))
		{
			Current++;
		}

		if (Current >= End)
		{
			break;
		}

		// 查找下一个分隔符或数组索引
		const char* SegmentStart = Current;
		const char* SegmentEnd = Current;

		while (SegmentEnd < End && *SegmentEnd != '.' && *SegmentEnd != '/' && *SegmentEnd != '[')
		{
			SegmentEnd++;
		}

		// 提取键名
		if (SegmentEnd > SegmentStart)
		{
			TString Key(SegmentStart, static_cast<int32_t>(SegmentEnd - SegmentStart));
			Segments.Add(SPathSegment(Key));
		}

		Current = SegmentEnd;

		// 处理数组索引
		if (Current < End && *Current == '[')
		{
			Current++; // 跳过 '['

			const char* IndexStart = Current;
			while (Current < End && *Current != ']')
			{
				Current++;
			}

			if (Current < End && *Current == ']')
			{
				TString IndexStr(IndexStart, static_cast<int32_t>(Current - IndexStart));
				try
				{
					size_t Index = static_cast<size_t>(std::stoull(IndexStr.GetData()));
					Segments.Add(SPathSegment(Index));
				}
				catch (...)
				{
					NLOG_CONFIG(Error, "Invalid array index in path: {}", IndexStr.GetData());
				}
				Current++; // 跳过 ']'
			}
		}
	}

	return Segments;
}

// === CConfigValue 路径访问实现 ===

const CConfigValue& CConfigValue::GetByPath(const TString& Path) const
{
	static CConfigValue NullValue;

	if (Path.IsEmpty())
	{
		return *this;
	}

	auto Segments = ParsePath(Path);
	const CConfigValue* Current = this;

	for (const auto& Segment : Segments)
	{
		if (Segment.bIsArrayIndex)
		{
			if (!Current->IsArray() || Segment.ArrayIndex >= Current->Size())
			{
				return NullValue;
			}
			Current = &(*Current)[Segment.ArrayIndex];
		}
		else
		{
			if (!Current->IsObject() || !Current->HasKey(Segment.Key))
			{
				return NullValue;
			}
			Current = &(*Current)[Segment.Key];
		}
	}

	return *Current;
}

void CConfigValue::SetByPath(const TString& Path, const CConfigValue& Val)
{
	if (Path.IsEmpty())
	{
		*this = Val;
		return;
	}

	auto Segments = ParsePath(Path);
	if (Segments.IsEmpty())
	{
		return;
	}

	CConfigValue* Current = this;

	for (int32_t i = 0; i < Segments.Size() - 1; ++i)
	{
		const auto& Segment = Segments[i];
		const auto& NextSegment = Segments[i + 1];

		if (Segment.bIsArrayIndex)
		{
			if (!Current->IsArray())
			{
				*Current = CConfigArray();
			}

			// 确保数组有足够的元素
			auto& Array = Current->AsArray();
			while (Array.Size() <= Segment.ArrayIndex)
			{
				Array.Add(CConfigValue());
			}

			Current = &Array[Segment.ArrayIndex];
		}
		else
		{
			if (!Current->IsObject())
			{
				*Current = CConfigObject();
			}

			auto& Object = Current->AsObject();
			if (!Object.Contains(Segment.Key))
			{
				// 根据下一个分段决定创建数组还是对象
				if (NextSegment.bIsArrayIndex)
				{
					Object.Add(Segment.Key, CConfigArray());
				}
				else
				{
					Object.Add(Segment.Key, CConfigObject());
				}
			}

			Current = &Object[Segment.Key];
		}
	}

	// 设置最后一个分段的值
	const auto& LastSegment = Segments.Last();
	if (LastSegment.bIsArrayIndex)
	{
		if (!Current->IsArray())
		{
			*Current = CConfigArray();
		}

		auto& Array = Current->AsArray();
		while (Array.Size() <= LastSegment.ArrayIndex)
		{
			Array.Add(CConfigValue());
		}

		Array[LastSegment.ArrayIndex] = Val;
	}
	else
	{
		if (!Current->IsObject())
		{
			*Current = CConfigObject();
		}

		Current->AsObject().Add(LastSegment.Key, Val);
	}
}

bool CConfigValue::HasPath(const TString& Path) const
{
	if (Path.IsEmpty())
	{
		return true;
	}

	auto Segments = ParsePath(Path);
	const CConfigValue* Current = this;

	for (const auto& Segment : Segments)
	{
		if (Segment.bIsArrayIndex)
		{
			if (!Current->IsArray() || Segment.ArrayIndex >= Current->Size())
			{
				return false;
			}
			Current = &(*Current)[Segment.ArrayIndex];
		}
		else
		{
			if (!Current->IsObject() || !Current->HasKey(Segment.Key))
			{
				return false;
			}
			Current = &(*Current)[Segment.Key];
		}
	}

	return true;
}

// === JSON 序列化实现 ===

static TString EscapeJsonString(const TString& Str)
{
	TString Result;
	Result.Reserve(Str.Length() * 2); // 预分配，避免频繁重分配

	for (int32_t i = 0; i < Str.Length(); ++i)
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
			if (Ch < 32 || Ch > 126)
			{
				// Unicode 转义
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

static TString GetIndentString(int32_t Indent)
{
	if (Indent <= 0)
	{
		return TString();
	}

	TString Result;
	for (int32_t i = 0; i < Indent; ++i)
	{
		Result += "  ";
	}
	return Result;
}

TString CConfigValue::ToJsonString(bool Pretty, int32_t Indent) const
{
	switch (GetType())
	{
	case EConfigValueType::Null:
		return TString("null");

	case EConfigValueType::Bool:
		return AsBool() ? TString("true") : TString("false");

	case EConfigValueType::Int32:
		return TString::FromInt(AsInt32());

	case EConfigValueType::Int64:
		return TString::FromInt64(AsInt64());

	case EConfigValueType::Float:
		return TString::FromFloat(AsFloat());

	case EConfigValueType::Double:
		return TString::FromDouble(AsDouble());

	case EConfigValueType::String:
		return TString("\"") + EscapeJsonString(AsString()) + TString("\"");

	case EConfigValueType::Array: {
		const auto& Array = AsArray();
		if (Array.IsEmpty())
		{
			return TString("[]");
		}

		TString Result = TString("[");

		if (Pretty)
		{
			Result += "\n";
			for (int32_t i = 0; i < Array.Size(); ++i)
			{
				Result += GetIndentString(Indent + 1);
				Result += Array[i].ToJsonString(Pretty, Indent + 1);
				if (i < Array.Size() - 1)
				{
					Result += ",";
				}
				Result += "\n";
			}
			Result += GetIndentString(Indent);
		}
		else
		{
			for (int32_t i = 0; i < Array.Size(); ++i)
			{
				Result += Array[i].ToJsonString(Pretty, Indent + 1);
				if (i < Array.Size() - 1)
				{
					Result += ",";
				}
			}
		}

		Result += "]";
		return Result;
	}

	case EConfigValueType::Object: {
		const auto& Object = AsObject();
		if (Object.IsEmpty())
		{
			return TString("{}");
		}

		TString Result = TString("{");

		if (Pretty)
		{
			Result += "\n";
			int32_t Count = 0;
			for (const auto& Pair : Object)
			{
				Result += GetIndentString(Indent + 1);
				Result += "\"" + EscapeJsonString(Pair.Key) + "\": ";
				Result += Pair.Value.ToJsonString(Pretty, Indent + 1);
				if (Count < Object.Size() - 1)
				{
					Result += ",";
				}
				Result += "\n";
				Count++;
			}
			Result += GetIndentString(Indent);
		}
		else
		{
			int32_t Count = 0;
			for (const auto& Pair : Object)
			{
				Result += "\"" + EscapeJsonString(Pair.Key) + "\":";
				Result += Pair.Value.ToJsonString(Pretty, Indent + 1);
				if (Count < Object.Size() - 1)
				{
					Result += ",";
				}
				Count++;
			}
		}

		Result += "}";
		return Result;
	}

	default:
		return TString("null");
	}
}

} // namespace NLib