// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonIndex.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonAttributes
{
	FGLTFJsonAccessorIndex Position; // always required
	FGLTFJsonAccessorIndex Color0;
	FGLTFJsonAccessorIndex Normal;
	FGLTFJsonAccessorIndex Tangent;
	TArray<FGLTFJsonAccessorIndex> TexCoords;

	// skeletal mesh attributes
	TArray<FGLTFJsonAccessorIndex> Joints;
	TArray<FGLTFJsonAccessorIndex> Weights;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("POSITION"), Position);

		if (Color0 != INDEX_NONE) JsonWriter.WriteValue(TEXT("COLOR_0"), Color0);
		if (Normal != INDEX_NONE) JsonWriter.WriteValue(TEXT("NORMAL"), Normal);
		if (Tangent != INDEX_NONE) JsonWriter.WriteValue(TEXT("TANGENT"), Tangent);

		for (int32 Index = 0; Index < TexCoords.Num(); ++Index)
		{
			JsonWriter.WriteValue(TEXT("TEXCOORD_") + FString::FromInt(Index), TexCoords[Index]);
		}

		for (int32 Index = 0; Index < Joints.Num(); ++Index)
		{
			JsonWriter.WriteValue(TEXT("JOINTS_") + FString::FromInt(Index), Joints[Index]);
		}

		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			JsonWriter.WriteValue(TEXT("WEIGHTS_") + FString::FromInt(Index), Weights[Index]);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonPrimitive
{
	FGLTFJsonAccessorIndex Indices;
	FGLTFJsonMaterialIndex Material;
	ESKGLTFJsonPrimitiveMode Mode;
	FGLTFJsonAttributes    Attributes;

	FGLTFJsonPrimitive()
		: Mode(ESKGLTFJsonPrimitiveMode::Triangles)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteIdentifierPrefix(TEXT("attributes"));
		Attributes.WriteObject(JsonWriter, Extensions);

		if (Indices != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("indices"), Indices);
		}

		if (Material != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("material"), Material);
		}

		if (Mode != ESKGLTFJsonPrimitiveMode::Triangles)
		{
			JsonWriter.WriteValue(TEXT("mode"), FGLTFJsonUtility::ToInteger(Mode));
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonMesh
{
	FString Name;

	TArray<FGLTFJsonPrimitive> Primitives;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("primitives"), Primitives, Extensions, true);

		JsonWriter.WriteObjectEnd();
	}
};
