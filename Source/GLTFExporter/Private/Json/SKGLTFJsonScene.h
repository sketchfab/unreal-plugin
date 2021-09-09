// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonScene
{
	FString Name;

	TArray<FGLTFJsonNodeIndex> Nodes;
	TArray<FGLTFJsonVariationIndex>  Variations;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		if (Nodes.Num() > 0)
		{
			JsonWriter.WriteValue(TEXT("nodes"), Nodes);
		}

		if (Variations.Num() > 0)
		{
			const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_LevelVariantSets;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteObjectStart(TEXT("extensions"));
			JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));

			JsonWriter.WriteValue(TEXT("levelVariantSets"), Variations);

			JsonWriter.WriteObjectEnd();
			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
