// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonExtensions.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonHotspot
{
	FString Name;

	FGLTFJsonAnimationIndex Animation;
	FGLTFJsonTextureIndex   Image;
	FGLTFJsonTextureIndex   HoveredImage;
	FGLTFJsonTextureIndex   ToggledImage;
	FGLTFJsonTextureIndex   ToggledHoveredImage;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		JsonWriter.WriteValue(TEXT("animation"), Animation);
		JsonWriter.WriteValue(TEXT("image"), Image);

		if (HoveredImage != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("hoveredImage"), HoveredImage);
		}

		if (ToggledImage != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("toggledImage"), ToggledImage);
		}

		if (ToggledHoveredImage != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("toggledHoveredImage"), ToggledHoveredImage);
		}

		JsonWriter.WriteObjectEnd();
	}
};
