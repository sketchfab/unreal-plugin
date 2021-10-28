// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFPrimResolver.h"
#include "SKGLTFImporter.h"
#include "SKGLTFConversionUtils.h"
#include "AssetData.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistryModule.h"
#include "AssetSelection.h"
#include "IGLTFImporterModule.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "ActorFactories/ActorFactory.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

void USKGLTFPrimResolver::Init()
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
}

void USKGLTFPrimResolver::FindPrimsToImport(FGLTFImportContext& ImportContext, TArray<FGLTFPrimToImport>& OutPrimsToImport)
{
	if (ImportContext.Model->scenes.size() == 0)
		return;

	FMatrix scaleMat = FScaleMatrix(FVector(-100, 100, 100)); //Scale everything up and flip in the X axis

	auto &sceneNodes = ImportContext.Model->scenes[0].nodes;
	auto &nodes = ImportContext.Model->nodes;
	for (int32 i = 0; i < sceneNodes.size(); i++)
	{
		FindPrimsToImport_Recursive(ImportContext, &nodes[sceneNodes[i]], OutPrimsToImport, scaleMat);
	}
}

void USKGLTFPrimResolver::FindPrimsToImport_Recursive(FGLTFImportContext& ImportContext, tinygltf::Node* Prim, TArray<FGLTFPrimToImport>& OutTopLevelPrims, FMatrix ParentMat)
{
	const FString PrimName = GLTFToUnreal::ConvertString(Prim->name);

	FMatrix local = GLTFToUnreal::ConvertMatrix(*Prim);
	ParentMat = local * ParentMat;

	if (Prim->mesh >= 0 && Prim->mesh < ImportContext.Model->meshes.size())
	{
		FGLTFPrimToImport NewPrim;
		NewPrim.NumLODs = 0;
		NewPrim.Prim = Prim;

		NewPrim.LocalPrimTransform = local;
		NewPrim.WorldPrimTransform = ParentMat;

		OutTopLevelPrims.Add(NewPrim);
	}

	int32 NumChildren = Prim->children.size();
	for (int32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		int nodeIdx = Prim->children[ChildIdx];
		if (nodeIdx >= 0 && nodeIdx < ImportContext.Model->nodes.size())
		{
			FindPrimsToImport_Recursive(ImportContext, &ImportContext.Model->nodes[nodeIdx], OutTopLevelPrims, ParentMat);
		}
	}
}

bool USKGLTFPrimResolver::IsValidPathForImporting(const FString& TestPath) const
{
	return FPackageName::GetPackageMountPoint(TestPath) != NAME_None;
}

#undef LOCTEXT_NAMESPACE