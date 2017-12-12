// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "SubclassOf.h"

THIRD_PARTY_INCLUDES_START
#include "tiny_gltf.h"
THIRD_PARTY_INCLUDES_END

#include "GLTFPrimResolver.generated.h"

class IAssetRegistry;
class UActorFactory;
struct FGltfImportContext;
struct FGltfGeomData;
struct FGLTFSceneImportContext;

struct FGltfPrimToImport
{
	FGltfPrimToImport()
		: Prim(nullptr)
		, NumLODs(0)
		, LocalPrimTransform(FMatrix::Identity)
		, WorldPrimTransform(FMatrix::Identity)
	{}

	tinygltf::Node* Prim;
	int32 NumLODs;
	FMatrix LocalPrimTransform;
	FMatrix WorldPrimTransform;

	const FGltfGeomData* GetGeomData(int32 LODIndex, double Time) const;
};

struct FActorSpawnData
{
	FActorSpawnData()
		: ActorPrim(nullptr)
		, AttachParentPrim(nullptr)
		, MeshPrim(nullptr)
	{}

	FMatrix WorldTransform;
	/** The prim that represents this actor */
	tinygltf::Node* ActorPrim;
	/** The prim that represents the parent of this actor for attachment (not necessarily the parent of this prim) */
	tinygltf::Node* AttachParentPrim;
	/** The prim that represents the mesh to import and apply to this actor */
	tinygltf::Node* MeshPrim;
	FString ActorClassName;
	FString AssetPath;
	FName ActorName;
};

/** Base class for all evaluation of prims for geometry and actors */
UCLASS(transient, MinimalAPI)
class UGLTFPrimResolver : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init();

	virtual void FindPrimsToImport(FGltfImportContext& ImportContext, TArray<FGltfPrimToImport>& OutPrimsToImport);

	virtual void FindActorsToSpawn(FGLTFSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnDatas);

	virtual AActor* SpawnActor(FGLTFSceneImportContext& ImportContext, const FActorSpawnData& SpawnData);

	virtual TSubclassOf<AActor> FindActorClass(FGLTFSceneImportContext& ImportContext, const FActorSpawnData& SpawnData) const;

protected:
	void FindPrimsToImport_Recursive(FGltfImportContext& ImportContext, tinygltf::Node* Prim, TArray<FGltfPrimToImport>& OutTopLevelPrims, FMatrix ParentMat);
	virtual void FindActorsToSpawn_Recursive(FGLTFSceneImportContext& ImportContext, tinygltf::Node* Prim, tinygltf::Node* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas);
	bool IsValidPathForImporting(const FString& TestPath) const;
protected:
	IAssetRegistry* AssetRegistry;
	TMap<tinygltf::Node*, AActor*> PrimToActorMap;
};