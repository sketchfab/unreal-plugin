// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "StaticMeshImporter.h"
#include "GLTFImporter.h"
#include "GLTFConversionUtils.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

UStaticMesh* FGLTFStaticMeshImporter::ImportStaticMesh(FGLTFImportContext& ImportContext, const FGLTFPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh)
{
	const FTransform& ConversionTransform = ImportContext.ConversionTransform;
	const FMatrix PrimToWorld = ImportContext.bApplyWorldTransformToGeometry ? PrimToImport.WorldPrimTransform : FMatrix::Identity;

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
		ImportedMesh->StaticMaterials.Empty();
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
		int32 TangentXOffset = RawTriangles.WedgeTangentX.Num();
		int32 TangentYOffset = RawTriangles.WedgeTangentY.Num();
		int32 TangentZOffset = RawTriangles.WedgeTangentZ.Num();
		int32 NumFaces = 0;

		// Indices
		{
			int accessorIndex = prim.indices;
			if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
			{
				const tinygltf::Accessor &accessor = model->accessors[accessorIndex];
				const tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
				const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
				size_t offset = accessor.byteOffset + bufferView.byteOffset;

				if (accessor.count > 0)
				{
					NumFaces = accessor.count / 3;
					RawTriangles.WedgeIndices.AddZeroed(accessor.count);
					ensure(accessor.type == TINYGLTF_TYPE_SCALAR);
					if (accessor.type == TINYGLTF_TYPE_SCALAR)
					{
						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							unsigned short *data = (unsigned short*)&buffer.data[offset];
							for (int i = 0; i < accessor.count; i++)
							{
								RawTriangles.WedgeIndices[WedgeOffset + i] = VertexOffset + data[i];
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_INT:
						{
							int32 *data = (int32*)&buffer.data[offset];
							for (int i = 0; i < accessor.count; i++)
							{
								RawTriangles.WedgeIndices[WedgeOffset + i] = VertexOffset + data[i];
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						{
							uint32 *data = (uint32*)&buffer.data[offset];
							for (int i = 0; i < accessor.count; i++)
							{
								RawTriangles.WedgeIndices[WedgeOffset + i] = VertexOffset + data[i];
							}
						}
						break;
						default: ensure(false); break;
						}
					}
				}
			}
		}

		const auto &position = prim.attributes.find("POSITION");
		if (position != prim.attributes.end())
		{
			int accessorIndex = position->second;
			if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
			{
				const tinygltf::Accessor &accessor = model->accessors[accessorIndex];
				const tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
				const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
				size_t offset = accessor.byteOffset + bufferView.byteOffset;

				{
					RawTriangles.VertexPositions.AddZeroed(accessor.count);
					ensure(accessor.type == TINYGLTF_TYPE_VEC3);
					if (accessor.type == TINYGLTF_TYPE_VEC3)
					{
						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							float *data = (float*)&buffer.data[offset];
							for (int i = 0; i < accessor.count; i++)
							{
								int index = i * 3;
								FVector Pos = FVector(data[index], data[index + 1], data[index + 2]);
								Pos = FinalTransform.TransformPosition(Pos);
								RawTriangles.VertexPositions[VertexOffset + i] = Pos;
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						{
							double *data = (double*)&buffer.data[offset];
							for (int i = 0; i < accessor.count; i++)
							{
								int index = i * 3;
								FVector Pos = FVector(data[index], data[index + 1], data[index + 2]);
								Pos = FinalTransform.TransformPosition(Pos);
								RawTriangles.VertexPositions[VertexOffset + i] = Pos;
							}
						}
						break;
						default: ensure(false); break;
						}
					}
				}
			}
		}


		const auto &normals = prim.attributes.find("NORMAL");
		if (normals != prim.attributes.end())
		{
			int accessorIndex = normals->second;
			if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
			{
				const tinygltf::Accessor &accessor = model->accessors[accessorIndex];
				const tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
				const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
				size_t offset = accessor.byteOffset + bufferView.byteOffset;

				{
					RawTriangles.WedgeTangentZ.AddUninitialized(NumFaces * 3);
					ensure(accessor.type == TINYGLTF_TYPE_VEC3);
					if (accessor.type == TINYGLTF_TYPE_VEC3)
					{
						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							float *data = (float*)&buffer.data[offset];
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
						break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						{
							double *data = (double*)&buffer.data[offset];
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
						break;
						default: ensure(false); break;
						}
					}
				}
			}
		}

		const auto &tangent = prim.attributes.find("TANGENT");
		if (tangent != prim.attributes.end())
		{
			int accessorIndex = tangent->second;
			if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
			{
				const tinygltf::Accessor &accessor = model->accessors[accessorIndex];
				const tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
				const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
				size_t offset = accessor.byteOffset + bufferView.byteOffset;

				{
					RawTriangles.WedgeTangentX.AddUninitialized(NumFaces * 3);
					switch (accessor.type)
					{
					case TINYGLTF_TYPE_VEC3:
					{
						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							float *data = (float*)&buffer.data[offset];
							for (int FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
							{
								for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
								{
									const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
									int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 3;
									FVector Tangent = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
									FVector TransformedTangent = FinalTransformIT.TransformVector(Tangent);
									RawTriangles.WedgeTangentX[WedgeIdx] = TransformedTangent.GetSafeNormal();
								}
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						{
							double *data = (double*)&buffer.data[offset];
							for (int32 FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
							{
								for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
								{
									const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
									int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 3;
									FVector Tangent = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
									FVector TransformedTangent = FinalTransformIT.TransformVector(Tangent);
									RawTriangles.WedgeTangentX[WedgeIdx] = TransformedTangent.GetSafeNormal();
								}
							}
						}
						break;
						default: ensure(false); break;
						}
					}
					break;
					case TINYGLTF_TYPE_VEC4:
					{
						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
						{
							float *data = (float*)&buffer.data[offset];
							for (int32 FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
							{
								for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
								{
									const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
									const int32 TangentXIdx = TangentXOffset + FaceIdx * 3 + CornerIdx;
									int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 4;
									FVector Tangent = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
									FVector TransformedTangent = FinalTransformIT.TransformVector(Tangent);
									RawTriangles.WedgeTangentX[TangentXIdx] = TransformedTangent.GetSafeNormal();
								}
							}
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						{
							double *data = (double*)&buffer.data[offset];
							for (int32 FaceIdx = 0; FaceIdx < NumFaces; FaceIdx++)
							{
								for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
								{
									const int32 WedgeIdx = WedgeOffset + FaceIdx * 3 + CornerIdx;
									const int32 TangentXIdx = TangentXOffset + FaceIdx * 3 + CornerIdx;
									int32 DataIdx = (RawTriangles.WedgeIndices[WedgeIdx] - VertexOffset) * 4;
									FVector Tangent = FVector(data[DataIdx], data[DataIdx + 1], data[DataIdx + 2]);
									FVector TransformedTangent = FinalTransformIT.TransformVector(Tangent);
									RawTriangles.WedgeTangentX[TangentXIdx] = TransformedTangent.GetSafeNormal();
								}
							}
						}
						break;
						default: ensure(false); break;
						}
					}
					break;
					default: ensure(false); break;
					}
				}
			}
		}

		bool uvsAdded = false;
		for (int32 uv = 0; uv < MAX_MESH_TEXTURE_COORDS; uv++)
		{
			uvsAdded |= AddUVs(ImportContext.Model, prim, RawTriangles, NumFaces, WedgeOffset, VertexOffset, uv);
		}

		if (!uvsAdded)
		{
			// Have to at least have one UV set
			ImportContext.AddErrorMessage(EMessageSeverity::Warning,
				FText::Format(LOCTEXT("StaticMeshesHaveNoUVS", "{0} (LOD {1}) has no UVs.  At least one valid UV set should exist on a static mesh. This mesh will likely have rendering issues"),
					FText::FromString(ImportContext.ObjectName),
					FText::AsNumber(0)));

			RawTriangles.WedgeTexCoords[0].AddZeroed(NumFaces * 3);
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

				I0 = TangentXOffset + fidx + 0;
				I2 = TangentXOffset + fidx + 2;
				if (RawTriangles.WedgeTangentX.Num())
				{
					Swap(RawTriangles.WedgeTangentX[I0], RawTriangles.WedgeTangentX[I2]);
				}

				I0 = TangentYOffset + fidx + 0;
				I2 = TangentYOffset + fidx + 2;
				if (RawTriangles.WedgeTangentY.Num())
				{
					Swap(RawTriangles.WedgeTangentY[I0], RawTriangles.WedgeTangentY[I2]);
				}

				I0 = TangentZOffset + fidx + 0;
				I2 = TangentZOffset + fidx + 2;
				if (RawTriangles.WedgeTangentZ.Num())
				{
					Swap(RawTriangles.WedgeTangentZ[I0], RawTriangles.WedgeTangentZ[I2]);
				}
			}
		}

		FColor WhiteVertex = FColor(255, 255, 255, 255);
		FVector EmptyVector = FVector(0, 0, 0);

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

			int32 FinalIndex = ImportedMesh->StaticMaterials.AddUnique(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
			ImportedMesh->GetSectionInfoMap().Set(LODIndex, FinalIndex, FMeshSectionInfo(FinalIndex));
			ImportedMesh->GetOriginalSectionInfoMap().Set(LODIndex, FinalIndex, ImportedMesh->GetSectionInfoMap().Get(LODIndex, FinalIndex));

			ImportContext.MaterialMap.Add(prim.material, FinalIndex);
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


	if (!ImportedMesh->GetSourceModels().IsValidIndex(LODIndex))
	{
		// Add one LOD
		ImportedMesh->GetSourceModels().AddDefaulted();
	}

	FStaticMeshSourceModel& SrcModel = ImportedMesh->GetSourceModel(LODIndex);

	RawTriangles.CompactMaterialIndices();

	SrcModel.RawMeshBulkData->SaveRawMesh(RawTriangles);

	// Recompute normals if we did not import any
	SrcModel.BuildSettings.bRecomputeNormals = RawTriangles.WedgeTangentZ.Num() == 0;

	// Use mikktSpace if we have normals
	SrcModel.BuildSettings.bUseMikkTSpace = RawTriangles.WedgeTangentZ.Num() != 0;

	// Recompute tangents if we did not import any
	SrcModel.BuildSettings.bRecomputeTangents = RawTriangles.WedgeTangentX.Num() == 0;

	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.bBuildAdjacencyBuffer = false;
	SrcModel.BuildSettings.bBuildReversedIndexBuffer = false;

	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;

	return ImportedMesh;
}

bool FGLTFStaticMeshImporter::AddUVs(const tinygltf::Model *model, const tinygltf::Primitive &prim, FRawMesh &RawTriangles, int32 NumFaces, int32 WedgeOffset, int32 VertexOffset, int32 uvIndex)
{
	bool uvsAdded = false;
	std::string name = "TEXCOORD_" + std::to_string(uvIndex);
	const auto &texcoords = prim.attributes.find(name);
	if (texcoords != prim.attributes.end())
	{
		int accessorIndex = texcoords->second;
		if (accessorIndex >= 0 && accessorIndex < model->accessors.size())
		{
			const tinygltf::Accessor &accessor = model->accessors[accessorIndex];
			const tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
			size_t offset = accessor.byteOffset + bufferView.byteOffset;

			if (accessor.count > 0)
			{
				//One uv per vertex on every polygon
				RawTriangles.WedgeTexCoords[uvIndex].AddUninitialized(NumFaces * 3);
				TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[uvIndex];
				ensure(accessor.type == TINYGLTF_TYPE_VEC2);
				if (accessor.type == TINYGLTF_TYPE_VEC2)
				{
					switch (accessor.componentType)
					{
					case TINYGLTF_COMPONENT_TYPE_FLOAT:
					{
						int32 WedgesCount = RawTriangles.WedgeIndices.Num();
						int32 TexCoordsCount = TexCoords.Num();
						float *data = (float*)&buffer.data[offset];
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

							if (index0 >= accessor.count || index1 >= accessor.count || index2 >= accessor.count)
								break;

							index0 *= 2;
							index1 *= 2;
							index2 *= 2;

							TexCoords[I0] = FVector2D(data[index0], data[index0 + 1]);
							TexCoords[I1] = FVector2D(data[index1], data[index1 + 1]);
							TexCoords[I2] = FVector2D(data[index2], data[index2 + 1]);
						}
						uvsAdded = true;
					}
					break;
					case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					{
						int32 WedgesCount = RawTriangles.WedgeIndices.Num();
						int32 TexCoordsCount = TexCoords.Num();
						double *data = (double*)&buffer.data[offset];
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

							if (index0 >= accessor.count || index1 >= accessor.count || index2 >= accessor.count)
								break;

							index0 *= 2;
							index1 *= 2;
							index2 *= 2;

							TexCoords[I0] = FVector2D(data[index0], data[index0 + 1]);
							TexCoords[I1] = FVector2D(data[index1], data[index1 + 1]);
							TexCoords[I2] = FVector2D(data[index2], data[index2 + 1]);
						}
					}
					break;
					default: ensure(false); break;
					}
				}
			}
		}
	}

	return uvsAdded;
}

#undef LOCTEXT_NAMESPACE
