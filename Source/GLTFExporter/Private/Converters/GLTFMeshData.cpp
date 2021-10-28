// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFMeshData.h"
#include "Converters/SKGLTFNameUtility.h"
#include "Converters/SKGLTFMeshUtility.h"
#include "StaticMeshAttributes.h"
#include "Developer/MeshMergeUtilities/Private/MeshMergeHelpers.h"
#include "Rendering/SkeletalMeshRenderData.h"

FGLTFMeshData::FGLTFMeshData(const UStaticMesh* StaticMesh, const UStaticMeshComponent* StaticMeshComponent, int32 LODIndex)
	: Parent(nullptr)
{
	FStaticMeshAttributes(Description).Register();

	if (StaticMeshComponent != nullptr)
	{
		Name = FGLTFNameUtility::GetName(StaticMeshComponent);
		FMeshMergeHelpers::RetrieveMesh(StaticMeshComponent, LODIndex, Description, true);
	}
	else
	{
		StaticMesh->GetName(Name);
		FMeshMergeHelpers::RetrieveMesh(StaticMesh, LODIndex, Description);
	}

	const int32 NumTexCoords = StaticMesh->GetLODForExport(LODIndex).VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	TexCoord = FMath::Min(StaticMesh->LightMapCoordinateIndex, NumTexCoords - 1);
#else
	TexCoord = FMath::Min(StaticMesh->GetLightMapCoordinateIndex(), NumTexCoords - 1);
#endif

	// TODO: add warning if texture coordinate has overlap
}

FGLTFMeshData::FGLTFMeshData(const USkeletalMesh* SkeletalMesh, const USkeletalMeshComponent* SkeletalMeshComponent, int32 LODIndex)
	: Parent(nullptr)
{
	FStaticMeshAttributes(Description).Register();

	if (SkeletalMeshComponent != nullptr)
	{
		Name = FGLTFNameUtility::GetName(SkeletalMeshComponent);
		FMeshMergeHelpers::RetrieveMesh(const_cast<USkeletalMeshComponent*>(SkeletalMeshComponent), LODIndex, Description, true);
	}
	else
	{
		SkeletalMesh->GetName(Name);

		// NOTE: this is a workaround for the fact that there's no overload for FMeshMergeHelpers::RetrieveMesh
		// that accepts a USkeletalMesh, only a USkeletalMeshComponent.
		// Writing a custom utility function that would work on a "standalone" skeletal mesh is problematic
		// since we would need to implement an equivalent of USkinnedMeshComponent::GetCPUSkinnedVertices too.

		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;
			SpawnParams.bAllowDuringConstructionScript = true;

			if (AActor* Actor = World->SpawnActor<AActor>(SpawnParams))
			{
				USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(Actor, TEXT(""), RF_Transient);
				Component->RegisterComponent();
				Component->SetSkeletalMesh(const_cast<USkeletalMesh*>(SkeletalMesh));

				FMeshMergeHelpers::RetrieveMesh(Component, LODIndex, Description, true);

				World->DestroyActor(Actor, false, false);
			}
		}
	}

	// TODO: don't assume last UV channel is non-overlapping
	const int32 NumTexCoords = SkeletalMesh->GetResourceForRendering()->LODRenderData[LODIndex].StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
	TexCoord = NumTexCoords - 1;

	// TODO: add warning if texture coordinate has overlap
}

const FGLTFMeshData* FGLTFMeshData::GetParent() const
{
	return Parent != nullptr ? Parent : this;
}
