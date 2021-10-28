// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/SKGLTFStaticMeshExporter.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Engine/StaticMesh.h"

USKGLTFStaticMeshExporter::USKGLTFStaticMeshExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStaticMesh::StaticClass();
}

bool USKGLTFStaticMeshExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const UStaticMesh* StaticMesh = CastChecked<UStaticMesh>(Object);

	const FGLTFJsonMeshIndex MeshIndex = Builder.GetOrAddMesh(StaticMesh);
	if (MeshIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(FString::Printf(TEXT("Failed to export static mesh %s"), *StaticMesh->GetName()));
		return false;
	}

	FGLTFJsonNode Node;
	Node.Mesh = MeshIndex;
	const FGLTFJsonNodeIndex NodeIndex = Builder.AddNode(Node);

	FGLTFJsonScene Scene;
	Scene.Nodes.Add(NodeIndex);
	const FGLTFJsonSceneIndex SceneIndex = Builder.AddScene(Scene);

	Builder.DefaultScene = SceneIndex;
	return true;
}
