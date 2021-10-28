// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonColor3.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonSpotLight
{
	float InnerConeAngle;
	float OuterConeAngle;

	FGLTFJsonSpotLight()
		: InnerConeAngle(0)
		, OuterConeAngle(PI / 2.0f)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (InnerConeAngle != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("innerConeAngle"), InnerConeAngle);
		}

		if (OuterConeAngle != PI / 2.0f)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("outerConeAngle"), OuterConeAngle);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonLight
{
	FString Name;

	ESKGLTFJsonLightType Type;

	FGLTFJsonColor3 Color;

	float Intensity;
	float Range;

	FGLTFJsonSpotLight Spot;

	FGLTFJsonLight()
		: Type(ESKGLTFJsonLightType::None)
		, Color(FGLTFJsonColor3::White)
		, Intensity(1)
		, Range(0)
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

		JsonWriter.WriteValue(TEXT("type"), FGLTFJsonUtility::ToString(Type));

		if (Color != FGLTFJsonColor3::White)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("color"));
			Color.WriteArray(JsonWriter);
		}

		if (Intensity != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("intensity"), Intensity);
		}

		if (Type == ESKGLTFJsonLightType::Point || Type == ESKGLTFJsonLightType::Spot)
		{
			if (Range != 0)
			{
				FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("range"), Range);
			}

			if (Type == ESKGLTFJsonLightType::Spot)
			{
				JsonWriter.WriteIdentifierPrefix(TEXT("spot"));
				Spot.WriteObject(JsonWriter, Extensions);
			}
		}

		JsonWriter.WriteObjectEnd();
	}
};
