// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

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

/** Base class for all evaluation of prims for geometry and actors */
UCLASS(transient, MinimalAPI)
class UGLTFPrimResolver : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init();
	virtual void FindPrimsToImport(FGltfImportContext& ImportContext, TArray<FGltfPrimToImport>& OutPrimsToImport);

protected:
	void FindPrimsToImport_Recursive(FGltfImportContext& ImportContext, tinygltf::Node* Prim, TArray<FGltfPrimToImport>& OutTopLevelPrims, FMatrix ParentMat);
	bool IsValidPathForImporting(const FString& TestPath) const;

protected:
	IAssetRegistry* AssetRegistry;
};