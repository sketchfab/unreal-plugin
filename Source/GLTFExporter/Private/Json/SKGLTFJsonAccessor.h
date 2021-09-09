// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonAccessor
{
	FString Name;

	FGLTFJsonBufferViewIndex BufferView;
	int64                    ByteOffset;
	int32                    Count;
	ESKGLTFJsonAccessorType    Type;
	ESKGLTFJsonComponentType   ComponentType;
	bool                     bNormalized;

	int32 MinMaxLength;
	float Min[16];
	float Max[16];

	FGLTFJsonAccessor()
		: ByteOffset(0)
		, Count(0)
		, Type(ESKGLTFJsonAccessorType::None)
		, ComponentType(ESKGLTFJsonComponentType::None)
		, bNormalized(false)
		, MinMaxLength(0)
		, Min{0}
		, Max{0}
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		JsonWriter.WriteValue(TEXT("bufferView"), BufferView);

		if (ByteOffset != 0)
		{
			JsonWriter.WriteValue(TEXT("byteOffset"), ByteOffset);
		}

		JsonWriter.WriteValue(TEXT("count"), Count);
		JsonWriter.WriteValue(TEXT("type"), FGLTFJsonUtility::ToString(Type));
		JsonWriter.WriteValue(TEXT("componentType"), FGLTFJsonUtility::ToInteger(ComponentType));

		if (bNormalized)
		{
			JsonWriter.WriteValue(TEXT("normalized"), bNormalized);
		}

		if (MinMaxLength > 0)
		{
			JsonWriter.WriteArrayStart(TEXT("min"));
			for (int32 ComponentIndex = 0; ComponentIndex < MinMaxLength; ++ComponentIndex)
			{
				FGLTFJsonUtility::WriteExactValue(JsonWriter, Min[ComponentIndex]);
			}
			JsonWriter.WriteArrayEnd();

			JsonWriter.WriteArrayStart(TEXT("max"));
			for (int32 ComponentIndex = 0; ComponentIndex < MinMaxLength; ++ComponentIndex)
			{
				FGLTFJsonUtility::WriteExactValue(JsonWriter, Max[ComponentIndex]);
			}
			JsonWriter.WriteArrayEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
