// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Json/SKGLTFJsonVector2.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonTextureTransform
{
	FGLTFJsonVector2 Offset;
	FGLTFJsonVector2 Scale;
	float Rotation;

	FGLTFJsonTextureTransform()
		: Offset(FGLTFJsonVector2::Zero)
		, Scale(FGLTFJsonVector2::One)
		, Rotation(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (Offset != FGLTFJsonVector2::Zero)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("offset"));
			Offset.WriteArray(JsonWriter);
		}

		if (Scale != FGLTFJsonVector2::One)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("scale"));
			Scale.WriteArray(JsonWriter);
		}

		if (Rotation != 0)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("rotation"));
			FGLTFJsonUtility::WriteExactValue(JsonWriter, Rotation);
		}

		JsonWriter.WriteObjectEnd();
	}

	bool operator==(const FGLTFJsonTextureTransform& Other) const
	{
		return Offset == Other.Offset
			&& Scale == Other.Scale
			&& Rotation == Other.Rotation;
	}

	bool operator!=(const FGLTFJsonTextureTransform& Other) const
	{
		return !(*this == Other);
	}
};
