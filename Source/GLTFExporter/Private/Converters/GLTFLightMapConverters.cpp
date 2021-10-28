// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFLightMapConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Engine/MapBuildDataRegistry.h"

FGLTFJsonLightMapIndex FGLTFLightMapConverter::Convert(const UStaticMeshComponent* StaticMeshComponent)
{
	const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();

	if (StaticMesh == nullptr)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	const int32 LODIndex = StaticMeshComponent->ForcedLodModel > 0 ? StaticMeshComponent->ForcedLodModel - 1 : Builder.ExportOptions->DefaultLevelOfDetail;

	if (LODIndex < 0 || StaticMesh->GetNumLODs() <= LODIndex || StaticMeshComponent->LODData.Num() <= LODIndex)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const int32 CoordinateIndex = StaticMesh->LightMapCoordinateIndex;
#else
	const int32 CoordinateIndex = StaticMesh->GetLightMapCoordinateIndex();
#endif
	const FStaticMeshLODResources& LODResources = StaticMesh->GetLODForExport(LODIndex);

	if (CoordinateIndex < 0 || CoordinateIndex >= LODResources.GetNumTexCoords())
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	const FStaticMeshComponentLODInfo& ComponentLODInfo = StaticMeshComponent->LODData[LODIndex];
	const FMeshMapBuildData* MeshMapBuildData = StaticMeshComponent->GetMeshMapBuildData(ComponentLODInfo);

	if (MeshMapBuildData == nullptr || MeshMapBuildData->LightMap == nullptr)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	const FLightMap2D* LightMap2D = MeshMapBuildData->LightMap->GetLightMap2D();
	if (LightMap2D == nullptr)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	const FLightMapInteraction LightMapInteraction = LightMap2D->GetInteraction(GMaxRHIFeatureLevel);
	const ULightMapTexture2D* Texture = LightMapInteraction.GetTexture(true);
	const FGLTFJsonTextureIndex TextureIndex = Builder.GetOrAddTexture(Texture);

	if (TextureIndex == INDEX_NONE)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	const FVector2D& CoordinateBias = LightMap2D->GetCoordinateBias();
	const FVector2D& CoordinateScale = LightMap2D->GetCoordinateScale();
	const FVector4& LightMapAdd = LightMapInteraction.GetAddArray()[0];
	const FVector4& LightMapScale = LightMapInteraction.GetScaleArray()[0];

	FGLTFJsonLightMap JsonLightMap;
	StaticMeshComponent->GetName(JsonLightMap.Name); // TODO: use better name (similar to light and camera)
	JsonLightMap.Texture.Index = TextureIndex;
	JsonLightMap.Texture.TexCoord = CoordinateIndex;
	JsonLightMap.LightMapScale = { LightMapScale.X, LightMapScale.Y, LightMapScale.Z, LightMapScale.W };
	JsonLightMap.LightMapAdd = { LightMapAdd.X, LightMapAdd.Y, LightMapAdd.Z, LightMapAdd.W };
	JsonLightMap.CoordinateScaleBias = { CoordinateScale.X, CoordinateScale.Y, CoordinateBias.X, CoordinateBias.Y };

	return Builder.AddLightMap(JsonLightMap);
}
