// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once 

class UStaticMesh;
struct FGltfImportContext;
struct FGltfPrimToImport;
struct FRawMesh;

class FGLTFStaticMeshImporter
{
public:
	static UStaticMesh* ImportStaticMesh(FGltfImportContext& ImportContext, const FGltfPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh = nullptr);
	static bool AddUVs(const tinygltf::Model *model, const tinygltf::Primitive &prim, FRawMesh &RawTriangles, int32 NumFaces, int32 WedgeOffset, int32 VertexOffset, int32 uvIndex);

};