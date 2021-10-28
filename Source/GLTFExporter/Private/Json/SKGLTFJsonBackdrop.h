// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonVector3.h"
#include "Json/SKGLTFJsonUtility.h"

struct FGLTFJsonBackdrop
{
	FString Name;

	FGLTFJsonMeshIndex Mesh;
	FGLTFJsonTextureIndex Cubemap[6];

	float Intensity;
	float Size;
	float Angle;

	FGLTFJsonVector3 ProjectionCenter;

	float LightingDistanceFactor;
	bool UseCameraProjection;

	FGLTFJsonBackdrop()
		: Intensity(0)
		, Size(0)
		, Angle(0)
		, ProjectionCenter(FGLTFJsonVector3::Zero)
		, LightingDistanceFactor(0)
		, UseCameraProjection(false)
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

		if (Mesh != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("mesh"), Mesh);
		}

		FGLTFJsonUtility::WriteFixedArray(JsonWriter, TEXT("cubemap"), Cubemap);

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("intensity"), Intensity);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("size"), Size);

		if (!FMath::IsNearlyZero(Angle))
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("angle"), Angle);
		}

		JsonWriter.WriteIdentifierPrefix(TEXT("projectionCenter"));
		ProjectionCenter.WriteArray(JsonWriter);

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("lightingDistanceFactor"), LightingDistanceFactor);
		JsonWriter.WriteValue(TEXT("useCameraProjection"), UseCameraProjection);

		JsonWriter.WriteObjectEnd();
	}
};
