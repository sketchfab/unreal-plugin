// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFImageBuilder.h"
#include "Converters/SKGLTFAccessorConverters.h"
#include "Converters/SKGLTFMeshConverters.h"
#include "Converters/SKGLTFMeshDataConverters.h"
#include "Converters/SKGLTFMaterialConverters.h"
#include "Converters/SKGLTFSamplerConverters.h"
#include "Converters/SKGLTFTextureConverters.h"
#include "Converters/SKGLTFNodeConverters.h"
#include "Converters/SKGLTFSkinConverters.h"
#include "Converters/SKGLTFAnimationConverters.h"
#include "Converters/SKGLTFSceneConverters.h"
#include "Converters/SKGLTFCameraConverters.h"
#include "Converters/SKGLTFLightConverters.h"
#include "Converters/SKGLTFBackdropConverters.h"
#include "Converters/SKGLTFVarationConverters.h"
#include "Converters/SKGLTFLightMapConverters.h"
#include "Converters/SKGLTFHotspotConverters.h"
#include "Converters/SKGLTFSkySphereConverters.h"

class FGLTFConvertBuilder : public FGLTFImageBuilder
{
protected:

	FGLTFConvertBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions, bool bSelectedActorsOnly);

public:

	const bool bSelectedActorsOnly;

	// TODO: find a better place for this types of indirect converters
	FGLTFStaticMeshDataConverter StaticMeshDataConverter;
	FGLTFSkeletalMeshDataConverter SkeletalMeshDataConverter;

	FGLTFJsonAccessorIndex GetOrAddPositionAccessor(const FGLTFMeshSection* MeshSection, const FPositionVertexBuffer* VertexBuffer);
	FGLTFJsonAccessorIndex GetOrAddColorAccessor(const FGLTFMeshSection* MeshSection, const FColorVertexBuffer* VertexBuffer);
	FGLTFJsonAccessorIndex GetOrAddNormalAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer);
	FGLTFJsonAccessorIndex GetOrAddTangentAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer);
	FGLTFJsonAccessorIndex GetOrAddUVAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer, int32 UVIndex);
	FGLTFJsonAccessorIndex GetOrAddJointAccessor(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, int32 InfluenceOffset);
	FGLTFJsonAccessorIndex GetOrAddWeightAccessor(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, int32 InfluenceOffset);
	FGLTFJsonAccessorIndex GetOrAddIndexAccessor(const FGLTFMeshSection* MeshSection);

	FGLTFJsonMeshIndex GetOrAddMesh(const UStaticMesh* StaticMesh, const FGLTFMaterialArray& Materials = {}, int32 LODIndex = -1);
	FGLTFJsonMeshIndex GetOrAddMesh(const UStaticMeshComponent* StaticMeshComponent, const FGLTFMaterialArray& Materials = {}, int32 LODIndex = -1);
	FGLTFJsonMeshIndex GetOrAddMesh(const USkeletalMesh* SkeletalMesh, const FGLTFMaterialArray& Materials = {}, int32 LODIndex = -1);
	FGLTFJsonMeshIndex GetOrAddMesh(const USkeletalMeshComponent* SkeletalMeshComponent, const FGLTFMaterialArray& Materials = {}, int32 LODIndex = -1);

	FGLTFJsonMaterialIndex GetOrAddMaterial(const UMaterialInterface* Material, const FGLTFMeshData* MeshData = nullptr, const FGLTFIndexArray& SectionIndices = {});

	FGLTFJsonSamplerIndex GetOrAddSampler(const UTexture* Texture);
	FGLTFJsonTextureIndex GetOrAddTexture(const UTexture2D* Texture);
	FGLTFJsonTextureIndex GetOrAddTexture(const UTextureCube* Texture, ECubeFace CubeFace);
	FGLTFJsonTextureIndex GetOrAddTexture(const UTextureRenderTarget2D* Texture);
	FGLTFJsonTextureIndex GetOrAddTexture(const UTextureRenderTargetCube* Texture, ECubeFace CubeFace);
	FGLTFJsonTextureIndex GetOrAddTexture(const ULightMapTexture2D* Texture);

	FGLTFJsonSkinIndex GetOrAddSkin(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh);
	FGLTFJsonSkinIndex GetOrAddSkin(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent);
	FGLTFJsonAnimationIndex GetOrAddAnimation(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, const UAnimSequence* AnimSequence);
	FGLTFJsonAnimationIndex GetOrAddAnimation(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent);
	FGLTFJsonAnimationIndex GetOrAddAnimation(const ULevel* Level, const ULevelSequence* LevelSequence);
	FGLTFJsonAnimationIndex GetOrAddAnimation(const ALevelSequenceActor* LevelSequenceActor);

	FGLTFJsonNodeIndex GetOrAddNode(const AActor* Actor);
	FGLTFJsonNodeIndex GetOrAddNode(const USceneComponent* SceneComponent);
	FGLTFJsonNodeIndex GetOrAddNode(const USceneComponent* SceneComponent, FName SocketName);
	FGLTFJsonNodeIndex GetOrAddNode(FGLTFJsonNodeIndex RootNode, const UStaticMesh* StaticMesh, FName SocketName);
	FGLTFJsonNodeIndex GetOrAddNode(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, FName SocketName);
	FGLTFJsonNodeIndex GetOrAddNode(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, int32 BoneIndex);
	FGLTFJsonSceneIndex GetOrAddScene(const UWorld* World);

	FGLTFJsonCameraIndex GetOrAddCamera(const UCameraComponent* CameraComponent);
	FGLTFJsonLightIndex GetOrAddLight(const ULightComponent* LightComponent);
	FGLTFJsonBackdropIndex GetOrAddBackdrop(const AActor* BackdropActor);
	FGLTFJsonVariationIndex GetOrAddVariation(const ULevelVariantSets* LevelVariantSets);
	FGLTFJsonLightMapIndex GetOrAddLightMap(const UStaticMeshComponent* StaticMeshComponent);
	FGLTFJsonHotspotIndex GetOrAddHotspot(const ASKGLTFHotspotActor* HotspotActor);
	FGLTFJsonSkySphereIndex GetOrAddSkySphere(const AActor* SkySphereActor);

private:

	FGLTFPositionBufferConverter PositionBufferConverter{ *this };
	FGLTFColorBufferConverter ColorBufferConverter{ *this };
	FGLTFNormalBufferConverter NormalBufferConverter{ *this };
	FGLTFTangentBufferConverter TangentBufferConverter{ *this };
	FGLTFUVBufferConverter UVBufferConverter{ *this };
	FGLTFBoneIndexBufferConverter BoneIndexBufferConverter{ *this };
	FGLTFBoneWeightBufferConverter BoneWeightBufferConverter{ *this };
	FGLTFIndexBufferConverter IndexBufferConverter{ *this };

	FGLTFStaticMeshConverter StaticMeshConverter{ *this };
	FGLTFSkeletalMeshConverter SkeletalMeshConverter{ *this };

	FGLTFMaterialConverter MaterialConverter{ *this };
	FGLTFSamplerConverter SamplerConverter{ *this };
	FGLTFTexture2DConverter Texture2DConverter{ *this };
	FGLTFTextureCubeConverter TextureCubeConverter{ *this };
	FGLTFTextureRenderTarget2DConverter TextureRenderTarget2DConverter{ *this };
	FGLTFTextureRenderTargetCubeConverter TextureRenderTargetCubeConverter{ *this };
	FGLTFTextureLightMapConverter TextureLightMapConverter{ *this };

	FGLTFSkinConverter SkinConverter{ *this };
	FGLTFAnimationConverter AnimationConverter{ *this };
	FGLTFAnimationDataConverter AnimationDataConverter{ *this };
	FGLTFLevelSequenceConverter LevelSequenceConverter{ *this };
	FGLTFLevelSequenceDataConverter LevelSequenceDataConverter{ *this };

	FGLTFActorConverter ActorConverter{ *this };
	FGLTFComponentConverter ComponentConverter{ *this };
	FGLTFComponentSocketConverter ComponentSocketConverter{ *this };
	FGLTFStaticSocketConverter StaticSocketConverter{ *this };
	FGLTFSkeletalSocketConverter SkeletalSocketConverter{ *this };
	FGLTFSkeletalBoneConverter SkeletalBoneConverter{ *this };
	FGLTFSceneConverter SceneConverter{ *this };

	FGLTFCameraConverter CameraConverter{ *this };
	FGLTFLightConverter LightConverter{ *this };
	FGLTFBackdropConverter BackdropConverter{ *this };
	FGLTFVariationConverter VariationConverter{ *this };
	FGLTFLightMapConverter LightMapConverter{ *this };
	FGLTFHotspotConverter HotspotConverter{ *this };
	FGLTFSkySphereConverter SkySphereConverter{ *this };
};
