// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonCameraControl.h"
#include "Json/SKGLTFJsonUtility.h"
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

	ESKGLTFJsonCameraType               Type;
	TOptional<FGLTFJsonCameraControl> CameraControl;

	union {
		FGLTFJsonOrthographic Orthographic;
		FGLTFJsonPerspective  Perspective;
	};

	FGLTFJsonCamera()
		: Type(ESKGLTFJsonCameraType::None)
		, Orthographic()
#if (ENGINE_MAJOR_VERSION == 5 || ENGINE_MINOR_VERSION <=27) && PLATFORM_WINDOWS
		, Perspective()
#endif
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
			case ESKGLTFJsonCameraType::Orthographic:
				JsonWriter.WriteIdentifierPrefix(TEXT("orthographic"));
				Orthographic.WriteObject(JsonWriter, Extensions);
				break;

			case ESKGLTFJsonCameraType::Perspective:
				JsonWriter.WriteIdentifierPrefix(TEXT("perspective"));
				Perspective.WriteObject(JsonWriter, Extensions);
				break;

			default:
				break;
		}

		if (CameraControl.IsSet())
		{
			JsonWriter.WriteObjectStart(TEXT("extensions"));

			const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_CameraControls;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
			CameraControl.GetValue().WriteObject(JsonWriter, Extensions);

			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
