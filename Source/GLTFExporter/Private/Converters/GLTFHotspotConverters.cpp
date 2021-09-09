// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFHotspotConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Animation/SkeletalMeshActor.h"

FGLTFJsonHotspotIndex FGLTFHotspotConverter::Convert(const ASKGLTFHotspotActor* HotspotActor)
{
	FGLTFJsonHotspot JsonHotspot;
	HotspotActor->GetName(JsonHotspot.Name);

	if (const ASkeletalMeshActor* SkeletalMeshActor = HotspotActor->SkeletalMeshActor)
	{
		if (!Builder.ExportOptions->bExportVertexSkinWeights)
		{
			Builder.AddWarningMessage(
				FString::Printf(TEXT("Can't export animation in hotspot %s because vertex skin weights are disabled by export options"),
				*JsonHotspot.Name));
		}
		else if (!Builder.ExportOptions->bExportAnimationSequences)
		{
			Builder.AddWarningMessage(
				FString::Printf(TEXT("Can't export animation in hotspot %s because animation sequences are disabled by export options"),
				*JsonHotspot.Name));
		}
		else
		{
			const FGLTFJsonNodeIndex RootNode = Builder.GetOrAddNode(SkeletalMeshActor);

			const USkeletalMeshComponent* SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent();
			if (const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh)
			{
				if (const UAnimSequence* AnimSequence = HotspotActor->AnimationSequence)
				{
					JsonHotspot.Animation = Builder.GetOrAddAnimation(RootNode, SkeletalMesh, AnimSequence);
				}
				else
				{
					// TODO: report warning
				}
			}
			else
			{
				// TODO: report warning
			}
		}
	}
	else if (const ULevelSequence* LevelSequence = HotspotActor->LevelSequence)
	{
		if (!Builder.ExportOptions->bExportLevelSequences)
		{
			Builder.AddWarningMessage(
				FString::Printf(TEXT("Can't export animation in hotspot %s because level sequences are disabled by export options"),
				*JsonHotspot.Name));
		}
		else
		{
			JsonHotspot.Animation = Builder.GetOrAddAnimation(HotspotActor->GetLevel(), LevelSequence);
		}
	}
	else
	{
		// TODO: report warning
	}

	JsonHotspot.Image = Builder.GetOrAddTexture(HotspotActor->GetImageForState(ESKGLTFHotspotState::Default));
	JsonHotspot.HoveredImage = Builder.GetOrAddTexture(HotspotActor->GetImageForState(ESKGLTFHotspotState::Hovered));
	JsonHotspot.ToggledImage = Builder.GetOrAddTexture(HotspotActor->GetImageForState(ESKGLTFHotspotState::Toggled));
	JsonHotspot.ToggledHoveredImage = Builder.GetOrAddTexture(HotspotActor->GetImageForState(ESKGLTFHotspotState::ToggledHovered));

	return Builder.AddHotspot(JsonHotspot);
}
