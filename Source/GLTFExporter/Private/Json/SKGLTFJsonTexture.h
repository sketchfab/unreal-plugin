// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonTexture
{
	FString Name;

	FGLTFJsonSamplerIndex Sampler;

	FGLTFJsonImageIndex Source;

	ESKGLTFJsonHDREncoding Encoding;

	FGLTFJsonTexture()
		: Encoding(ESKGLTFJsonHDREncoding::None)
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

		if (Sampler != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("sampler"), Sampler);
		}

		if (Source != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("source"), Source);
		}

		if (Encoding != ESKGLTFJsonHDREncoding::None)
		{
			const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_TextureHDREncoding;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteObjectStart(TEXT("extensions"));
			JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));

			JsonWriter.WriteValue(TEXT("encoding"), FGLTFJsonUtility::ToString(Encoding));

			JsonWriter.WriteObjectEnd();
			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
