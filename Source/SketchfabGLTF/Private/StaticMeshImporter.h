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
	static unsigned char* GetAccessorData(const tinygltf::Model* model, const tinygltf::Primitive& prim, tinygltf::Accessor*& accessor, const std::string attributeName);
	static bool AddUVs(const tinygltf::Model *model, const tinygltf::Primitive &prim, FRawMesh &RawTriangles, int32 NumFaces, int32 WedgeOffset, int32 VertexOffset, int32 uvIndex);

};