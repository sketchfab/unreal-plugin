// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonColor4.h"
#include "Json/SKGLTFJsonExtensions.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonSkySphereColorCurve
{
	struct FKey
	{
		float Time;
		float Value;
	};

	struct FComponentCurve
	{
		TArray<FKey> Keys;
	};

	TArray<FComponentCurve> ComponentCurves;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();

		for (int32 ComponentIndex = 0; ComponentIndex < ComponentCurves.Num(); ++ComponentIndex)
		{
			const FComponentCurve& ComponentCurve = ComponentCurves[ComponentIndex];

			JsonWriter.WriteArrayStart();

			for (const FKey& Key: ComponentCurve.Keys)
			{
				FGLTFJsonUtility::WriteExactValue(JsonWriter, Key.Time);
				FGLTFJsonUtility::WriteExactValue(JsonWriter, Key.Value);
			}

			JsonWriter.WriteArrayEnd();
		}

		JsonWriter.WriteArrayEnd();
	}
};

struct FGLTFJsonSkySphere
{
	FString Name;

	FGLTFJsonMeshIndex    SkySphereMesh;
	FGLTFJsonTextureIndex SkyTexture;
	FGLTFJsonTextureIndex CloudsTexture;
	FGLTFJsonTextureIndex StarsTexture;
	FGLTFJsonNodeIndex    DirectionalLight;

	float SunHeight;
	float SunBrightness;
	float StarsBrightness;
	float CloudSpeed;
	float CloudOpacity;
	float HorizonFalloff;

	float SunRadius;
	float NoisePower1;
	float NoisePower2;

	bool bColorsDeterminedBySunPosition;

	FGLTFJsonColor4 ZenithColor;
	FGLTFJsonColor4 HorizonColor;
	FGLTFJsonColor4 CloudColor;
	FGLTFJsonColor4 OverallColor;

	FGLTFJsonSkySphereColorCurve ZenithColorCurve;
	FGLTFJsonSkySphereColorCurve HorizonColorCurve;
	FGLTFJsonSkySphereColorCurve CloudColorCurve;

	FGLTFJsonVector3 Scale;

	FGLTFJsonSkySphere()
		: SunHeight(0)
		, SunBrightness(0)
		, StarsBrightness(0)
		, CloudSpeed(0)
		, CloudOpacity(0)
		, HorizonFalloff(0)
		, SunRadius(0)
		, NoisePower1(0)
		, NoisePower2(0)
		, bColorsDeterminedBySunPosition(false)
		, ZenithColor(FGLTFJsonColor4::White)
		, HorizonColor(FGLTFJsonColor4::White)
		, CloudColor(FGLTFJsonColor4::White)
		, OverallColor(FGLTFJsonColor4::White)
		, Scale(FGLTFJsonVector3::One)
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

		JsonWriter.WriteValue(TEXT("skySphereMesh"), SkySphereMesh);
		JsonWriter.WriteValue(TEXT("skyTexture"), SkyTexture);
		JsonWriter.WriteValue(TEXT("cloudsTexture"), CloudsTexture);
		JsonWriter.WriteValue(TEXT("starsTexture"), StarsTexture);

		if (DirectionalLight != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("directionalLight"), DirectionalLight);
		}

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("sunHeight"), SunHeight);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("sunBrightness"), SunBrightness);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("starsBrightness"), StarsBrightness);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("cloudSpeed"), CloudSpeed);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("cloudOpacity"), CloudOpacity);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("horizonFalloff"), HorizonFalloff);

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("sunRadius"), SunRadius);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("noisePower1"), NoisePower1);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("noisePower2"), NoisePower2);

		JsonWriter.WriteValue(TEXT("colorsDeterminedBySunPosition"), bColorsDeterminedBySunPosition);

		JsonWriter.WriteIdentifierPrefix(TEXT("zenithColor"));
		ZenithColor.WriteArray(JsonWriter);

		JsonWriter.WriteIdentifierPrefix(TEXT("horizonColor"));
		HorizonColor.WriteArray(JsonWriter);

		JsonWriter.WriteIdentifierPrefix(TEXT("cloudColor"));
		CloudColor.WriteArray(JsonWriter);

		if (OverallColor != FGLTFJsonColor4::White)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("overallColor"));
			OverallColor.WriteArray(JsonWriter);
		}

		if (ZenithColorCurve.ComponentCurves.Num() >= 3)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("zenithColorCurve"));
			ZenithColorCurve.WriteArray(JsonWriter);
		}

		if (HorizonColorCurve.ComponentCurves.Num() >= 3)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("horizonColorCurve"));
			HorizonColorCurve.WriteArray(JsonWriter);
		}

		if (CloudColorCurve.ComponentCurves.Num() >= 3)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("cloudColorCurve"));
			CloudColorCurve.WriteArray(JsonWriter);
		}

		if (Scale != FGLTFJsonVector3::One)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("scale"));
			Scale.WriteArray(JsonWriter);
		}

		JsonWriter.WriteObjectEnd();
	}
};
