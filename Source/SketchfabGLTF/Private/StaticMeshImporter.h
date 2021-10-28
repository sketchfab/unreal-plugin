// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

class UStaticMesh;
struct FGLTFImportContext;
struct FGLTFPrimToImport;
struct FRawMesh;

class FGLTFStaticMeshImporter
{
public:
	static UStaticMesh* ImportStaticMesh(FGLTFImportContext& ImportContext, const FGLTFPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh = nullptr);
	static void commitRawMesh(UStaticMesh* ImportedMesh, FRawMesh& RawTriangles);
};