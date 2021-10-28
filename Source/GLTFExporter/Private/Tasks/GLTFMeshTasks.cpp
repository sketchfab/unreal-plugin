// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/SKGLTFMeshTasks.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFMeshUtility.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Rendering/SkeletalMeshRenderData.h"

namespace
{
	template <typename TangentVectorType>
	void CheckTangentVectors(const FStaticMeshVertexBuffer* VertexBuffer, bool& bOutZeroNormals, bool& bOutZeroTangents)
	{
		bool bZeroNormals = false;
		bool bZeroTangents = false;

		typedef TStaticMeshVertexTangentDatum<TangentVectorType> VertexTangentType;

		const void* TangentData = const_cast<FStaticMeshVertexBuffer*>(VertexBuffer)->GetTangentData();
		const VertexTangentType* VertexTangents = TangentData != nullptr ? static_cast<const VertexTangentType*>(TangentData) : nullptr;

		if (VertexTangents != nullptr)
		{
			const uint32 VertexCount = VertexBuffer->GetNumVertices();
			for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
			{
				const VertexTangentType& VertexTangent = VertexTangents[VertexIndex];
				bZeroNormals |= VertexTangent.TangentZ.ToFVector().IsNearlyZero();
				bZeroTangents |= VertexTangent.TangentX.ToFVector().IsNearlyZero();
			}
		}

		bOutZeroNormals = bZeroNormals;
		bOutZeroTangents = bZeroTangents;
	}

	void ValidateVertexBuffer(FGLTFConvertBuilder& Builder, const FStaticMeshVertexBuffer* VertexBuffer, const TCHAR* MeshName)
	{
		if (VertexBuffer == nullptr)
		{
			return;
		}

		bool bZeroNormals;
		bool bZeroTangents;

		if (VertexBuffer->GetUseHighPrecisionTangentBasis())
		{
			CheckTangentVectors<FPackedRGBA16N>(VertexBuffer, bZeroNormals, bZeroTangents);
		}
		else
		{
			CheckTangentVectors<FPackedNormal>(VertexBuffer, bZeroNormals, bZeroTangents);
		}

		if (bZeroNormals)
		{
			Builder.AddWarningMessage(FString::Printf(
				TEXT("Mesh %s has some nearly zero-length normals which can create some issues. Consider checking 'Recompute Normals' in the asset settings"),
				MeshName));
		}

		if (bZeroTangents)
		{
			Builder.AddWarningMessage(FString::Printf(
				TEXT("Mesh %s has some nearly zero-length tangents which can create some issues. Consider checking 'Recompute Tangents' in the asset settings"),
				MeshName));
		}
	}

	bool HasVertexColors(const FColorVertexBuffer* VertexBuffer)
	{
		const uint32 VertexCount = VertexBuffer->GetNumVertices();

		for (uint32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
		{
			const FColor& Color = VertexBuffer->VertexColor(VertexIndex);
			if (Color != FColor::White)
			{
				return true;
			}
		}

		return false;
	}
}

void FGLTFStaticMeshTask::Complete()
{
	FGLTFJsonMesh& JsonMesh = Builder.GetMesh(MeshIndex);
	JsonMesh.Name = StaticMeshComponent != nullptr ? FGLTFNameUtility::GetName(StaticMeshComponent) : StaticMesh->GetName();

	const FStaticMeshLODResources& MeshLOD = StaticMesh->GetLODForExport(LODIndex);
	const FPositionVertexBuffer* PositionBuffer = &MeshLOD.VertexBuffers.PositionVertexBuffer;
	const FStaticMeshVertexBuffer* VertexBuffer = &MeshLOD.VertexBuffers.StaticMeshVertexBuffer;
	const FColorVertexBuffer* ColorBuffer = &MeshLOD.VertexBuffers.ColorVertexBuffer; // TODO: add support for overriding color buffer by component

	if (Builder.ExportOptions->bExportVertexColors && HasVertexColors(ColorBuffer))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Vertex colors in mesh %s will act as a multiplier for base color in glTF, regardless of material, which may produce undesirable results."),
			*StaticMesh->GetName()));
	}
	else
	{
		ColorBuffer = nullptr;
	}

	if (StaticMeshComponent != nullptr && StaticMeshComponent->LODData.IsValidIndex(LODIndex))
	{
		const FStaticMeshComponentLODInfo& LODInfo = StaticMeshComponent->LODData[LODIndex];
		ColorBuffer = LODInfo.OverrideVertexColors != nullptr ? LODInfo.OverrideVertexColors : ColorBuffer;
	}

	const FGLTFMeshData* MeshData = Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::UseMeshData ?
		Builder.StaticMeshDataConverter.GetOrAdd(StaticMesh, StaticMeshComponent, LODIndex) : nullptr;

	if (MeshData != nullptr)
	{
		if (MeshData->Description.IsEmpty())
		{
			// TODO: report warning in case the mesh actually has data, which means we failed to extract a mesh description.
			MeshData = nullptr;
		}
		else if (MeshData->TexCoord < 0)
		{
			// TODO: report warning (about missing lightmap texture coordinate for baking with mesh data).
			MeshData = nullptr;
		}
	}

	ValidateVertexBuffer(Builder, VertexBuffer, *StaticMesh->GetName());

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const int32 MaterialCount = StaticMesh->StaticMaterials.Num();
#else
	const int32 MaterialCount = StaticMesh->GetStaticMaterials().Num();
#endif
	JsonMesh.Primitives.AddDefaulted(MaterialCount);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		const FGLTFIndexArray SectionIndices = FGLTFMeshUtility::GetSectionIndices(MeshLOD, MaterialIndex);
		const FGLTFMeshSection* ConvertedSection = MeshSectionConverter.GetOrAdd(&MeshLOD, SectionIndices);

		FGLTFJsonPrimitive& JsonPrimitive = JsonMesh.Primitives[MaterialIndex];
		JsonPrimitive.Indices = Builder.GetOrAddIndexAccessor(ConvertedSection);

		JsonPrimitive.Attributes.Position = Builder.GetOrAddPositionAccessor(ConvertedSection, PositionBuffer);
		if (JsonPrimitive.Attributes.Position == INDEX_NONE)
		{
			// TODO: report warning?
		}

		if (ColorBuffer != nullptr)
		{
			JsonPrimitive.Attributes.Color0 = Builder.GetOrAddColorAccessor(ConvertedSection, ColorBuffer);
		}

		// TODO: report warning if both Mesh Quantization (export options) and Use High Precision Tangent Basis (vertex buffer) are disabled
		JsonPrimitive.Attributes.Normal = Builder.GetOrAddNormalAccessor(ConvertedSection, VertexBuffer);
		JsonPrimitive.Attributes.Tangent = Builder.GetOrAddTangentAccessor(ConvertedSection, VertexBuffer);

		const uint32 UVCount = VertexBuffer->GetNumTexCoords();
		// TODO: report warning or option to limit UV channels since most viewers don't support more than 2?
		JsonPrimitive.Attributes.TexCoords.AddUninitialized(UVCount);

		for (uint32 UVIndex = 0; UVIndex < UVCount; ++UVIndex)
		{
			JsonPrimitive.Attributes.TexCoords[UVIndex] = Builder.GetOrAddUVAccessor(ConvertedSection, VertexBuffer, UVIndex);
		}

		const UMaterialInterface* Material = Materials[MaterialIndex];
		JsonPrimitive.Material =  Builder.GetOrAddMaterial(Material, MeshData, SectionIndices);
	}
}

void FGLTFSkeletalMeshTask::Complete()
{
	FGLTFJsonMesh& JsonMesh = Builder.GetMesh(MeshIndex);
	JsonMesh.Name = SkeletalMeshComponent != nullptr ? FGLTFNameUtility::GetName(SkeletalMeshComponent) : SkeletalMesh->GetName();

	const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
	const FSkeletalMeshLODRenderData& MeshLOD = RenderData->LODRenderData[LODIndex];

	const FPositionVertexBuffer* PositionBuffer = &MeshLOD.StaticVertexBuffers.PositionVertexBuffer;
	const FStaticMeshVertexBuffer* VertexBuffer = &MeshLOD.StaticVertexBuffers.StaticMeshVertexBuffer;
	const FColorVertexBuffer* ColorBuffer = &MeshLOD.StaticVertexBuffers.ColorVertexBuffer; // TODO: add support for overriding color buffer by component
	const FSkinWeightVertexBuffer* SkinWeightBuffer = MeshLOD.GetSkinWeightVertexBuffer(); // TODO: add support for overriding skin weight buffer by component
	// TODO: add support for skin weight profiles?
	// TODO: add support for morph targets

	if (Builder.ExportOptions->bExportVertexColors && HasVertexColors(ColorBuffer))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Vertex colors in mesh %s will act as a multiplier for base color in glTF, regardless of material, which may produce undesirable results."),
			*SkeletalMesh->GetName()));
	}
	else
	{
		ColorBuffer = nullptr;
	}

	if (SkeletalMeshComponent != nullptr && SkeletalMeshComponent->LODInfo.IsValidIndex(LODIndex))
	{
		const FSkelMeshComponentLODInfo& LODInfo = SkeletalMeshComponent->LODInfo[LODIndex];
		ColorBuffer = LODInfo.OverrideVertexColors != nullptr ? LODInfo.OverrideVertexColors : ColorBuffer;
		SkinWeightBuffer = LODInfo.OverrideSkinWeights != nullptr ? LODInfo.OverrideSkinWeights : SkinWeightBuffer;
	}

	const FGLTFMeshData* MeshData = Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::UseMeshData ?
		Builder.SkeletalMeshDataConverter.GetOrAdd(SkeletalMesh, SkeletalMeshComponent, LODIndex) : nullptr;

	if (MeshData != nullptr)
	{
		if (MeshData->Description.IsEmpty())
		{
			// TODO: report warning in case the mesh actually has data, which means we failed to extract a mesh description.
			MeshData = nullptr;
		}
		else if (MeshData->TexCoord < 0)
		{
			// TODO: report warning (about missing non-overlapping texture coordinate for baking with mesh data).
			MeshData = nullptr;
		}
	}

	ValidateVertexBuffer(Builder, VertexBuffer, *SkeletalMesh->GetName());

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	const uint16 MaterialCount = SkeletalMesh->Materials.Num();
#else
	const uint16 MaterialCount = SkeletalMesh->GetMaterials().Num();
#endif
	JsonMesh.Primitives.AddDefaulted(MaterialCount);

	for (uint16 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		const FGLTFIndexArray SectionIndices = FGLTFMeshUtility::GetSectionIndices(MeshLOD, MaterialIndex);
		const FGLTFMeshSection* ConvertedSection = MeshSectionConverter.GetOrAdd(&MeshLOD, SectionIndices);

		FGLTFJsonPrimitive& JsonPrimitive = JsonMesh.Primitives[MaterialIndex];
		JsonPrimitive.Indices = Builder.GetOrAddIndexAccessor(ConvertedSection);

		JsonPrimitive.Attributes.Position = Builder.GetOrAddPositionAccessor(ConvertedSection, PositionBuffer);
		if (JsonPrimitive.Attributes.Position == INDEX_NONE)
		{
			// TODO: report warning?
		}

		if (ColorBuffer != nullptr)
		{
			JsonPrimitive.Attributes.Color0 = Builder.GetOrAddColorAccessor(ConvertedSection, ColorBuffer);
		}

		// TODO: report warning if both Mesh Quantization (export options) and Use High Precision Tangent Basis (vertex buffer) are disabled
		JsonPrimitive.Attributes.Normal = Builder.GetOrAddNormalAccessor(ConvertedSection, VertexBuffer);
		JsonPrimitive.Attributes.Tangent = Builder.GetOrAddTangentAccessor(ConvertedSection, VertexBuffer);

		const uint32 UVCount = VertexBuffer->GetNumTexCoords();
		// TODO: report warning or option to limit UV channels since most viewers don't support more than 2?
		JsonPrimitive.Attributes.TexCoords.AddUninitialized(UVCount);

		for (uint32 UVIndex = 0; UVIndex < UVCount; ++UVIndex)
		{
			JsonPrimitive.Attributes.TexCoords[UVIndex] = Builder.GetOrAddUVAccessor(ConvertedSection, VertexBuffer, UVIndex);
		}

		if (Builder.ExportOptions->bExportVertexSkinWeights)
		{
			const uint32 GroupCount = (SkinWeightBuffer->GetMaxBoneInfluences() + 3) / 4;
			// TODO: report warning or option to limit groups (of joints and weights) since most viewers don't support more than one?
			JsonPrimitive.Attributes.Joints.AddUninitialized(GroupCount);
			JsonPrimitive.Attributes.Weights.AddUninitialized(GroupCount);

			for (uint32 GroupIndex = 0; GroupIndex < GroupCount; ++GroupIndex)
			{
				JsonPrimitive.Attributes.Joints[GroupIndex] = Builder.GetOrAddJointAccessor(ConvertedSection, SkinWeightBuffer, GroupIndex * 4);
				JsonPrimitive.Attributes.Weights[GroupIndex] = Builder.GetOrAddWeightAccessor(ConvertedSection, SkinWeightBuffer, GroupIndex * 4);
			}
		}

		const UMaterialInterface* Material = Materials[MaterialIndex];
		JsonPrimitive.Material =  Builder.GetOrAddMaterial(Material, MeshData, SectionIndices);
	}
}
