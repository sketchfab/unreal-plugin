// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "GLTFImportOptions_SKETCHFAB.h"
#include "UObject/UnrealType.h"

UGLTFImportOptions_SKETCHFAB::UGLTFImportOptions_SKETCHFAB(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MeshImportType = EGLTFMeshImportType::StaticMesh;
	bApplyWorldTransformToGeometry = true;
	bImportMaterials = true;
}

void UGLTFImportOptions_SKETCHFAB::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
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

bool UGLTFSceneImportOptions::CanEditChange(const FProperty* InProperty) const
{
	bool bCanEdit = Super::CanEditChange(InProperty);

	FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;

	if (GET_MEMBER_NAME_CHECKED(UGLTFImportOptions_SKETCHFAB, MeshImportType) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions_SKETCHFAB, bApplyWorldTransformToGeometry) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions_SKETCHFAB, bGenerateUniquePathPerMesh) == PropertyName ||
		GET_MEMBER_NAME_CHECKED(UGLTFImportOptions_SKETCHFAB, bImportMaterials) == PropertyName)
	{
		bCanEdit &= bImportMeshes;
	}

	return bCanEdit;
}

