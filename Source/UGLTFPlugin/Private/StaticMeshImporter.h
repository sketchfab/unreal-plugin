// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once 

class UStaticMesh;
struct FGltfImportContext;
struct FGltfPrimToImport;
struct FRawMesh;

class FGLTFStaticMeshImporter
{
public:
	static UStaticMesh* ImportStaticMesh(FGltfImportContext& ImportContext, const FGltfPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh = nullptr);
};