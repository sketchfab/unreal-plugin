// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/GLTFAnimSequenceExporter.h"
#include "Exporters/GLTFExporterUtility.h"
#include "Builders/GLTFContainerBuilder.h"
#include "Animation/AnimSequence.h"

UGLTFAnimSequenceExporter::UGLTFAnimSequenceExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAnimSequence::StaticClass();
}

bool UGLTFAnimSequenceExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const UAnimSequence* AnimSequence = CastChecked<UAnimSequence>(Object);

	if (!Builder.ExportOptions->bExportAnimationSequences)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export animation sequence %s because animation sequences are disabled by export options"),
			*AnimSequence->GetName()));
		return false;
	}

	if (!Builder.ExportOptions->bExportVertexSkinWeights)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export animation sequence %s because vertex skin weights are disabled by export options"),
			*AnimSequence->GetName()));
		return false;
	}

	const USkeletalMesh* SkeletalMesh = FGLTFExporterUtility::GetPreviewMesh(AnimSequence);
	if (SkeletalMesh == nullptr)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export animation sequence %s because of missing preview mesh"),
			*AnimSequence->GetName()));
		return false;
	}

	FGLTFJsonNode Node;

	if (Builder.ExportOptions->bExportPreviewMesh)
	{
		const FGLTFJsonMeshIndex MeshIndex = Builder.GetOrAddMesh(SkeletalMesh);
		if (MeshIndex == INDEX_NONE)
		{
			Builder.AddErrorMessage(
				FString::Printf(TEXT("Failed to export skeletal mesh %s for animation sequence %s"),
				*SkeletalMesh->GetName(),
				*AnimSequence->GetName()));
			return false;
		}

		Node.Mesh = MeshIndex;
	}

	const FGLTFJsonNodeIndex NodeIndex = Builder.AddNode(Node);

	const FGLTFJsonSkinIndex SkinIndex = Builder.GetOrAddSkin(NodeIndex, SkeletalMesh);
	if (SkinIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export bones in skeletal mesh %s for animation sequence %s"),
			*SkeletalMesh->GetName(),
			*AnimSequence->GetName()));
		return false;
	}

	Builder.GetNode(NodeIndex).Skin = SkinIndex;

	const FGLTFJsonAnimationIndex AnimationIndex =  Builder.GetOrAddAnimation(NodeIndex, SkeletalMesh, AnimSequence);
	if (AnimationIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export animation sequence %s"),
			*AnimSequence->GetName()));
		return false;
	}

	FGLTFJsonScene Scene;
	Scene.Nodes.Add(NodeIndex);
	const FGLTFJsonSceneIndex SceneIndex = Builder.AddScene(Scene);

	Builder.DefaultScene = SceneIndex;
	return true;
}
