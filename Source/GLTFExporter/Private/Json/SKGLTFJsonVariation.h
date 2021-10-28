// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonExtensions.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Optional.h"

struct FGLTFJsonVariantMaterial
{
	FGLTFJsonMaterialIndex Material;
	int32                  Index;

	FGLTFJsonVariantMaterial()
		: Material(INDEX_NONE)
		, Index(INDEX_NONE)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("material"), Material);

		if (Index != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("index"), Index);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonVariantNodeProperties
{
	FGLTFJsonNodeIndex Node;
	TOptional<bool>    bIsVisible;

	TOptional<FGLTFJsonMeshIndex>    Mesh;
	TArray<FGLTFJsonVariantMaterial> Materials;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (Node != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("node"), Node);
		}

		JsonWriter.WriteObjectStart(TEXT("properties"));

		if (bIsVisible.IsSet())
		{
			JsonWriter.WriteValue(TEXT("visible"), bIsVisible.GetValue());
		}

		if (Mesh.IsSet())
		{
			JsonWriter.WriteValue(TEXT("mesh"), Mesh.GetValue());
		}

		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("materials"), Materials, Extensions);

		JsonWriter.WriteObjectEnd();
		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonVariant
{
	typedef TMap<FGLTFJsonNodeIndex, FGLTFJsonVariantNodeProperties> FNodeProperties;

	FString Name;
	bool    bIsActive;

	FGLTFJsonTextureIndex Thumbnail;
	FNodeProperties       Nodes;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();
		JsonWriter.WriteValue(TEXT("name"), Name);
		JsonWriter.WriteValue(TEXT("active"), bIsActive);

		if (Thumbnail != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("thumbnail"), Thumbnail);
		}

		JsonWriter.WriteArrayStart(TEXT("nodes"));

		for (const auto& DataPair: Nodes)
		{
			if (DataPair.Key != INDEX_NONE)
			{
				DataPair.Value.WriteObject(JsonWriter, Extensions);
			}
		}

		JsonWriter.WriteArrayEnd();
		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonVariantSet
{
	FString Name;

	TArray<FGLTFJsonVariant> Variants;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("variants"), Variants, Extensions);

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonVariation
{
	FString Name;

	TArray<FGLTFJsonVariantSet> VariantSets;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("variantSets"), VariantSets, Extensions);

		JsonWriter.WriteObjectEnd();
	}
};
