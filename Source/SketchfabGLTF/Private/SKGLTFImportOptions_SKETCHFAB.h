// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "Factories/MaterialImportHelpers.h"
#include "SKGLTFImportOptions_SKETCHFAB.generated.h"


UENUM()
enum class ESKGLTFMeshImportType : uint8
{
	StaticMesh,
};

UCLASS(config = EditorPerProjectUserSettings)
class USKGLTFImportOptions_SKETCHFAB : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	/** Defines what should happen with existing actors */
	//UPROPERTY(config, EditAnywhere, Category=Mesh)
	ESKGLTFMeshImportType MeshImportType;

	/**
	 * If unchecked, to enforce unique asset paths, all assets will be created in directories that match with their mesh path
	 * e.g a GLTF node /scene/node/mymesh will generate the path in the game directory "/Game/myassets/" with a mesh asset called "mymesh" within that path.
	 * Can be useful for debugging purposes or tweaking individual meshes
	 */
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bMergeMeshes;

	/*
	* Uncheck to ignore mesh transformations (translation, rotation and scale) upon import
	* Can be useful to use mesh primitives individually or import assets packs for isntance
	*/
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bApplyWorldTransform;

	UPROPERTY(config, EditAnywhere, Category = Mesh)
	bool bImportMaterials;

	/** If checked, static meshes, materials and textures will be imported into a new folder */
	UPROPERTY(config, EditAnywhere, Category = Mesh)
	bool bImportInNewFolder;

	//UPROPERTY(config, EditAnywhere, Category = Mesh)
	//bool bImportTextures;

	//UPROPERTY(EditAnywhere, config, Category="Mesh|Materials")
	//EMaterialSearchLocation MaterialSearchLocation;

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
};

UCLASS(config = EditorPerProjectUserSettings)
class USKGLTFSceneImportOptions : public USKGLTFImportOptions_SKETCHFAB
{
	GENERATED_UCLASS_BODY()
public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
public:
	/** If checked, all actors generated will have a world space transform and will not have any attachment hierarchy */
	UPROPERTY(config, EditAnywhere, Category=General)
	bool bFlattenHierarchy;

	/** Whether or not to import custom properties and set their unreal equivalent on spawned actors */
	UPROPERTY(config, EditAnywhere, Category = General)
	bool bImportProperties;

	/** Whether or not to import mesh geometry or to just spawn actors using existing meshes */
	UPROPERTY(config, EditAnywhere, Category=Mesh)
	bool bImportMeshes;

	/** The path where new assets are imported */
	UPROPERTY(config, EditAnywhere, Category=Mesh, meta=(ContentDir, EditCondition = bImportMeshes))
	FDirectoryPath PathForAssets;
};

