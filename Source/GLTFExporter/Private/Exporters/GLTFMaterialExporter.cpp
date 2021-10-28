// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/SKGLTFMaterialExporter.h"
#include "Exporters/SKGLTFExporterUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Materials/MaterialInterface.h"

USKGLTFMaterialExporter::USKGLTFMaterialExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMaterialInterface::StaticClass();
}

bool USKGLTFMaterialExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const UMaterialInterface* Material = CastChecked<UMaterialInterface>(Object);
	const UStaticMesh* PreviewMesh = Builder.ExportOptions->bExportPreviewMesh ? FGLTFExporterUtility::GetPreviewMesh(Material) : nullptr;

	if (Builder.ExportOptions->bExportPreviewMesh)
	{
		if (PreviewMesh != nullptr)
		{
			const FGLTFJsonMeshIndex MeshIndex = Builder.GetOrAddMesh(PreviewMesh, { Material });
			if (MeshIndex == INDEX_NONE)
			{
				Builder.AddErrorMessage(
					FString::Printf(TEXT("Failed to export preview mesh %s for material %s"),
					*Material->GetName(),
					*PreviewMesh->GetName()));
				return false;
			}

			FGLTFJsonNode Node;
			Node.Mesh = MeshIndex;
			const FGLTFJsonNodeIndex NodeIndex = Builder.AddNode(Node);

			FGLTFJsonScene Scene;
			Scene.Nodes.Add(NodeIndex);
			const FGLTFJsonSceneIndex SceneIndex = Builder.AddScene(Scene);

			Builder.DefaultScene = SceneIndex;
		}
		else
		{
			Builder.AddErrorMessage(
				FString::Printf(TEXT("Failed to export material %s because of missing preview mesh"),
				*Material->GetName()));
			return false;
		}
	}
	else
	{
		const FGLTFJsonMaterialIndex MaterialIndex = Builder.GetOrAddMaterial(Material);
		if (MaterialIndex == INDEX_NONE)
		{
			Builder.AddErrorMessage(
				FString::Printf(TEXT("Failed to export material %s"),
				*Material->GetName()));
			return false;
		}
	}

	return true;
}
