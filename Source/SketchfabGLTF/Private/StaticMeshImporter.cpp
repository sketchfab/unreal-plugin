// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "StaticMeshImporter.h"
#include "SKGLTFImporter.h"
#include "SKGLTFConversionUtils.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

unsigned char* GetAccessorData(const tinygltf::Model* model, const tinygltf::Primitive& prim, tinygltf::Accessor*& accessor, const std::string attributeName) {
	accessor = nullptr;
	int accessorIndex;
	if (!attributeName.empty()) {
		const auto& attribute = prim.attributes.find(attributeName);
		if (attribute != prim.attributes.end())
		{
			accessorIndex = attribute->second;
		}
		else
		{
			return nullptr;
		}
	}
	else {
		accessorIndex = prim.indices;
	}
	if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
	{
		accessor = const_cast<tinygltf::Accessor*>(&model->accessors[accessorIndex]);
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor->bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		size_t offset = accessor->byteOffset + bufferView.byteOffset;
		if (accessor->count > 0)
		{
			return const_cast<unsigned char*>(&buffer.data[offset]);
		}
	}
	return nullptr;
}

template<typename T>
void parseIndicesData(unsigned char* rawData, int dataSize, FRawMesh& RawTriangles, int32 VertexOffset, int32 WedgeOffset)
{
	T* data = (T*)rawData;
	for (int i = 0; i < dataSize; i++)
	{
		RawTriangles.WedgeIndices[WedgeOffset + i] = VertexOffset + data[i];
	}
}

template<typename T>
void parsePositionData(unsigned char* rawData, int dataSize, FRawMesh& RawTriangles, int32 VertexOffset, FTransform FinalTransform)
{
	T* data = (T*)rawData;
	for (int i = 0; i < dataSize; i++)
	{
		int index = i * 3;
		FVector Pos = FVector(data[index], data[index + 1], data[index + 2]);
		Pos = FinalTransform.TransformPosition(Pos);
		RawTriangles.VertexPositions[VertexOffset + i] = Pos;
	}
}

template<typename T>
void parseNormalData(unsigned char* rawData, int32 NumFaces, FRawMesh& RawTriangles, int32 VertexOffset, int32 WedgeOffset, int32 TangentZOffset, FMatrix FinalTransformIT)
{
	T* data = (T*)rawData;
	for (int FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
	{
		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
			const int32 NormalIdx = TangentZOffset + FaceIdx * 3 + CornerIdx;
			int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 3;
			FVector Normal = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
			FVector TransformedNormal = FinalTransformIT.TransformVector(Normal);
			RawTriangles.WedgeTangentZ[NormalIdx] = TransformedNormal.GetSafeNormal();
		}
	}
}

template<typename T>
void parseTangentData(unsigned char* rawData, int32 NumFaces, FRawMesh& RawTriangles, int32 VertexOffset, int32 WedgeOffset, int32 TangentXOffset, bool fromVec3, FMatrix FinalTransformIT)
{
	float* data = (float*)rawData;
	for (int FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
	{
		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
			const int32 TangentXIdx = TangentXOffset + FaceIdx * 3 + CornerIdx;
			int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 3;
			FVector Tangent = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
			FVector TransformedTangent = FinalTransformIT.TransformVector(Tangent);

			if (fromVec3)
				RawTriangles.WedgeTangentX[WedgeIdx] = TransformedTangent.GetSafeNormal();
			else
				RawTriangles.WedgeTangentX[TangentXIdx] = TransformedTangent.GetSafeNormal();
		}
	}
}

template<typename T>
void parseColorData(unsigned char* rawData, int32 NumFaces, FRawMesh& RawTriangles, int32 VertexOffset, int32 WedgeOffset, int32 ColorOffset, int NumComponents)
{
	int32 WedgesCount = RawTriangles.WedgeIndices.Num();
	// There might be cases in which no map was previously parsed, but now it is.
	// In that case we add zeros UVs for the previous unparsed UVs
	if (RawTriangles.WedgeColors.Num() != WedgesCount)
	{
		RawTriangles.WedgeColors.AddDefaulted(WedgesCount - RawTriangles.WedgeColors.Num());
	}

	T* data = (T*)rawData;
	for (int FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
	{
		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
			const int32 ColorIdx = ColorOffset + FaceIdx * 3 + CornerIdx;
			int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * NumComponents;
			RawTriangles.WedgeColors[ColorIdx] = FLinearColor(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]).ToFColor(false);
		}
	}
}

template<typename T>
void parseUVs(FRawMesh& RawTriangles, const unsigned char* rawData, const tinygltf::Accessor* accessor, int32 NumFaces, int32 WedgeOffset, int32 VertexOffset, int32 uvIndex)
{
	TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[uvIndex];
	int32 WedgesCount = RawTriangles.WedgeIndices.Num();

	// There might be cases in which no map was previously parsed, but now it is.
	// In that case we add zeros UVs for the previous unparsed UVs
	if (TexCoords.Num() != WedgesCount)
	{
		TexCoords.AddZeroed(WedgesCount - TexCoords.Num());
	}
	int32 TexCoordsCount = TexCoords.Num();

	T* data = (T*)rawData;
	for (int FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
	{
		const int32 I0 = WedgeOffset + FaceIdx * 3 + 0;
		const int32 I1 = WedgeOffset + FaceIdx * 3 + 1;
		const int32 I2 = WedgeOffset + FaceIdx * 3 + 2;

		if (I0 >= WedgesCount || I1 >= WedgesCount || I2 >= WedgesCount)
		{
			ensure(false);
			break;
		}

		if (I0 >= TexCoordsCount || I1 >= TexCoordsCount || I2 >= TexCoordsCount)
		{
			ensure(false);
			break;
		}

		int32 index0 = RawTriangles.WedgeIndices[I0] - VertexOffset;
		int32 index1 = RawTriangles.WedgeIndices[I1] - VertexOffset;
		int32 index2 = RawTriangles.WedgeIndices[I2] - VertexOffset;

		if (index0 >= accessor->count || index1 >= accessor->count || index2 >= accessor->count)
			break;

		index0 *= 2;
		index1 *= 2;
		index2 *= 2;

		TexCoords[I0] = FVector2D(data[index0], data[index0 + 1]);
		TexCoords[I1] = FVector2D(data[index1], data[index1 + 1]);
		TexCoords[I2] = FVector2D(data[index2], data[index2 + 1]);
	}
}

bool AddUVs(const tinygltf::Model* model, const tinygltf::Primitive& prim, FRawMesh& RawTriangles, int32 NumFaces, int32 WedgeOffset, int32 VertexOffset, int32 uvIndex)
{
	bool uvsAdded = false;

	tinygltf::Accessor* accessor = nullptr;
	unsigned char* rawData;

	rawData = GetAccessorData(model, prim, accessor, "TEXCOORD_" + std::to_string(uvIndex));
	if (rawData && accessor)
	{
		ensure(accessor->type == TINYGLTF_TYPE_VEC2);
		switch (accessor->componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:  parseUVs<float>(RawTriangles, rawData, accessor, NumFaces, WedgeOffset, VertexOffset, uvIndex); uvsAdded = true;  break;
		case TINYGLTF_COMPONENT_TYPE_DOUBLE: parseUVs<double>(RawTriangles, rawData, accessor, NumFaces, WedgeOffset, VertexOffset, uvIndex); uvsAdded = true; break;
		default: ensure(false); break;
		}
	}

	return uvsAdded;
}

UStaticMesh* FGLTFStaticMeshImporter::ImportStaticMesh(FGLTFImportContext& ImportContext, const FGLTFPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh)
{
	const FTransform& ConversionTransform = ImportContext.ConversionTransform;
	const FMatrix PrimToWorld = ImportContext.bApplyWorldTransform ? PrimToImport.WorldPrimTransform : FMatrix::Identity;

	FTransform FinalTransform = FTransform(PrimToWorld)*ConversionTransform;
	FMatrix FinalTransformIT = FinalTransform.ToInverseMatrixWithScale().GetTransposed();

	const bool bFlip = FinalTransform.GetDeterminant() > 0.0f;

	UStaticMesh* ImportedMesh = nullptr;

	if (singleMesh)
	{
		ImportedMesh = singleMesh;
	}
	else
	{
		ImportedMesh = GLTFUtils::FindOrCreateObject<UStaticMesh>(ImportContext.Parent, ImportContext.ObjectName, ImportContext.ImportObjectFlags);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
		ImportedMesh->StaticMaterials.Empty();
#else
		ImportedMesh->GetStaticMaterials().Empty(); 
#endif
		
		ImportContext.MaterialMap.Empty();
	}

	ensure(ImportedMesh);


	int32 LODIndex = 0;

	const auto &model = ImportContext.Model;
	const auto &primitives = model->meshes[PrimToImport.Prim->mesh].primitives;

	int32 primCount = primitives.size();
	int32 currentFaceCount = 0;
	for (int32 primIdx = 0; primIdx < primCount; primIdx++)
	{
		const auto &prim = primitives[primIdx];

		if (prim.mode != TINYGLTF_MODE_TRIANGLES)
			continue;

		int32 VertexOffset = RawTriangles.VertexPositions.Num();
		int32 WedgeOffset = RawTriangles.WedgeIndices.Num();
		int32 FaceOffset = RawTriangles.FaceMaterialIndices.Num();
		int32 ColorOffset = RawTriangles.WedgeColors.Num();
		int32 TangentXOffset = RawTriangles.WedgeTangentX.Num();
		int32 TangentYOffset = RawTriangles.WedgeTangentY.Num();
		int32 TangentZOffset = RawTriangles.WedgeTangentZ.Num();
		int32 NumFaces = 0;

		tinygltf::Accessor* accessor = nullptr;
		unsigned char* rawData;

		// Indices
		rawData = GetAccessorData(model, prim, accessor, "");
		if (rawData && accessor)
		{
			NumFaces = accessor->count / 3;
			ensure(accessor->type == TINYGLTF_TYPE_SCALAR);
			if (accessor->type == TINYGLTF_TYPE_SCALAR)
			{
				RawTriangles.WedgeIndices.AddZeroed(accessor->count);
				switch (accessor->componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: parseIndicesData<unsigned short>(rawData, accessor->count, RawTriangles, VertexOffset, WedgeOffset); break;
				case TINYGLTF_COMPONENT_TYPE_INT:            parseIndicesData<int32>(rawData, accessor->count, RawTriangles, VertexOffset, WedgeOffset); break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   parseIndicesData<uint32>(rawData, accessor->count, RawTriangles, VertexOffset, WedgeOffset); break;
				default: ensure(false); break;
				}
			}
		}

		rawData = GetAccessorData(model, prim, accessor, "POSITION");
		if (rawData && accessor)
		{
			ensure(accessor->type == TINYGLTF_TYPE_VEC3);
			if (accessor->type == TINYGLTF_TYPE_VEC3)
			{
				RawTriangles.VertexPositions.AddZeroed(accessor->count);
				switch (accessor->componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:  parsePositionData<float>(rawData, accessor->count, RawTriangles, VertexOffset, FinalTransform); break;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE: parsePositionData<double>(rawData, accessor->count, RawTriangles, VertexOffset, FinalTransform); break;
				default: ensure(false); break;
				}
			}
		}

		rawData = GetAccessorData(model, prim, accessor, "COLOR_0");
		if (rawData && accessor)
		{
			RawTriangles.WedgeColors.AddUninitialized(NumFaces * 3);
			ensure(accessor->type == TINYGLTF_TYPE_VEC4 || accessor->type == TINYGLTF_TYPE_VEC3);
			if (accessor->type == TINYGLTF_TYPE_VEC4 || accessor->type == TINYGLTF_TYPE_VEC3)
			{
				int NumComponents = (accessor->type == TINYGLTF_TYPE_VEC4) ? 4 : 3;
				switch (accessor->componentType)
				{
					case TINYGLTF_COMPONENT_TYPE_FLOAT:  parseColorData<float>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, ColorOffset, NumComponents); break;
					case TINYGLTF_COMPONENT_TYPE_DOUBLE: parseColorData<double>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, ColorOffset, NumComponents); break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: parseColorData<uint8>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, ColorOffset, NumComponents); break;
					default: ensure(false); break;
				}
			}
		}

		rawData = GetAccessorData(model, prim, accessor, "NORMAL");
		if (rawData && accessor && !ImportContext.disableNormals)
		{
			RawTriangles.WedgeTangentZ.AddUninitialized(NumFaces * 3);
			ensure(accessor->type == TINYGLTF_TYPE_VEC3);
			if (accessor->type == TINYGLTF_TYPE_VEC3)
			{
				switch (accessor->componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:  parseNormalData<float>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentZOffset, FinalTransformIT); break;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE: parseNormalData<double>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentZOffset, FinalTransformIT); break;
				default: ensure(false); break;
				}
			}
		}

		rawData = GetAccessorData(model, prim, accessor, "TANGENT");
		if (rawData && accessor && !ImportContext.disableTangents)
		{
			RawTriangles.WedgeTangentX.AddUninitialized(NumFaces * 3);
			switch (accessor->type)
			{
			case TINYGLTF_TYPE_VEC3:
			{
				switch (accessor->componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:  parseTangentData<float>( rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentXOffset, true, FinalTransformIT); break;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE: parseTangentData<double>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentXOffset, true, FinalTransformIT); break;
				default: ensure(false); break;
				}
			}
			break;
			case TINYGLTF_TYPE_VEC4:
			{
				switch (accessor->componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:  parseTangentData<float>( rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentXOffset, false, FinalTransformIT); break;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE: parseTangentData<double>(rawData, NumFaces, RawTriangles, VertexOffset, WedgeOffset, TangentXOffset, false, FinalTransformIT); break;
				default: ensure(false); break;
				}
			}
			break;
			default: ensure(false); break;
			}
		}

		bool uvsAdded = false;
		int32 maxUVcount = 0;
		for (int32 uv = 0; uv < MAX_MESH_TEXTURE_COORDS; uv++)
		{
			uvsAdded |= AddUVs(ImportContext.Model, prim, RawTriangles, NumFaces, WedgeOffset, VertexOffset, uv);
			maxUVcount = maxUVcount > RawTriangles.WedgeTexCoords[uv].Num() ? maxUVcount : RawTriangles.WedgeTexCoords[uv].Num();
		}

		if (!uvsAdded)
		{
			// Have to at least have one UV set
			ImportContext.AddErrorMessage(EMessageSeverity::Warning,
				FText::Format(LOCTEXT("StaticMeshesHaveNoUVS", "{0} (LOD {1}) has no UVs.  At least one valid UV set should exist on a static mesh. This mesh will likely have rendering issues"),
					FText::FromString(ImportContext.ObjectName),
					FText::AsNumber(0)));

			RawTriangles.WedgeTexCoords[0].AddZeroed(NumFaces * 3);
			maxUVcount = RawTriangles.WedgeTexCoords[0].Num();
		}

		// If there are multiple UV maps present, pad them with zeros to ensure they have the same size
		for (int32 uv = 0; uv < MAX_MESH_TEXTURE_COORDS; uv++)
		{
			int32 nUVs = RawTriangles.WedgeTexCoords[uv].Num();
			if ((nUVs != 0) && (nUVs != maxUVcount))
			{
				RawTriangles.WedgeTexCoords[uv].AddZeroed(maxUVcount - nUVs);
			}
		}
		// Do the same for vertex colors
		int32 nColors = RawTriangles.WedgeColors.Num();
		if ((nColors != 0) && (nColors != RawTriangles.WedgeIndices.Num()))
		{
			for (int i = 0; i < RawTriangles.WedgeIndices.Num() - nColors; i++)
			{
				RawTriangles.WedgeColors.Add(FColor::White);
			}
		}
		// Disable normals if not all primitives have them
		// This should never happen as Sketchfab model should always have normals
		if (!ImportContext.disableNormals && RawTriangles.WedgeTangentZ.Num() != RawTriangles.WedgeIndices.Num())
		{
			ImportContext.disableNormals = true;
			RawTriangles.WedgeTangentZ.Empty();
			UE_LOG(LogGLTFImport, Warning, TEXT("Not all primitives have normals, disabling normals"));
		}
		// Disable tangents if not all primitives have them
		if (!ImportContext.disableTangents && RawTriangles.WedgeTangentX.Num() != RawTriangles.WedgeIndices.Num())
		{
			ImportContext.disableTangents = true;
			RawTriangles.WedgeTangentX.Empty();
			UE_LOG(LogGLTFImport, Warning, TEXT("Not all primitives have tangents, disabling parsing"));
		}

		if (bFlip)
		{
			// Flip anything that is indexed
			for (int32 FaceIdx = 0; FaceIdx < NumFaces; ++FaceIdx)
			{
				int32 fidx = FaceIdx * 3;
				int32 I0 = WedgeOffset + fidx + 0;
				int32 I2 = WedgeOffset + fidx + 2;
				Swap(RawTriangles.WedgeIndices[I0], RawTriangles.WedgeIndices[I2]);

				for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_MESH_TEXTURE_COORDS; ++TexCoordIdx)
				{
					TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[TexCoordIdx];
					if (TexCoords.Num() > 0)
					{
						Swap(TexCoords[I0], TexCoords[I2]);
					}
				}

				if(!ImportContext.disableTangents && RawTriangles.WedgeTangentX.Num())
				{
					I0 = TangentXOffset + fidx + 0;
					I2 = TangentXOffset + fidx + 2;
					Swap(RawTriangles.WedgeTangentX[I0], RawTriangles.WedgeTangentX[I2]);
				}

				if(!ImportContext.disableNormals && RawTriangles.WedgeTangentZ.Num())
				{
					I0 = TangentZOffset + fidx + 0;
					I2 = TangentZOffset + fidx + 2;
					Swap(RawTriangles.WedgeTangentZ[I0], RawTriangles.WedgeTangentZ[I2]);
				}
			}
		}

		//Check to see if we have added this material already to the object.
		int* staticMaterialIndex = ImportContext.MaterialMap.Find(prim.material);
		if (!staticMaterialIndex)
		{
			//Add material
			UMaterialInterface* ExistingMaterial = nullptr;
			if (prim.material >= 0 && prim.material < ImportContext.Materials.Num())
			{
				ExistingMaterial = ImportContext.Materials[prim.material];
			}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
			int32 FinalIndex = ImportedMesh->StaticMaterials.AddUnique(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
#else
			int32 FinalIndex = ImportedMesh->GetStaticMaterials().AddUnique(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
#endif
			ImportedMesh->GetSectionInfoMap().Set(LODIndex, FinalIndex, FMeshSectionInfo(FinalIndex));
			ImportedMesh->GetOriginalSectionInfoMap().Set(LODIndex, FinalIndex, ImportedMesh->GetSectionInfoMap().Get(LODIndex, FinalIndex));

			ImportContext.MaterialMap.Add(prim.material, FinalIndex);

			// If the mesh has vertex colors, add them to the material
			if (nColors > 0)
			{
				USKGLTFImporter::UseVertexColors(ExistingMaterial->GetMaterial());
				ExistingMaterial->GetMaterial()->PreEditChange(NULL);
				ExistingMaterial->GetMaterial()->PostEditChange();
			}
		}

		// Faces and UV/Normals
		staticMaterialIndex = ImportContext.MaterialMap.Find(prim.material);
		ensure(staticMaterialIndex);
		for (int faceIndex = 0; faceIndex < NumFaces; faceIndex++)
		{
			// Materials
			RawTriangles.FaceMaterialIndices.Add(*staticMaterialIndex);

			// Phong Smoothing
			RawTriangles.FaceSmoothingMasks.Add(0xFFFFFFFF);
		}
	}

	return ImportedMesh;
}

void FGLTFStaticMeshImporter::commitRawMesh(UStaticMesh* ImportedMesh, FRawMesh& RawTriangles) {

	RawTriangles.CompactMaterialIndices();

#if ENGINE_MAJOR_VERSION < 5
	if (!ImportedMesh->GetSourceModels().IsValidIndex(0))
	{
		ImportedMesh->GetSourceModels().AddDefaulted();
	}
	FStaticMeshSourceModel& SrcModel = ImportedMesh->GetSourceModel(0);
	SrcModel.RawMeshBulkData->SaveRawMesh(RawTriangles);
	SrcModel.BuildSettings.bBuildAdjacencyBuffer = false;
#else
	FStaticMeshSourceModel& SrcModel = ImportedMesh->GetSourceModels().IsValidIndex(0) ? ImportedMesh->GetSourceModel(0) : ImportedMesh->AddSourceModel();
	SrcModel.SaveRawMesh(RawTriangles, true);
#endif

	SrcModel.BuildSettings.bRecomputeNormals = RawTriangles.WedgeTangentZ.Num() == 0;
	SrcModel.BuildSettings.bUseMikkTSpace = RawTriangles.WedgeTangentZ.Num() != 0;
	SrcModel.BuildSettings.bRecomputeTangents = RawTriangles.WedgeTangentX.Num() == 0;
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.bBuildReversedIndexBuffer = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
}

#undef LOCTEXT_NAMESPACE
