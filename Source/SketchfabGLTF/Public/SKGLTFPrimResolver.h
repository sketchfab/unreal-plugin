// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Templates/SubclassOf.h"

THIRD_PARTY_INCLUDES_START
#include "tiny_gltf.h"
THIRD_PARTY_INCLUDES_END

#include "SKGLTFPrimResolver.generated.h"

class IAssetRegistry;
class UActorFactory;
struct FGLTFImportContext;
struct FGltfGeomData;
struct FGLTFSceneImportContext;

struct FGLTFPrimToImport
{
	FGLTFPrimToImport()
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
class USKGLTFPrimResolver : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init();
	virtual void FindPrimsToImport(FGLTFImportContext& ImportContext, TArray<FGLTFPrimToImport>& OutPrimsToImport);

protected:
	void FindPrimsToImport_Recursive(FGLTFImportContext& ImportContext, tinygltf::Node* Prim, TArray<FGLTFPrimToImport>& OutTopLevelPrims, FMatrix ParentMat);
	bool IsValidPathForImporting(const FString& TestPath) const;

protected:
	IAssetRegistry* AssetRegistry;
};