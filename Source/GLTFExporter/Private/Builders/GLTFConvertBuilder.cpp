// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFConvertBuilder.h"

FGLTFConvertBuilder::FGLTFConvertBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions, bool bSelectedActorsOnly)
	: FGLTFImageBuilder(FilePath, ExportOptions)
	, bSelectedActorsOnly(bSelectedActorsOnly)
{
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddPositionAccessor(const FGLTFMeshSection* MeshSection, const FPositionVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return PositionBufferConverter.GetOrAdd(MeshSection, VertexBuffer);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddColorAccessor(const FGLTFMeshSection* MeshSection, const FColorVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return ColorBufferConverter.GetOrAdd(MeshSection, VertexBuffer);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddNormalAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return NormalBufferConverter.GetOrAdd(MeshSection, VertexBuffer);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddTangentAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return TangentBufferConverter.GetOrAdd(MeshSection, VertexBuffer);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddUVAccessor(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer, int32 UVIndex)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return UVBufferConverter.GetOrAdd(MeshSection, VertexBuffer, UVIndex);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddJointAccessor(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, int32 InfluenceOffset)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return BoneIndexBufferConverter.GetOrAdd(MeshSection, VertexBuffer, InfluenceOffset);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddWeightAccessor(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, int32 InfluenceOffset)
{
	if (VertexBuffer == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return BoneWeightBufferConverter.GetOrAdd(MeshSection, VertexBuffer, InfluenceOffset);
}

FGLTFJsonAccessorIndex FGLTFConvertBuilder::GetOrAddIndexAccessor(const FGLTFMeshSection* MeshSection)
{
	if (MeshSection == nullptr)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	return IndexBufferConverter.GetOrAdd(MeshSection);
}

FGLTFJsonMeshIndex FGLTFConvertBuilder::GetOrAddMesh(const UStaticMesh* StaticMesh, const FGLTFMaterialArray& Materials, int32 LODIndex)
{
	if (StaticMesh == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	return StaticMeshConverter.GetOrAdd(StaticMesh, nullptr, Materials, LODIndex);
}

FGLTFJsonMeshIndex FGLTFConvertBuilder::GetOrAddMesh(const UStaticMeshComponent* StaticMeshComponent, const FGLTFMaterialArray& Materials, int32 LODIndex)
{
	if (StaticMeshComponent == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	if (StaticMesh == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	return StaticMeshConverter.GetOrAdd(StaticMesh, StaticMeshComponent, Materials, LODIndex);
}

FGLTFJsonMeshIndex FGLTFConvertBuilder::GetOrAddMesh(const USkeletalMesh* SkeletalMesh, const FGLTFMaterialArray& Materials, int32 LODIndex)
{
	if (SkeletalMesh == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	return SkeletalMeshConverter.GetOrAdd(SkeletalMesh, nullptr, Materials, LODIndex);
}

FGLTFJsonMeshIndex FGLTFConvertBuilder::GetOrAddMesh(const USkeletalMeshComponent* SkeletalMeshComponent, const FGLTFMaterialArray& Materials, int32 LODIndex)
{
	if (SkeletalMeshComponent == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
	if (SkeletalMesh == nullptr)
	{
		return FGLTFJsonMeshIndex(INDEX_NONE);
	}

	return SkeletalMeshConverter.GetOrAdd(SkeletalMesh, SkeletalMeshComponent, Materials, LODIndex);
}

FGLTFJsonMaterialIndex FGLTFConvertBuilder::GetOrAddMaterial(const UMaterialInterface* Material, const FGLTFMeshData* MeshData, const FGLTFIndexArray& SectionIndices)
{
	if (Material == nullptr)
	{
		return FGLTFJsonMaterialIndex(INDEX_NONE);
	}

	return MaterialConverter.GetOrAdd(Material, MeshData, SectionIndices);
}

FGLTFJsonSamplerIndex FGLTFConvertBuilder::GetOrAddSampler(const UTexture* Texture)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonSamplerIndex(INDEX_NONE);
	}

	return SamplerConverter.GetOrAdd(Texture);
}

FGLTFJsonTextureIndex FGLTFConvertBuilder::GetOrAddTexture(const UTexture2D* Texture)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return Texture2DConverter.GetOrAdd(Texture);
}

FGLTFJsonTextureIndex FGLTFConvertBuilder::GetOrAddTexture(const UTextureCube* Texture, ECubeFace CubeFace)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return TextureCubeConverter.GetOrAdd(Texture, CubeFace);
}

FGLTFJsonTextureIndex FGLTFConvertBuilder::GetOrAddTexture(const UTextureRenderTarget2D* Texture)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return TextureRenderTarget2DConverter.GetOrAdd(Texture);
}

FGLTFJsonTextureIndex FGLTFConvertBuilder::GetOrAddTexture(const UTextureRenderTargetCube* Texture, ECubeFace CubeFace)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return TextureRenderTargetCubeConverter.GetOrAdd(Texture, CubeFace);
}

FGLTFJsonTextureIndex FGLTFConvertBuilder::GetOrAddTexture(const ULightMapTexture2D* Texture)
{
	if (Texture == nullptr)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return TextureLightMapConverter.GetOrAdd(Texture);
}

FGLTFJsonSkinIndex FGLTFConvertBuilder::GetOrAddSkin(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh)
{
	if (RootNode == INDEX_NONE || SkeletalMesh == nullptr)
	{
		return FGLTFJsonSkinIndex(INDEX_NONE);
	}

	return SkinConverter.GetOrAdd(RootNode, SkeletalMesh);
}

FGLTFJsonSkinIndex FGLTFConvertBuilder::GetOrAddSkin(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent)
{
	if (RootNode == INDEX_NONE || SkeletalMeshComponent == nullptr)
	{
		return FGLTFJsonSkinIndex(INDEX_NONE);
	}

	const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;

	if (SkeletalMesh == nullptr)
	{
		return FGLTFJsonSkinIndex(INDEX_NONE);
	}

	return GetOrAddSkin(RootNode, SkeletalMesh);
}

FGLTFJsonAnimationIndex FGLTFConvertBuilder::GetOrAddAnimation(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, const UAnimSequence* AnimSequence)
{
	if (RootNode == INDEX_NONE || SkeletalMesh == nullptr || AnimSequence == nullptr)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	return AnimationConverter.GetOrAdd(RootNode, SkeletalMesh, AnimSequence);
}

FGLTFJsonAnimationIndex FGLTFConvertBuilder::GetOrAddAnimation(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent)
{
	if (RootNode == INDEX_NONE || SkeletalMeshComponent == nullptr)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	return AnimationDataConverter.GetOrAdd(RootNode, SkeletalMeshComponent);
}

FGLTFJsonAnimationIndex FGLTFConvertBuilder::GetOrAddAnimation(const ULevel* Level, const ULevelSequence* LevelSequence)
{
	if (Level == nullptr || LevelSequence == nullptr)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	return LevelSequenceConverter.GetOrAdd(Level, LevelSequence);
}

FGLTFJsonAnimationIndex FGLTFConvertBuilder::GetOrAddAnimation(const ALevelSequenceActor* LevelSequenceActor)
{
	if (LevelSequenceActor == nullptr)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	return LevelSequenceDataConverter.GetOrAdd(LevelSequenceActor);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(const AActor* Actor)
{
	if (Actor == nullptr)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return ActorConverter.GetOrAdd(Actor);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(const USceneComponent* SceneComponent)
{
	if (SceneComponent == nullptr)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return ComponentConverter.GetOrAdd(SceneComponent);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(const USceneComponent* SceneComponent, FName SocketName)
{
	if (SceneComponent == nullptr)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return ComponentSocketConverter.GetOrAdd(SceneComponent, SocketName);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(FGLTFJsonNodeIndex RootNode, const UStaticMesh* StaticMesh, FName SocketName)
{
	if (RootNode == INDEX_NONE || StaticMesh == nullptr || SocketName == NAME_None)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return StaticSocketConverter.GetOrAdd(RootNode, StaticMesh, SocketName);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, FName SocketName)
{
	if (RootNode == INDEX_NONE || SkeletalMesh == nullptr || SocketName == NAME_None)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return SkeletalSocketConverter.GetOrAdd(RootNode, SkeletalMesh, SocketName);
}

FGLTFJsonNodeIndex FGLTFConvertBuilder::GetOrAddNode(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, int32 BoneIndex)
{
	if (RootNode == INDEX_NONE || SkeletalMesh == nullptr || BoneIndex == INDEX_NONE)
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	return SkeletalBoneConverter.GetOrAdd(RootNode, SkeletalMesh, BoneIndex);
}

FGLTFJsonSceneIndex FGLTFConvertBuilder::GetOrAddScene(const UWorld* World)
{
	if (World == nullptr)
	{
		return FGLTFJsonSceneIndex(INDEX_NONE);
	}

	return SceneConverter.GetOrAdd(World);
}

FGLTFJsonCameraIndex FGLTFConvertBuilder::GetOrAddCamera(const UCameraComponent* CameraComponent)
{
	if (CameraComponent == nullptr)
	{
		return FGLTFJsonCameraIndex(INDEX_NONE);
	}

	return CameraConverter.GetOrAdd(CameraComponent);
}

FGLTFJsonLightIndex FGLTFConvertBuilder::GetOrAddLight(const ULightComponent* LightComponent)
{
	if (LightComponent == nullptr)
	{
		return FGLTFJsonLightIndex(INDEX_NONE);
	}

	return LightConverter.GetOrAdd(LightComponent);
}

FGLTFJsonBackdropIndex FGLTFConvertBuilder::GetOrAddBackdrop(const AActor* BackdropActor)
{
	if (BackdropActor == nullptr)
	{
		return FGLTFJsonBackdropIndex(INDEX_NONE);
	}

	return BackdropConverter.GetOrAdd(BackdropActor);
}

FGLTFJsonVariationIndex FGLTFConvertBuilder::GetOrAddVariation(const ULevelVariantSets* LevelVariantSets)
{
	if (LevelVariantSets == nullptr)
	{
		return FGLTFJsonVariationIndex(INDEX_NONE);
	}

	return VariationConverter.GetOrAdd(LevelVariantSets);
}

FGLTFJsonLightMapIndex FGLTFConvertBuilder::GetOrAddLightMap(const UStaticMeshComponent* StaticMeshComponent)
{
	if (StaticMeshComponent == nullptr)
	{
		return FGLTFJsonLightMapIndex(INDEX_NONE);
	}

	return LightMapConverter.GetOrAdd(StaticMeshComponent);
}

FGLTFJsonHotspotIndex FGLTFConvertBuilder::GetOrAddHotspot(const ASKGLTFHotspotActor* HotspotActor)
{
	if (HotspotActor == nullptr)
	{
		return FGLTFJsonHotspotIndex(INDEX_NONE);
	}

	return HotspotConverter.GetOrAdd(HotspotActor);
}

FGLTFJsonSkySphereIndex FGLTFConvertBuilder::GetOrAddSkySphere(const AActor* SkySphereActor)
{
	if (SkySphereActor == nullptr)
	{
		return FGLTFJsonSkySphereIndex(INDEX_NONE);
	}

	return SkySphereConverter.GetOrAdd(SkySphereActor);
}
