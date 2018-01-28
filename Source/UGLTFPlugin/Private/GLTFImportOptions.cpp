// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "GLTFImportOptions.h"
#include "UnrealType.h"

UGLTFImportOptions::UGLTFImportOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MeshImportType = EGltfMeshImportType::StaticMesh;
	bApplyWorldTransformToGeometry = true;
	bImportMaterials = true;
}

void UGLTFImportOptions::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		SaveConfig();
	}
}

UGLTFSceneImportOptions::UGLTFSceneImportOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bFlattenHierarchy = true;
	bImportMeshes = true;
	PathForAssets.Path = TEXT("/Game");
	bGenerateUniquePathPerMesh = true;
	bApplyWorldTransformToGeometry = true;
	bImportMaterials = true;
}

void UGLTFSceneImportOptions::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UGLTFSceneImportOptions::CanEditChange(const UProperty* InProperty) const
{
	bool bCanEdit = Super::CanEditChange(InProperty);

	FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;

	if (GET_MEMBER_NAME_CHECKED(UGLTFImportOptions, MeshImportType) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions, bApplyWorldTransformToGeometry) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions, bGenerateUniquePathPerMesh) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions, bImportMaterials) == PropertyName)
	{
		bCanEdit &= bImportMeshes;
	}

	return bCanEdit;
}

