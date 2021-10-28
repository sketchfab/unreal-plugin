// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonSkin
{
	FString Name;

	FGLTFJsonAccessorIndex InverseBindMatrices;
	FGLTFJsonNodeIndex Skeleton;

	TArray<FGLTFJsonNodeIndex> Joints;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		if (InverseBindMatrices != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("inverseBindMatrices"), InverseBindMatrices);
		}

		if (Skeleton != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("skeleton"), Skeleton);
		}

		if (Joints.Num() > 0)
		{
			JsonWriter.WriteValue(TEXT("joints"), Joints);
		}

		JsonWriter.WriteObjectEnd();
	}
};
