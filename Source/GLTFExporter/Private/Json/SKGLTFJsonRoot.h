// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonExtensions.h"
#include "Json/SKGLTFJsonAccessor.h"
#include "Json/SKGLTFJsonAnimation.h"
#include "Json/SKGLTFJsonBuffer.h"
#include "Json/SKGLTFJsonBufferView.h"
#include "Json/SKGLTFJsonCamera.h"
#include "Json/SKGLTFJsonImage.h"
#include "Json/SKGLTFJsonMaterial.h"
#include "Json/SKGLTFJsonMesh.h"
#include "Json/SKGLTFJsonNode.h"
#include "Json/SKGLTFJsonSampler.h"
#include "Json/SKGLTFJsonScene.h"
#include "Json/SKGLTFJsonSkin.h"
#include "Json/SKGLTFJsonTexture.h"
#include "Json/SKGLTFJsonBackdrop.h"
#include "Json/SKGLTFJsonHotspot.h"
#include "Json/SKGLTFJsonLight.h"
#include "Json/SKGLTFJsonLightMap.h"
#include "Json/SKGLTFJsonSkySphere.h"
#include "Json/SKGLTFJsonVariation.h"
#include "Policies/CondensedJsonPrintPolicy.h"


struct FGLTFJsonAsset
{
	FString Version;
	FString Generator;
	FString Copyright;

	FGLTFJsonAsset();

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("version"), Version);

		if (!Generator.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("generator"), Generator);
		}

		if (!Copyright.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("copyright"), Copyright);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonRoot
{
	FGLTFJsonAsset Asset;

	FGLTFJsonExtensions Extensions;

	FGLTFJsonSceneIndex DefaultScene;

	TArray<TUniquePtr<FGLTFJsonAccessor>>   Accessors;
	TArray<TUniquePtr<FGLTFJsonAnimation>>  Animations;
	TArray<TUniquePtr<FGLTFJsonBuffer>>     Buffers;
	TArray<TUniquePtr<FGLTFJsonBufferView>> BufferViews;
	TArray<TUniquePtr<FGLTFJsonCamera>>     Cameras;
	TArray<TUniquePtr<FGLTFJsonMaterial>>   Materials;
	TArray<TUniquePtr<FGLTFJsonMesh>>       Meshes;
	TArray<TUniquePtr<FGLTFJsonNode>>       Nodes;
	TArray<TUniquePtr<FGLTFJsonImage>>      Images;
	TArray<TUniquePtr<FGLTFJsonSampler>>    Samplers;
	TArray<TUniquePtr<FGLTFJsonScene>>      Scenes;
	TArray<TUniquePtr<FGLTFJsonSkin>>       Skins;
	TArray<TUniquePtr<FGLTFJsonTexture>>    Textures;
	TArray<TUniquePtr<FGLTFJsonBackdrop>>   Backdrops;
	TArray<TUniquePtr<FGLTFJsonHotspot>>    Hotspots;
	TArray<TUniquePtr<FGLTFJsonLight>>      Lights;
	TArray<TUniquePtr<FGLTFJsonLightMap>>   LightMaps;
	TArray<TUniquePtr<FGLTFJsonSkySphere>>  SkySpheres;
	TArray<TUniquePtr<FGLTFJsonVariation>>  Variations;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter)
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteIdentifierPrefix(TEXT("asset"));
		Asset.WriteObject(JsonWriter, Extensions);

		if (DefaultScene != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("scene"), DefaultScene);
		}

		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("accessors"), Accessors, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("animations"), Animations, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("buffers"), Buffers, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("bufferViews"), BufferViews, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("cameras"), Cameras, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("images"), Images, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("materials"), Materials, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("meshes"), Meshes, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("nodes"), Nodes, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("samplers"), Samplers, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("scenes"), Scenes, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("skins"), Skins, Extensions);
		FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("textures"), Textures, Extensions);

		if (Backdrops.Num() > 0 || Hotspots.Num() > 0 || Lights.Num() > 0 || LightMaps.Num() > 0 || SkySpheres.Num() > 0 || Variations.Num() > 0)
		{
			JsonWriter.WriteObjectStart(TEXT("extensions"));

			if (Backdrops.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_HDRIBackdrops;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("backdrops"), Backdrops, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			if (Hotspots.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_AnimationHotspots;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("hotspots"), Hotspots, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			if (Variations.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_LevelVariantSets;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("levelVariantSets"), Variations, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			if (Lights.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::KHR_LightsPunctual;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("lights"), Lights, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			if (LightMaps.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_LightmapTextures;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("lightmaps"), LightMaps, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			if (SkySpheres.Num() > 0)
			{
				const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_SkySpheres;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteObjectStart(FGLTFJsonUtility::ToString(Extension));
				FGLTFJsonUtility::WriteObjectPtrArray(JsonWriter, TEXT("skySpheres"), SkySpheres, Extensions);
				JsonWriter.WriteObjectEnd();
			}

			JsonWriter.WriteObjectEnd();
		}

		FGLTFJsonUtility::WriteStringArray(JsonWriter, TEXT("extensionsUsed"), Extensions.Used);
		FGLTFJsonUtility::WriteStringArray(JsonWriter, TEXT("extensionsRequired"), Extensions.Required);

		JsonWriter.WriteObjectEnd();
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void ToJson(FArchive* const Archive)
	{
		TSharedRef<TJsonWriter<CharType, PrintPolicy>> JsonWriter = TJsonWriterFactory<CharType, PrintPolicy>::Create(Archive);
		WriteObject(*JsonWriter);
		JsonWriter->Close();
	}

	void ToJson(FArchive* const Archive, bool bCondensedPrint)
	{
		if (bCondensedPrint)
		{
			ToJson<UTF8CHAR, TCondensedJsonPrintPolicy<UTF8CHAR>>(Archive);
		}
		else
		{
			ToJson<UTF8CHAR, TPrettyJsonPrintPolicy<UTF8CHAR>>(Archive);
		}
	}
};
