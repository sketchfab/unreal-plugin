// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFMeshSection.h"
#include "Converters/SKGLTFIndexArray.h"

template <typename MeshLODType>
class TGLTFMeshSectionConverter final : public TGLTFConverter<const FGLTFMeshSection*, const MeshLODType*, FGLTFIndexArray>
{
	TArray<TUniquePtr<FGLTFMeshSection>> Outputs;

	const FGLTFMeshSection* Convert(const MeshLODType* MeshLOD, FGLTFIndexArray SectionIndices)
	{
		return Outputs.Add_GetRef(MakeUnique<FGLTFMeshSection>(MeshLOD, SectionIndices)).Get();
	}
};

typedef TGLTFMeshSectionConverter<FStaticMeshLODResources> FGLTFStaticMeshSectionConverter;
typedef TGLTFMeshSectionConverter<FSkeletalMeshLODRenderData> FGLTFSkeletalMeshSectionConverter;
