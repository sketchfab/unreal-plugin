// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonMatrix4.h"
#include "Json/SKGLTFJsonVector3.h"
#include "Json/SKGLTFJsonQuaternion.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonNode
{
	FString Name;

	bool bUseMatrix;

	union
	{
		FGLTFJsonMatrix4 Matrix;

		struct
		{
			FGLTFJsonVector3    Translation;
			FGLTFJsonQuaternion Rotation;
			FGLTFJsonVector3    Scale;
		};
	};

	FGLTFJsonCameraIndex    Camera;
	FGLTFJsonSkinIndex      Skin;
	FGLTFJsonMeshIndex      Mesh;
	FGLTFJsonBackdropIndex  Backdrop;
	FGLTFJsonHotspotIndex   Hotspot;
	FGLTFJsonLightIndex     Light;
	FGLTFJsonLightMapIndex  LightMap;
	FGLTFJsonSkySphereIndex SkySphere;

	FGLTFJsonNodeIndex ComponentNode;

	TArray<FGLTFJsonNodeIndex> Children;

	FGLTFJsonNode()
		: bUseMatrix(false)
		, Translation(FGLTFJsonVector3::Zero)
		, Rotation(FGLTFJsonQuaternion::Identity)
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

		if (bUseMatrix)
		{
			if (Matrix != FGLTFJsonMatrix4::Identity)
			{
				JsonWriter.WriteIdentifierPrefix(TEXT("matrix"));
				Matrix.WriteArray(JsonWriter);
			}
		}
		else
		{
			if (Translation != FGLTFJsonVector3::Zero)
			{
				JsonWriter.WriteIdentifierPrefix(TEXT("translation"));
				Translation.WriteArray(JsonWriter);
			}

			if (Rotation != FGLTFJsonQuaternion::Identity)
			{
				JsonWriter.WriteIdentifierPrefix(TEXT("rotation"));
				Rotation.WriteArray(JsonWriter);
			}

			if (Scale != FGLTFJsonVector3::One)
			{
				JsonWriter.WriteIdentifierPrefix(TEXT("scale"));
				Scale.WriteArray(JsonWriter);
			}
		}

		if (Camera != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("camera"), Camera);
		}

		if (Skin != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("skin"), Skin);
		}

		if (Mesh != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("mesh"), Mesh);
		}

		if (Backdrop != INDEX_NONE || Hotspot != INDEX_NONE || Light != INDEX_NONE || LightMap != INDEX_NONE || SkySphere != INDEX_NONE)
		{
			JsonWriter.WriteObjectStart(TEXT("extensions"));

			if (Backdrop != INDEX_NONE)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_HDRIBackdrops;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteValue(TEXT("backdrop"), Backdrop);
				JsonWriter.WriteObjectEnd();
			}

			if (Hotspot != INDEX_NONE)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_AnimationHotspots;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteValue(TEXT("hotspot"), Hotspot);
				JsonWriter.WriteObjectEnd();
			}

			if (Light != INDEX_NONE)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::KHR_LightsPunctual;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteValue(TEXT("light"), Light);
				JsonWriter.WriteObjectEnd();
			}

			if (LightMap != INDEX_NONE)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_LightmapTextures;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteValue(TEXT("lightmap"), LightMap);
				JsonWriter.WriteObjectEnd();
			}

			if (SkySphere != INDEX_NONE)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_SkySpheres;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteValue(TEXT("skySphere"), SkySphere);
				JsonWriter.WriteObjectEnd();
			}

			JsonWriter.WriteObjectEnd();
		}

		if (Children.Num() > 0)
		{
			JsonWriter.WriteValue(TEXT("children"), Children);
		}

		JsonWriter.WriteObjectEnd();
	}
};
