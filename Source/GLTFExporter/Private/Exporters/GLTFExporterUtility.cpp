// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/SKGLTFExporterUtility.h"
#include "AssetRegistryModule.h"
#include "LevelSequence.h"
#include "UnrealEd.h"

const UStaticMesh* FGLTFExporterUtility::GetPreviewMesh(const UMaterialInterface* Material)
{
	// The following implementation is based of FMaterialInstanceEditor::RefreshPreviewAsset

	const UStaticMesh* PreviewMesh = Cast<UStaticMesh>(Material->PreviewMesh.TryLoad());
	if (PreviewMesh == nullptr)
	{
		// Attempt to use the parent material's preview mesh if the instance's preview mesh is invalid
		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const UMaterialInterface* ParentMaterial = MaterialInstance->Parent;
			if (ParentMaterial != nullptr)
			{
				PreviewMesh = Cast<UStaticMesh>(ParentMaterial->PreviewMesh.TryLoad());
			}
		}
	}

	if (PreviewMesh == nullptr)
	{
		// Use a default sphere instead if the parent material's preview mesh is also invalid
		PreviewMesh = GUnrealEd->GetThumbnailManager()->EditorSphere;
	}

	return PreviewMesh;
}

const USkeletalMesh* FGLTFExporterUtility::GetPreviewMesh(const UAnimSequence* AnimSequence)
{
	const USkeletalMesh* PreviewMesh = AnimSequence->GetPreviewMesh();
	if (PreviewMesh == nullptr)
	{
		const USkeleton* Skeleton = AnimSequence->GetSkeleton();
		if (Skeleton != nullptr)
		{
			PreviewMesh = Skeleton->GetPreviewMesh();
		}
	}

	return PreviewMesh;
}

TArray<UWorld*> FGLTFExporterUtility::GetReferencedWorlds(const UObject* Object)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FName OuterPathName = *Object->GetOutermost()->GetPathName();

	TArray<UWorld*> ReferencedWorlds;
	TArray<FAssetIdentifier> AssetDependencies;
	AssetRegistryModule.Get().GetDependencies(OuterPathName, AssetDependencies);

	for (FAssetIdentifier& AssetDependency : AssetDependencies)
	{
		FString PackageName = AssetDependency.PackageName.ToString();
		if (FEditorFileUtils::IsMapPackageAsset(PackageName))
		{
			UWorld* World = LoadObject<UWorld>(nullptr, *PackageName);
			if (World != nullptr)
			{
				ReferencedWorlds.AddUnique(World);
			}
		}
	}

	return ReferencedWorlds;
}
