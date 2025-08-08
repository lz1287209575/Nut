#include "JsonSerializer.h"

#include "Logging/LogCategory.h"

namespace NLib
{
// === CJsonSerializationArchive 实现 ===

CJsonSerializationArchive::CJsonSerializationArchive(TSharedPtr<NStream> InStream,
                                                     const SSerializationContext& InContext)
    : CSerializationArchive(InStream, InContext)
{}

SSerializationResult CJsonSerializationArchive::Initialize()
{
	if (bInitialized)
	{
		return SSerializationResult(true);
	}

	if (IsDeserializing())
	{
		auto Result = ReadJsonFromStream();
		if (!Result.bSuccess)
		{
			return Result;
		}
		bJsonLoaded = true;
	}
	else
	{
		// 序列化时，从空对象开始
		RootValue = CConfigObject();
	}

	// 初始化导航栈
	NavigationStack.Clear();
	NavigationStack.Add(SNavigationFrame(&RootValue));

	bInitialized = true;
	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::Finalize()
{
	if (!bInitialized)
	{
		return SSerializationResult(false, "Archive not initialized");
	}

	if (IsSerializing())
	{
		return WriteJsonToStream();
	}

	return SSerializationResult(true);
}

// === 基础类型序列化实现 ===

SSerializationResult CJsonSerializationArchive::Serialize(bool& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsBool();
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(int8_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int32_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<int8_t>(CurrentValue->AsInt32());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(uint8_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int32_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<uint8_t>(CurrentValue->AsInt32());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(int16_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int32_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<int16_t>(CurrentValue->AsInt32());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(uint16_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int32_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<uint16_t>(CurrentValue->AsInt32());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(int32_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsInt32();
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(uint32_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int64_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<uint32_t>(CurrentValue->AsInt64());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(int64_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsInt64();
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(uint64_t& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(static_cast<int64_t>(Value)));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = static_cast<uint64_t>(CurrentValue->AsInt64());
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(float& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsFloat();
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(double& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsDouble();
		return SSerializationResult(true);
	}
}

SSerializationResult CJsonSerializationArchive::Serialize(CString& Value)
{
	if (IsSerializing())
	{
		return SetCurrentValue(CConfigValue(Value));
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || CurrentValue->IsNull())
		{
			return SSerializationResult(false, "Value is null");
		}

		Value = CurrentValue->AsString();
		return SSerializationResult(true);
	}
}

// === JSON特有的序列化方法 ===

SSerializationResult CJsonSerializationArchive::SerializeNull()
{
	return SetCurrentValue(CConfigValue());
}

SSerializationResult CJsonSerializationArchive::BeginArray(int32_t Size)
{
	if (IsSerializing())
	{
		auto Result = SetCurrentValue(CConfigArray());
		if (!Result.bSuccess)
		{
			return Result;
		}

		CurrentArrayIndex = 0;
		CurrentArraySize = Size;
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || !CurrentValue->IsArray())
		{
			return SSerializationResult(false, "Expected array value");
		}

		CurrentArrayIndex = 0;
		CurrentArraySize = CurrentValue->Size();
	}

	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::EndArray()
{
	CurrentArrayIndex = 0;
	CurrentArraySize = 0;
	return SSerializationResult(true);
}

// === 配置值转换 ===

SSerializationResult CJsonSerializationArchive::SerializeFromConfigValue(const CConfigValue& Value)
{
	return SetCurrentValue(Value);
}

SSerializationResult CJsonSerializationArchive::SerializeToConfigValue(CConfigValue& Value)
{
	CConfigValue* CurrentValue = GetCurrentValue();
	if (!CurrentValue)
	{
		return SSerializationResult(false, "No current value");
	}

	Value = *CurrentValue;
	return SSerializationResult(true);
}

// === CSerializationArchive 虚函数实现 ===

SSerializationResult CJsonSerializationArchive::BeginObject(const CString& TypeName)
{
	if (IsSerializing())
	{
		auto Result = EnsureCurrentIsObject();
		if (!Result.bSuccess)
		{
			return Result;
		}

		// 可选地添加类型信息
		if (Context.HasFlag(ESerializationFlags::IncludeTypeInfo) && !TypeName.IsEmpty())
		{
			CConfigValue* CurrentValue = GetCurrentValue();
			if (CurrentValue && CurrentValue->IsObject())
			{
				CurrentValue->AsObject().Add("__type", CConfigValue(TypeName));
			}
		}
	}
	else
	{
		CConfigValue* CurrentValue = GetCurrentValue();
		if (!CurrentValue || !CurrentValue->IsObject())
		{
			return SSerializationResult(false, "Expected object value");
		}

		// 验证类型信息
		if (Context.HasFlag(ESerializationFlags::ValidateOnRead) && !TypeName.IsEmpty())
		{
			const auto& Object = CurrentValue->AsObject();
			if (Object.Contains("__type"))
			{
				CString ActualType = Object["__type"].AsString();
				if (ActualType != TypeName)
				{
					return SSerializationResult(
					    false, CString("Type mismatch: expected ") + TypeName + CString(", got ") + ActualType);
				}
			}
		}
	}

	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::EndObject(const CString& TypeName)
{
	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::BeginField(const CString& FieldName)
{
	return NavigateToField(FieldName);
}

SSerializationResult CJsonSerializationArchive::EndField(const CString& FieldName)
{
	return NavigateFromField(FieldName);
}

// === 内部实现 ===

SSerializationResult CJsonSerializationArchive::ReadJsonFromStream()
{
	if (!Stream || !Stream->CanRead())
	{
		return SSerializationResult(false, "Stream cannot be read");
	}

	// 读取整个流到字符串
	CString JsonString;
	auto AllData = Stream->ReadAll();
	if (!AllData.IsEmpty())
	{
		JsonString = CString(reinterpret_cast<const char*>(AllData.GetData()), AllData.Size());
	}

	if (JsonString.IsEmpty())
	{
		return SSerializationResult(false, "Empty JSON stream");
	}

	// 解析JSON
	auto ParseResult = CJsonParser::Parse(JsonString);
	if (!ParseResult.bSuccess)
	{
		return SSerializationResult(false, CString("JSON parse error: ") + ParseResult.Error.ToString());
	}

	RootValue = ParseResult.Value;
	NLOG_SERIALIZATION(Debug, "JSON loaded from stream");
	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::WriteJsonToStream()
{
	if (!Stream || !Stream->CanWrite())
	{
		return SSerializationResult(false, "Stream cannot be written");
	}

	// 生成JSON字符串
	bool bPrettyPrint = Context.HasFlag(ESerializationFlags::PrettyPrint);
	CString JsonString = RootValue.ToJsonString(bPrettyPrint);

	if (JsonString.IsEmpty())
	{
		return SSerializationResult(false, "Failed to generate JSON");
	}

	// 写入到流
	auto WriteResult = Stream->Write(reinterpret_cast<const uint8_t*>(JsonString.GetData()), JsonString.Length());

	if (!WriteResult.bSuccess)
	{
		return SSerializationResult(false, "Failed to write JSON to stream");
	}

	Stream->Flush();
	NLOG_SERIALIZATION(Debug, "JSON written to stream ({} bytes)", WriteResult.BytesProcessed);
	return SSerializationResult(true, WriteResult.BytesProcessed);
}

CConfigValue* CJsonSerializationArchive::GetCurrentValue()
{
	if (NavigationStack.IsEmpty())
	{
		return nullptr;
	}

	return NavigationStack.Last().Value;
}

SSerializationResult CJsonSerializationArchive::SetCurrentValue(const CConfigValue& Value)
{
	if (NavigationStack.IsEmpty())
	{
		return SSerializationResult(false, "Navigation stack is empty");
	}

	auto& Frame = NavigationStack.Last();
	if (!Frame.Value)
	{
		return SSerializationResult(false, "Current frame has null value");
	}

	*Frame.Value = Value;
	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::NavigateToField(const CString& FieldName)
{
	CConfigValue* CurrentValue = GetCurrentValue();
	if (!CurrentValue)
	{
		return SSerializationResult(false, "No current value for field navigation");
	}

	if (IsSerializing())
	{
		// 确保当前值是对象
		if (!CurrentValue->IsObject())
		{
			*CurrentValue = CConfigObject();
		}

		auto& Object = CurrentValue->AsObject();
		if (!Object.Contains(FieldName))
		{
			Object.Add(FieldName, CConfigValue());
		}

		NavigationStack.Add(SNavigationFrame(&Object[FieldName], FieldName));
	}
	else
	{
		if (!CurrentValue->IsObject())
		{
			return SSerializationResult(false, CString("Expected object for field '") + FieldName + CString("'"));
		}

		const auto& Object = CurrentValue->AsObject();
		if (!Object.Contains(FieldName))
		{
			if (Context.HasFlag(ESerializationFlags::AllowPartialRead))
			{
				// 创建一个空值用于部分读取
				static CConfigValue NullValue;
				NavigationStack.Add(SNavigationFrame(&NullValue, FieldName));
			}
			else
			{
				return SSerializationResult(false, CString("Field '") + FieldName + CString("' not found"));
			}
		}
		else
		{
			NavigationStack.Add(SNavigationFrame(const_cast<CConfigValue*>(&Object[FieldName]), FieldName));
		}
	}

	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::NavigateFromField(const CString& FieldName)
{
	if (NavigationStack.IsEmpty())
	{
		return SSerializationResult(false, "Navigation stack is empty");
	}

	auto& TopFrame = NavigationStack.Last();
	if (TopFrame.Key != FieldName)
	{
		return SSerializationResult(false,
		                            CString("Field name mismatch: expected '") + FieldName + CString("', got '") +
		                                TopFrame.Key + CString("'"));
	}

	NavigationStack.RemoveLast();
	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::EnsureCurrentIsObject()
{
	CConfigValue* CurrentValue = GetCurrentValue();
	if (!CurrentValue)
	{
		return SSerializationResult(false, "No current value");
	}

	if (!CurrentValue->IsObject())
	{
		*CurrentValue = CConfigObject();
	}

	return SSerializationResult(true);
}

SSerializationResult CJsonSerializationArchive::EnsureCurrentIsArray()
{
	CConfigValue* CurrentValue = GetCurrentValue();
	if (!CurrentValue)
	{
		return SSerializationResult(false, "No current value");
	}

	if (!CurrentValue->IsArray())
	{
		*CurrentValue = CConfigArray();
	}

	return SSerializationResult(true);
}

// === CJsonSerializationHelper 实现 ===

CString CJsonSerializationHelper::ConfigValueToJson(const CConfigValue& Value, bool bPrettyPrint)
{
	return Value.ToJsonString(bPrettyPrint);
}

CConfigValue CJsonSerializationHelper::JsonToConfigValue(const CString& JsonString)
{
	auto ParseResult = CJsonParser::Parse(JsonString);
	if (ParseResult.bSuccess)
	{
		return ParseResult.Value;
	}
	else
	{
		NLOG_SERIALIZATION(Error, "Failed to parse JSON: {}", ParseResult.Error.ToString().GetData());
		return CConfigValue();
	}
}

} // namespace NLib