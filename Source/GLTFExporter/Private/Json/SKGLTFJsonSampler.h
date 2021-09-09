// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonSampler
{
	FString Name;

	ESKGLTFJsonTextureFilter MinFilter;
	ESKGLTFJsonTextureFilter MagFilter;

	ESKGLTFJsonTextureWrap WrapS;
	ESKGLTFJsonTextureWrap WrapT;

	FGLTFJsonSampler()
		: MinFilter(ESKGLTFJsonTextureFilter::None)
		, MagFilter(ESKGLTFJsonTextureFilter::None)
		, WrapS(ESKGLTFJsonTextureWrap::Repeat)
		, WrapT(ESKGLTFJsonTextureWrap::Repeat)
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

		if (MinFilter != ESKGLTFJsonTextureFilter::None)
		{
			JsonWriter.WriteValue(TEXT("minFilter"), FGLTFJsonUtility::ToInteger(MinFilter));
		}

		if (MagFilter != ESKGLTFJsonTextureFilter::None)
		{
			JsonWriter.WriteValue(TEXT("magFilter"), FGLTFJsonUtility::ToInteger(MagFilter));
		}

		if (WrapS != ESKGLTFJsonTextureWrap::Repeat)
		{
			JsonWriter.WriteValue(TEXT("wrapS"), FGLTFJsonUtility::ToInteger(WrapS));
		}

		if (WrapT != ESKGLTFJsonTextureWrap::Repeat)
		{
			JsonWriter.WriteValue(TEXT("wrapT"), FGLTFJsonUtility::ToInteger(WrapT));
		}

		JsonWriter.WriteObjectEnd();
	}
};
