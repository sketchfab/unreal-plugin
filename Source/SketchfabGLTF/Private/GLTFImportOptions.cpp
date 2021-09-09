// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFImportOptions_SKETCHFAB.h"
#include "UObject/UnrealType.h"

USKGLTFImportOptions_SKETCHFAB::USKGLTFImportOptions_SKETCHFAB(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MeshImportType = ESKGLTFMeshImportType::StaticMesh;
	bApplyWorldTransformToGeometry = true;
	bImportMaterials = true;
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
	bGenerateUniquePathPerMesh = true;
	bApplyWorldTransformToGeometry = true;
	bImportMaterials = true;
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
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bApplyWorldTransformToGeometry) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bGenerateUniquePathPerMesh) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(USKGLTFImportOptions_SKETCHFAB, bImportMaterials) == PropertyName)
	{
		bCanEdit &= bImportMeshes;
	}

	return bCanEdit;
}

