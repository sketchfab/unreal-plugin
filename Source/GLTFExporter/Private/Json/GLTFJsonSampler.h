// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonSampler
{
	FString Name;

	EGLTFJsonTextureFilter MinFilter;
	EGLTFJsonTextureFilter MagFilter;

	EGLTFJsonTextureWrap WrapS;
	EGLTFJsonTextureWrap WrapT;

	FGLTFJsonSampler()
		: MinFilter(EGLTFJsonTextureFilter::None)
		, MagFilter(EGLTFJsonTextureFilter::None)
		, WrapS(EGLTFJsonTextureWrap::Repeat)
		, WrapT(EGLTFJsonTextureWrap::Repeat)
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

		if (MinFilter != EGLTFJsonTextureFilter::None)
		{
			JsonWriter.WriteValue(TEXT("minFilter"), FGLTFJsonUtility::ToInteger(MinFilter));
		}

		if (MagFilter != EGLTFJsonTextureFilter::None)
		{
			JsonWriter.WriteValue(TEXT("magFilter"), FGLTFJsonUtility::ToInteger(MagFilter));
		}

		if (WrapS != EGLTFJsonTextureWrap::Repeat)
		{
			JsonWriter.WriteValue(TEXT("wrapS"), FGLTFJsonUtility::ToInteger(WrapS));
		}

		if (WrapT != EGLTFJsonTextureWrap::Repeat)
		{
			JsonWriter.WriteValue(TEXT("wrapT"), FGLTFJsonUtility::ToInteger(WrapT));
		}

		JsonWriter.WriteObjectEnd();
	}
};
