// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonMaterial.h"
#include "Json/SKGLTFJsonVector4.h"

struct FGLTFJsonLightMap
{
	FString              Name;
	FGLTFJsonTextureInfo Texture;
	FGLTFJsonVector4     LightMapScale;
	FGLTFJsonVector4     LightMapAdd;
	FGLTFJsonVector4     CoordinateScaleBias;

	FGLTFJsonLightMap()
		: LightMapScale(FGLTFJsonVector4::One)
		, LightMapAdd(FGLTFJsonVector4::Zero)
		, CoordinateScaleBias({ 1, 1, 0, 0 })
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

		if (Texture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("texture"));
			Texture.WriteObject(JsonWriter, Extensions);
		}

		JsonWriter.WriteIdentifierPrefix(TEXT("lightmapScale"));
		LightMapScale.WriteArray(JsonWriter);

		JsonWriter.WriteIdentifierPrefix(TEXT("lightmapAdd"));
		LightMapAdd.WriteArray(JsonWriter);

		JsonWriter.WriteIdentifierPrefix(TEXT("coordinateScaleBias"));
		CoordinateScaleBias.WriteArray(JsonWriter);

		JsonWriter.WriteObjectEnd();
	}
};
