// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonVector4.h"

struct FGLTFJsonCameraControl
{
	ESKGLTFJsonCameraControlMode Mode;
	FGLTFJsonNodeIndex Target;
	float MaxDistance;
	float MinDistance;
	float MaxPitch;
	float MinPitch;
	float MaxYaw;
	float MinYaw;
	float RotationSensitivity;
	float RotationInertia;
	float DollySensitivity;
	float DollyDuration;

	FGLTFJsonCameraControl()
		: Mode(ESKGLTFJsonCameraControlMode::FreeLook)
		, MaxDistance(0)
		, MinDistance(0)
		, MaxPitch(90.0f)
		, MinPitch(-90.0f)
		, MaxYaw(360.0f)
		, MinYaw(0)
		, RotationSensitivity(0)
		, RotationInertia(0)
		, DollySensitivity(0)
		, DollyDuration(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("mode"), FGLTFJsonUtility::ToString(Mode));

		if (Target != INDEX_NONE && Mode == ESKGLTFJsonCameraControlMode::Orbital)
		{
			JsonWriter.WriteValue(TEXT("target"), Target);
		}

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("maxDistance"), MaxDistance);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("minDistance"), MinDistance);

		if (!FMath::IsNearlyEqual(MaxPitch, 90.0f))
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("maxPitch"), MaxPitch);
		}

		if (!FMath::IsNearlyEqual(MinPitch, -90.0f))
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("minPitch"), MinPitch);
		}

		if (!FMath::IsNearlyEqual(MaxYaw, 360.0f))
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("maxYaw"), MaxYaw);
		}

		if (!FMath::IsNearlyEqual(MinYaw, 0.0f))
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("minYaw"), MinYaw);
		}

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("rotationSensitivity"), RotationSensitivity);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("rotationInertia"), RotationInertia);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("dollySensitivity"), DollySensitivity);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("dollyDuration"), DollyDuration);

		JsonWriter.WriteObjectEnd();
	}
};
