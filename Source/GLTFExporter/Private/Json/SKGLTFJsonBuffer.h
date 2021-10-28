// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonBuffer
{
	FString Name;

	FString URI;
	int64   ByteLength;

	FGLTFJsonBuffer()
		: ByteLength(0)
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

		if (!URI.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("uri"), URI);
		}

		JsonWriter.WriteValue(TEXT("byteLength"), ByteLength);

		JsonWriter.WriteObjectEnd();
	}
};
