// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFNodeConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFActorUtility.h"
#include "Converters/SKGLTFNameUtility.h"
#include "Actors/SKGLTFCameraActor.h"
#include "LevelSequenceActor.h"

FGLTFJsonNodeIndex FGLTFActorConverter::Convert(const AActor* Actor)
{
	if (Builder.bSelectedActorsOnly && !Actor->IsSelected())
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	const USceneComponent* RootComponent = Actor->GetRootComponent();
	const FGLTFJsonNodeIndex RootNodeIndex = Builder.GetOrAddNode(RootComponent);

	const UBlueprint* Blueprint = FGLTFActorUtility::GetBlueprintFromActor(Actor);
	if (FGLTFActorUtility::IsSkySphereBlueprint(Blueprint))
	{
		if (Builder.ExportOptions->bExportSkySpheres)
		{
			FGLTFJsonNode& RootNode = Builder.GetNode(RootNodeIndex);
			RootNode.SkySphere = Builder.GetOrAddSkySphere(Actor);
		}
	}
	else if (FGLTFActorUtility::IsHDRIBackdropBlueprint(Blueprint))
	{
		if (Builder.ExportOptions->bExportHDRIBackdrops)
		{
			FGLTFJsonNode& RootNode = Builder.GetNode(RootNodeIndex);
			RootNode.Backdrop = Builder.GetOrAddBackdrop(Actor);
		}
	}
	else if (const ALevelSequenceActor* LevelSequenceActor = Cast<ALevelSequenceActor>(Actor))
	{
		if (Builder.ExportOptions->bExportLevelSequences)
		{
			Builder.GetOrAddAnimation(LevelSequenceActor);
		}
	}
	else if (const ASKGLTFHotspotActor* HotspotActor = Cast<ASKGLTFHotspotActor>(Actor))
	{
		if (Builder.ExportOptions->bExportAnimationHotspots)
		{
			FGLTFJsonNode& RootNode = Builder.GetNode(RootNodeIndex);
			RootNode.Hotspot = Builder.GetOrAddHotspot(HotspotActor);
		}
	}
	else
	{
		// TODO: to reduce number of nodes, only export components that are of interest

		for (const UActorComponent* Component : Actor->GetComponents())
		{
			const USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
			if (SceneComponent != nullptr)
			{
				Builder.GetOrAddNode(SceneComponent);
			}
		}
	}

	return RootNodeIndex;
}

FGLTFJsonNodeIndex FGLTFComponentConverter::Convert(const USceneComponent* SceneComponent)
{
	const AActor* Owner = SceneComponent->GetOwner();
	if (Owner == nullptr)
	{
		// TODO: report error (invalid scene component)
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	if (Builder.bSelectedActorsOnly && !Owner->IsSelected())
	{
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	const bool bIsRootComponent = Owner->GetRootComponent() == SceneComponent;
	const bool bIsRootNode = bIsRootComponent && FGLTFActorUtility::IsRootActor(Owner, Builder.bSelectedActorsOnly);

	const USceneComponent* ParentComponent = !bIsRootNode ? SceneComponent->GetAttachParent() : nullptr;
	const FName SocketName = SceneComponent->GetAttachSocketName();
	const FGLTFJsonNodeIndex ParentNodeIndex = Builder.GetOrAddNode(ParentComponent, SocketName);

	if (ParentComponent != nullptr && !SceneComponent->IsUsingAbsoluteScale())
	{
		const FVector ParentScale = ParentComponent->GetComponentScale();
		if (!ParentScale.IsUniform())
		{
			Builder.AddWarningMessage(
				FString::Printf(TEXT("Non-uniform parent scale (%s) for component %s (in actor %s) may be represented differently in glTF"),
				*ParentScale.ToString(),
				*SceneComponent->GetName(),
				*Owner->GetName()));
		}
	}

	const FTransform Transform = SceneComponent->GetComponentTransform();
	const FTransform ParentTransform = ParentComponent != nullptr ? ParentComponent->GetSocketTransform(SocketName) : FTransform::Identity;
	const FTransform RelativeTransform = bIsRootNode ? Transform : Transform.GetRelativeTransform(ParentTransform);

	const FGLTFJsonNodeIndex NodeIndex = Builder.AddChildNode(ParentNodeIndex);
	FGLTFJsonNode& Node = Builder.GetNode(NodeIndex);
	Node.Name = FGLTFNameUtility::GetName(SceneComponent);
	Node.Translation = FGLTFConverterUtility::ConvertPosition(RelativeTransform.GetTranslation(), Builder.ExportOptions->ExportUniformScale);
	Node.Rotation = FGLTFConverterUtility::ConvertRotation(RelativeTransform.GetRotation());
	Node.Scale = FGLTFConverterUtility::ConvertScale(RelativeTransform.GetScale3D());

	// TODO: don't export invisible components unless visibility is variable due to variant sets

	// TODO: should hidden in game be configurable like this?
	if (!SceneComponent->bHiddenInGame || Builder.ExportOptions->bExportHiddenInGame)
	{
		if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent))
		{
			Node.Mesh = Builder.GetOrAddMesh(StaticMeshComponent);

			if (Builder.ExportOptions->bExportLightmaps)
			{
				Node.LightMap = Builder.GetOrAddLightMap(StaticMeshComponent);
			}
		}
		else if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SceneComponent))
		{
			Node.Mesh = Builder.GetOrAddMesh(SkeletalMeshComponent);

			if (Builder.ExportOptions->bExportVertexSkinWeights)
			{
				// TODO: remove need for NodeIndex by adding support for cyclic calls in converter
				const FGLTFJsonSkinIndex SkinIndex = Builder.GetOrAddSkin(NodeIndex, SkeletalMeshComponent);
				if (SkinIndex != INDEX_NONE)
				{
					Builder.GetNode(NodeIndex).Skin = SkinIndex;

					if (Builder.ExportOptions->bExportAnimationSequences)
					{
						Builder.GetOrAddAnimation(NodeIndex, SkeletalMeshComponent);
					}
				}
			}
		}
		else if (const UCameraComponent* CameraComponent = Cast<UCameraComponent>(SceneComponent))
		{
			if (Builder.ExportOptions->bExportCameras)
			{
				// TODO: conversion of camera direction should be done in separate converter
				FGLTFJsonNode CameraNode;
				CameraNode.Name = FGLTFNameUtility::GetName(CameraComponent);
				CameraNode.Rotation = FGLTFConverterUtility::ConvertCameraDirection();
				CameraNode.Camera = Builder.GetOrAddCamera(CameraComponent);
				Builder.AddChildComponentNode(NodeIndex, CameraNode);
			}
		}
		else if (const ULightComponent* LightComponent = Cast<ULightComponent>(SceneComponent))
		{
			if (Builder.ShouldExportLight(LightComponent->Mobility))
			{
				// TODO: conversion of light direction should be done in separate converter
				FGLTFJsonNode LightNode;
				LightNode.Name = FGLTFNameUtility::GetName(LightComponent);
				LightNode.Rotation = FGLTFConverterUtility::ConvertLightDirection();
				LightNode.Light = Builder.GetOrAddLight(LightComponent);
				Builder.AddChildComponentNode(NodeIndex, LightNode);
			}
		}
	}

	return NodeIndex;
}

FGLTFJsonNodeIndex FGLTFComponentSocketConverter::Convert(const USceneComponent* SceneComponent, FName SocketName)
{
	const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(SceneComponent);

	if (SocketName != NAME_None)
	{
		if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent))
		{
			const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
			return Builder.GetOrAddNode(NodeIndex, StaticMesh, SocketName);
		}

		if (const USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(SceneComponent))
		{
			// TODO: add support for SocketOverrideLookup?
			const USkeletalMesh* SkeletalMesh = SkinnedMeshComponent->SkeletalMesh;
			return Builder.GetOrAddNode(NodeIndex, SkeletalMesh, SocketName);
		}

		// TODO: add support for more socket types

		Builder.AddWarningMessage(
			FString::Printf(TEXT("Can't export socket %s because it belongs to an unsupported mesh component %s"),
			*SocketName.ToString(),
			*SceneComponent->GetName()));
	}

	return NodeIndex;
}

FGLTFJsonNodeIndex FGLTFStaticSocketConverter::Convert(FGLTFJsonNodeIndex RootNode, const UStaticMesh* StaticMesh, FName SocketName)
{
	const UStaticMeshSocket* Socket = StaticMesh->FindSocket(SocketName);
	if (Socket == nullptr)
	{
		// TODO: report error
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	FGLTFJsonNode Node;
	Node.Name = SocketName.ToString();

	// TODO: add warning check for non-uniform scaling
	Node.Translation = FGLTFConverterUtility::ConvertPosition(Socket->RelativeLocation, Builder.ExportOptions->ExportUniformScale);
	Node.Rotation = FGLTFConverterUtility::ConvertRotation(Socket->RelativeRotation.Quaternion());
	Node.Scale = FGLTFConverterUtility::ConvertScale(Socket->RelativeScale);

	return Builder.AddChildNode(RootNode, Node);
}

FGLTFJsonNodeIndex FGLTFSkeletalSocketConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, FName SocketName)
{
	const USkeletalMeshSocket* Socket = SkeletalMesh->FindSocket(SocketName);
	if (Socket != nullptr)
	{
		FGLTFJsonNode Node;
		Node.Name = SocketName.ToString();

		// TODO: add warning check for non-uniform scaling
		Node.Translation = FGLTFConverterUtility::ConvertPosition(Socket->RelativeLocation, Builder.ExportOptions->ExportUniformScale);
		Node.Rotation = FGLTFConverterUtility::ConvertRotation(Socket->RelativeRotation.Quaternion());
		Node.Scale = FGLTFConverterUtility::ConvertScale(Socket->RelativeScale);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
		const int32 ParentBone = SkeletalMesh->RefSkeleton.FindBoneIndex(Socket->BoneName);
#else
		const int32 ParentBone = SkeletalMesh->GetRefSkeleton().FindBoneIndex(Socket->BoneName);
#endif
		const FGLTFJsonNodeIndex ParentNode = ParentBone != INDEX_NONE ? Builder.GetOrAddNode(RootNode, SkeletalMesh, ParentBone) : RootNode;
		return Builder.AddChildNode(ParentNode, Node);
	}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const int32 BoneIndex = SkeletalMesh->RefSkeleton.FindBoneIndex(SocketName);
#else
	const int32 BoneIndex = SkeletalMesh->GetRefSkeleton().FindBoneIndex(SocketName);
#endif
	if (BoneIndex != INDEX_NONE)
	{
		return Builder.GetOrAddNode(RootNode, SkeletalMesh, BoneIndex);
	}

	// TODO: report error
	return FGLTFJsonNodeIndex(INDEX_NONE);
}

FGLTFJsonNodeIndex FGLTFSkeletalBoneConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, int32 BoneIndex)
{
	// TODO: add support for MasterPoseComponent?

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const TArray<FMeshBoneInfo>& BoneInfos = SkeletalMesh->RefSkeleton.GetRefBoneInfo();
#else
	const TArray<FMeshBoneInfo>& BoneInfos = SkeletalMesh->GetRefSkeleton().GetRefBoneInfo();
#endif
	if (!BoneInfos.IsValidIndex(BoneIndex))
	{
		// TODO: report error
		return FGLTFJsonNodeIndex(INDEX_NONE);
	}

	const FMeshBoneInfo& BoneInfo = BoneInfos[BoneIndex];

	FGLTFJsonNode Node;
	Node.Name = BoneInfo.Name.ToString();

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const TArray<FTransform>& BonePoses = SkeletalMesh->RefSkeleton.GetRefBonePose();
#else
	const TArray<FTransform>& BonePoses = SkeletalMesh->GetRefSkeleton().GetRefBonePose();
#endif
	if (BonePoses.IsValidIndex(BoneIndex))
	{
		// TODO: add warning check for non-uniform scaling
		const FTransform& BonePose = BonePoses[BoneIndex];
		Node.Translation = FGLTFConverterUtility::ConvertPosition(BonePose.GetTranslation(), Builder.ExportOptions->ExportUniformScale);
		Node.Rotation = FGLTFConverterUtility::ConvertRotation(BonePose.GetRotation());
		Node.Scale = FGLTFConverterUtility::ConvertScale(BonePose.GetScale3D());
	}
	else
	{
		// TODO: report error
	}

	const FGLTFJsonNodeIndex ParentNode = BoneInfo.ParentIndex != INDEX_NONE ? Builder.GetOrAddNode(RootNode, SkeletalMesh, BoneInfo.ParentIndex) : RootNode;
	return Builder.AddChildNode(ParentNode, Node);
}
