// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonBufferView
{
	FString Name;

	FGLTFJsonBufferIndex Buffer;

	int64 ByteLength;
	int64 ByteOffset;
	int32 ByteStride;

	ESKGLTFJsonBufferTarget Target;

	FGLTFJsonBufferView()
		: ByteLength(0)
		, ByteOffset(0)
		, ByteStride(0)
		, Target(ESKGLTFJsonBufferTarget::None)
	{
		// check that view fits completely inside the buffer
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		JsonWriter.WriteValue(TEXT("buffer"), Buffer);
		JsonWriter.WriteValue(TEXT("byteLength"), ByteLength);

		if (ByteOffset != 0)
		{
			JsonWriter.WriteValue(TEXT("byteOffset"), ByteOffset);
		}

		if (ByteStride != 0)
		{
			JsonWriter.WriteValue(TEXT("byteStride"), ByteStride);
		}

		if (Target != ESKGLTFJsonBufferTarget::None)
		{
			JsonWriter.WriteValue(TEXT("target"), FGLTFJsonUtility::ToInteger(Target));
		}

		JsonWriter.WriteObjectEnd();
	}
};
