// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/GLTFStaticMeshExporter.h"
#include "Builders/GLTFContainerBuilder.h"
#include "Engine/StaticMesh.h"

UGLTFStaticMeshExporter::UGLTFStaticMeshExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UStaticMesh::StaticClass();
}

bool UGLTFStaticMeshExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
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
