// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonCameraControl.h"
#include "Json/GLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonOrthographic
{
	float XMag; // horizontal magnification of the view
	float YMag; // vertical magnification of the view
	float ZFar;
	float ZNear;

	FGLTFJsonOrthographic()
		: XMag(0)
		, YMag(0)
		, ZFar(0)
		, ZNear(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("xmag"), XMag);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("ymag"), YMag);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("zfar"), ZFar);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("znear"), ZNear);

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonPerspective
{
	float AspectRatio; // aspect ratio of the field of view
	float YFov; // vertical field of view in radians
	float ZFar;
	float ZNear;

	FGLTFJsonPerspective()
		: AspectRatio(0)
		, YFov(0)
		, ZFar(0)
		, ZNear(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (AspectRatio != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("aspectRatio"), AspectRatio);
		}

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("yfov"), YFov);

		if (ZFar != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("zfar"), ZFar);
		}

		FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("znear"), ZNear);

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonCamera
{
	FString Name;

	EGLTFJsonCameraType               Type;
	TOptional<FGLTFJsonCameraControl> CameraControl;

	union {
		FGLTFJsonOrthographic Orthographic;
		FGLTFJsonPerspective  Perspective;
	};

	FGLTFJsonCamera()
		: Type(EGLTFJsonCameraType::None)
		, Orthographic()
		, Perspective()
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

		switch (Type)
		{
			case EGLTFJsonCameraType::Orthographic:
				JsonWriter.WriteIdentifierPrefix(TEXT("orthographic"));
				Orthographic.WriteObject(JsonWriter, Extensions);
				break;

			case EGLTFJsonCameraType::Perspective:
				JsonWriter.WriteIdentifierPrefix(TEXT("perspective"));
				Perspective.WriteObject(JsonWriter, Extensions);
				break;

			default:
				break;
		}

		if (CameraControl.IsSet())
		{
			JsonWriter.WriteObjectStart(TEXT("extensions"));

			const EGLTFJsonExtension Extension = EGLTFJsonExtension::EPIC_CameraControls;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
			CameraControl.GetValue().WriteObject(JsonWriter, Extensions);

			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
