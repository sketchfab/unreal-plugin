// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFMeshSection.h"
#include "Rendering/MultiSizeIndexContainer.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Algo/MaxElement.h"

FGLTFMeshSection::FGLTFMeshSection(const FStaticMeshLODResources* MeshLOD, const FGLTFIndexArray& SectionIndices)
{
#if ENGINE_MAJOR_VERSION == 5
	const FStaticMeshSectionArray& Sections = MeshLOD->Sections;
#else
	const FStaticMeshLODResources::FStaticMeshSectionArray& Sections = MeshLOD->Sections;
#endif

	uint32 TotalIndexCount = 0;
	for (int32 SectionIndex : SectionIndices)
	{
		TotalIndexCount += Sections[SectionIndex].NumTriangles * 3;
	}

	IndexMap.Reserve(TotalIndexCount);
	IndexBuffer.AddUninitialized(TotalIndexCount);
	BoneMapLookup.Reserve(TotalIndexCount);

	TMap<uint32, uint32> IndexLookup;

	for (int32 SectionIndex : SectionIndices)
	{
		const FStaticMeshSection& MeshSection = Sections[SectionIndex];
		const uint32 IndexOffset = MeshSection.FirstIndex;
		const uint32 IndexCount = MeshSection.NumTriangles * 3;

		for (uint32 Index = 0; Index < IndexCount; Index++)
		{
			const uint32 OldIndex = MeshLOD->IndexBuffer.GetIndex(IndexOffset + Index);
			uint32 NewIndex;

			if (uint32* FoundIndex = IndexLookup.Find(OldIndex))
			{
				NewIndex = *FoundIndex;
			}
			else
			{
				NewIndex = IndexMap.Num();
				IndexLookup.Add(OldIndex, NewIndex);
				IndexMap.Add(OldIndex);
				BoneMapLookup.Add(0);
			}

			IndexBuffer[Index] = NewIndex;
		}
	}

	BoneMaps.Add({});
	MaxBoneIndex = 0;
}

FGLTFMeshSection::FGLTFMeshSection(const FSkeletalMeshLODRenderData* MeshLOD, const FGLTFIndexArray& SectionIndices)
	: MaxBoneIndex(0)
{
	const TArray<FSkelMeshRenderSection>& Sections = MeshLOD->RenderSections;

	uint32 TotalIndexCount = 0;
	for (int32 SectionIndex : SectionIndices)
	{
		TotalIndexCount += Sections[SectionIndex].NumTriangles * 3;
	}

	IndexMap.Reserve(TotalIndexCount);
	IndexBuffer.AddUninitialized(TotalIndexCount);
	BoneMapLookup.Reserve(TotalIndexCount);

	TMap<uint32, uint32> IndexLookup;

	const FRawStaticIndexBuffer16or32Interface* OldIndexBuffer = MeshLOD->MultiSizeIndexContainer.GetIndexBuffer();

	for (int32 SectionIndex : SectionIndices)
	{
		const FSkelMeshRenderSection& MeshSection = Sections[SectionIndex];
		const uint32 IndexOffset = MeshSection.BaseIndex;
		const uint32 IndexCount = MeshSection.NumTriangles * 3;
		const uint32 BoneMapIndex = BoneMaps.Num();

		for (uint32 Index = 0; Index < IndexCount; Index++)
		{
			const uint32 OldIndex = OldIndexBuffer->Get(IndexOffset + Index);
			uint32 NewIndex;

			if (uint32* FoundIndex = IndexLookup.Find(OldIndex))
			{
				NewIndex = *FoundIndex;
			}
			else
			{
				NewIndex = IndexMap.Num();
				IndexLookup.Add(OldIndex, NewIndex);
				IndexMap.Add(OldIndex);
				BoneMapLookup.Add(BoneMapIndex);
			}

			IndexBuffer[Index] = NewIndex;
		}

		BoneMaps.Add(MeshSection.BoneMap);

		if (const FBoneIndexType* MaxSectionBoneIndex = Algo::MaxElement(MeshSection.BoneMap))
		{
			MaxBoneIndex = FMath::Max(*MaxSectionBoneIndex, MaxBoneIndex);
		}
	}
}
