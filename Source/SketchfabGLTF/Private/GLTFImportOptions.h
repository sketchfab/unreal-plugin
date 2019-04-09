// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Factories/MaterialImportHelpers.h"
#include "GLTFImportOptions.generated.h"


UENUM()
enum class EExistingActorPolicy : uint8
{
	/** Replaces existing actors with new ones */
	Replace,
	/** Update transforms on existing actors but do not replace actor the actor class or any other data */
	UpdateTransform,
	/** Ignore any existing actor with the same name */
	Ignore,

};

UENUM()
enum class EExistingAssetPolicy : uint8
{
	/** Reimports existing assets */
	Reimport,

	/** Ignores existing assets and doesnt reimport them */
	Ignore,
};

UENUM()
enum class EGLTFMeshImportType : uint8
{
	StaticMesh,
};

UCLASS(config = EditorPerProjectUserSettings)
class UGLTFImportOptions : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	/** Defines what should happen with existing actors */
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	EGLTFMeshImportType MeshImportType;

	/**
	 * If checked, To enforce unique asset paths, all assets will be created in directories that match with their mesh path
	 * e.g a GLTF node /scene/node/mymesh will generate the path in the game directory "/Game/myassets/" with a mesh asset called "mymesh" within that path.
	 */
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bGenerateUniquePathPerMesh;

	//UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bApplyWorldTransformToGeometry;

	UPROPERTY(config, EditAnywhere, Category = Mesh)
	bool bImportMaterials;

	//UPROPERTY(config, EditAnywhere, Category = Mesh)
	//bool bImportTextures;

	//UPROPERTY(EditAnywhere, config, Category="Mesh|Materials")
	//EMaterialSearchLocation MaterialSearchLocation;

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
};

UCLASS(config = EditorPerProjectUserSettings)
class UGLTFSceneImportOptions : public UGLTFImportOptions
{
	GENERATED_UCLASS_BODY()
public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif
public:
	/** If checked, all actors generated will have a world space transform and will not have any attachment hierarchy */
	UPROPERTY(config, EditAnywhere, Category=General)
	bool bFlattenHierarchy;

	/** Defines what should happen with existing actors */
	UPROPERTY(config, EditAnywhere, Category=General)
	EExistingActorPolicy ExistingActorPolicy;

	/** Whether or not to import custom properties and set their unreal equivalent on spawned actors */
	UPROPERTY(config, EditAnywhere, Category = General)
	bool bImportProperties;

	/** Whether or not to import mesh geometry or to just spawn actors using existing meshes */
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bImportMeshes;

	/** The path where new assets are imported */
	UPROPERTY(config, EditAnywhere, Category=Mesh, meta=(ContentDir, EditCondition = bImportMeshes))
	FDirectoryPath PathForAssets;

	/** What should happen with existing assets */
	UPROPERTY(config, EditAnywhere, Category=Mesh, meta = (EditCondition=bImportMeshes))
	EExistingAssetPolicy ExistingAssetPolicy;
};

