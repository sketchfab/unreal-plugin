// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFImportOptions_SKETCHFAB.h"
#include "UObject/UnrealType.h"

USKGLTFImportOptions_SKETCHFAB::USKGLTFImportOptions_SKETCHFAB(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MeshImportType = ESKGLTFMeshImportType::StaticMesh;
	bApplyWorldTransform = true;
	bImportMaterials = true;
	bMergeMeshes = true;
	bImportInNewFolder = false;
}

void USKGLTFImportOptions_SKETCHFAB::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		SaveConfig();
	}
}

USKGLTFSceneImportOptions::USKGLTFSceneImportOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bFlattenHierarchy = true;
	bImportMeshes = true;
	PathForAssets.Path = TEXT("/Game");
	bMergeMeshes = true;
	bApplyWorldTransform = true;
	bImportMaterials = true;
	bImportInNewFolder = false;
}

void USKGLTFSceneImportOptions::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool USKGLTFSceneImportOptions::CanEditChange(const FProperty* InProperty) const
{
	bool bCanEdit = Super::CanEditChange(InProperty);

	FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;

	if (GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, MeshImportType) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bApplyWorldTransform) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bMergeMeshes) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bImportMaterials) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bImportInNewFolder) == PropertyName)
	{
		bCanEdit &= bImportMeshes;
	}

	return bCanEdit;
}

