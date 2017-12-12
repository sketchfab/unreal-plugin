// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GLTFPrimResolver.h"
#include "GLTFImporter.h"
#include "GLTFConversionUtils.h"
#include "AssetData.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "GLTFSceneImportFactory.h"
#include "AssetSelection.h"
#include "IGLTFImporterModule.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "ActorFactories/ActorFactory.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

void UGLTFPrimResolver::Init()
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
}

void UGLTFPrimResolver::FindPrimsToImport(FGltfImportContext& ImportContext, TArray<FGltfPrimToImport>& OutPrimsToImport)
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

void UGLTFPrimResolver::FindActorsToSpawn(FGLTFSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnDatas)
{
	/*
	IGltfPrim* RootPrim = ImportContext.RootPrim;

	if (RootPrim->HasTransform())
	{
		FindActorsToSpawn_Recursive(ImportContext, RootPrim, nullptr, OutActorSpawnDatas);
	}
	else
	{
		for (int32 ChildIdx = 0; ChildIdx < RootPrim->GetNumChildren(); ++ChildIdx)
		{
			IUsdPrim* Child = RootPrim->GetChild(ChildIdx);

			FindActorsToSpawn_Recursive(ImportContext, Child, nullptr, OutActorSpawnDatas);
		}
	}
	*/
}

AActor* UGLTFPrimResolver::SpawnActor(FGLTFSceneImportContext& ImportContext, const FActorSpawnData& SpawnData)
{
	/*
	UGLTFImporter* GLTFImporter = IGLTFImporterModule::Get().GetImporter();

	UGLTFSceneImportOptions* ImportOptions = Cast<UGLTFSceneImportOptions>(ImportContext.ImportOptions);

	const bool bFlattenHierarchy = ImportOptions->bFlattenHierarchy;

	AActor* ModifiedActor = nullptr;

	// Look for an existing actor and decide what to do based on the users choice
	AActor* ExistingActor = ImportContext.ExistingActors.FindRef(SpawnData.ActorName);

	bool bShouldSpawnNewActor = true;
	EExistingActorPolicy ExistingActorPolicy = ImportOptions->ExistingActorPolicy;

	const FMatrix& ActorMtx = SpawnData.WorldTransform;

	const FTransform ConversionTransform = ImportContext.ConversionTransform;

	const FTransform ActorTransform = ConversionTransform*FTransform(ActorMtx)*ConversionTransform;

	if (ExistingActor && ExistingActorPolicy == EExistingActorPolicy::UpdateTransform)
	{
		ExistingActor->Modify();
		ModifiedActor = ExistingActor;

		ExistingActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
		ExistingActor->SetActorRelativeTransform(ActorTransform);

		bShouldSpawnNewActor = false;
	}
	else if (ExistingActor && ExistingActorPolicy == EExistingActorPolicy::Ignore)
	{
		// Ignore this actor and do nothing
		bShouldSpawnNewActor = false;
	}

	if (bShouldSpawnNewActor)
	{
		UActorFactory* ActorFactory = ImportContext.EmptyActorFactory;

		AActor* SpawnedActor = nullptr;

		// The asset which should be used to spawn the actor
		UObject* ActorAsset = nullptr;

		TMap<FString, int32> NameToCount;

		// Note: it is expected that MeshPrim and ActorClassName are mutually exclusive in that if there is a mesh we do not assume we have a custom actor class yet
		if (SpawnData.MeshPrim)
		{
			//SlowTask.EnterProgressFrame(1.0f / PrimsToImport.Num(), FText::Format(LOCTEXT("ImportingGLTFMesh", "Importing Mesh {0} of {1}"), MeshCount + 1, PrimsToImport.Num()));

			FString FullPath;

			// If there is no asset path, come up with an asset path for the mesh to be imported
			if (SpawnData.AssetPath.IsEmpty())
			{
				FString MeshName = USDToUnreal::ConvertString(SpawnData.MeshPrim->GetPrimName());
				MeshName = ObjectTools::SanitizeObjectName(MeshName);

				// Generate a custom path to the actor

				if (ImportOptions->bGenerateUniquePathPerUSDPrim)
				{
					FString USDPath = USDToUnreal::ConvertString(SpawnData.ActorPrim->GetPrimPath());
					USDPath.RemoveFromEnd(MeshName);
					FullPath = ImportContext.ImportPathName / USDPath;
				}
				else
				{
					FullPath = ImportContext.ImportPathName;
				}

				// If we're generating unique meshes append a unique number if the mesh has already been found
				if (ImportOptions->bGenerateUniqueMeshes)
				{
					int32* Count = NameToCount.Find(MeshName);
					if (Count)
					{
						MeshName += TEXT("_");
						MeshName.AppendInt(*Count);
						++(*Count);
					}
					else
					{
						NameToCount.Add(MeshName, 1);
					}
				}

				FullPath /= MeshName;
			}
			else
			{
				FullPath = SpawnData.AssetPath;
			}

			ActorAsset = LoadObject<UObject>(nullptr, *FullPath, nullptr, LOAD_NoWarn | LOAD_Quiet);

			// Only import the asset if it doesnt exist || we're allowed to reimport it based on user settings
			const bool bImportAsset = ImportOptions->bImportMeshes && (!ActorAsset || ImportOptions->ExistingAssetPolicy == EExistingAssetPolicy::Reimport);

			if (bImportAsset)
			{
				if(IsValidPathForImporting(FullPath))
				{
					FString NewPackageName = FPackageName::ObjectPathToPackageName(FullPath);

					UPackage* Package = CreatePackage(nullptr, *NewPackageName);
					if (Package)
					{
						Package->FullyLoad();

						ImportContext.Parent = Package;
						ImportContext.ObjectName = FPackageName::GetLongPackageAssetName(Package->GetOutermost()->GetName());

						FUsdPrimToImport PrimToImport;
						PrimToImport.NumLODs = SpawnData.MeshPrim->GetNumLODs();
						PrimToImport.Prim = SpawnData.MeshPrim;

						// Compute a transform so that the mesh ends up in the correct place relative to the actor we are spawning.  This accounts for any skipped prims that should contribute to the final transform
						PrimToImport.CustomPrimTransform = USDToUnreal::ConvertMatrix(SpawnData.MeshPrim->GetLocalToAncestorTransform(SpawnData.ActorPrim));

						ActorAsset = USDImporter->ImportSingleMesh(ImportContext, ImportOptions->MeshImportType, PrimToImport);

						if (ActorAsset)
						{
							FAssetRegistryModule::AssetCreated(ActorAsset);
							Package->MarkPackageDirty();
						}
					}
				}
				else
				{
					ImportContext.AddErrorMessage(
						EMessageSeverity::Error, FText::Format(LOCTEXT("InvalidPathForImporting", "Could not import asset. '{0}' is not a valid path for assets"),
							FText::FromString(FullPath)));
				}
			}

		}
		else if (!SpawnData.ActorClassName.IsEmpty())
		{
			TSubclassOf<AActor> ActorClass = FindActorClass(ImportContext, SpawnData);
			if (ActorClass)
			{
				SpawnedActor = ImportContext.World->SpawnActor<AActor>(ActorClass);
			}
		}
		else if (!SpawnData.AssetPath.IsEmpty())
		{
			ActorAsset = LoadObject<UObject>(nullptr, *SpawnData.AssetPath);
			if (!ActorAsset)
			{
				ImportContext.AddErrorMessage(
					EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindUnrealAssetPath", "Could not find Unreal Asset '{0}' for USD prim '{1}'"),
						FText::FromString(SpawnData.AssetPath),
						FText::FromString(USDToUnreal::ConvertString(SpawnData.ActorPrim->GetPrimPath()))));

				UE_LOG(LogUSDImport, Error, TEXT("Could not find Unreal Asset '%s' for USD prim '%s'"), *SpawnData.AssetPath, *SpawnData.ActorName.ToString());
			}
		}

		if (ActorAsset)
		{
			UClass* AssetClass = ActorAsset->GetClass();

			UActorFactory* Factory = ImportContext.UsedFactories.FindRef(AssetClass);
			if (!Factory)
			{
				Factory = FActorFactoryAssetProxy::GetFactoryForAssetObject(ActorAsset);
				if (Factory)
				{
					ImportContext.UsedFactories.Add(AssetClass, Factory);
				}
				else
				{
					ImportContext.AddErrorMessage(
						EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindActorFactory", "Could not find an actor type to spawn for '{0}'"),
							FText::FromString(ActorAsset->GetName()))
					);
				}
			}

			ActorFactory = Factory;

		}

		if (ActorFactory)
		{
			SpawnedActor = ActorFactory->CreateActor(ActorAsset, ImportContext.World->GetCurrentLevel(), FTransform::Identity, RF_Transactional, SpawnData.ActorName);

			// For empty group actors set their initial mobility to static 
			if (ActorFactory == ImportContext.EmptyActorFactory)
			{
				SpawnedActor->GetRootComponent()->SetMobility(EComponentMobility::Static);
			}
		}

		if(SpawnedActor)
		{
			SpawnedActor->SetActorRelativeTransform(ActorTransform);

			if (SpawnData.AttachParentPrim && !bFlattenHierarchy)
			{
				// Spawned actor should be attached to a parent
				AActor* AttachPrim = PrimToActorMap.FindRef(SpawnData.AttachParentPrim);
				if (AttachPrim)
				{
					SpawnedActor->AttachToActor(AttachPrim, FAttachmentTransformRules::KeepRelativeTransform);
				}
			}

			FActorLabelUtilities::SetActorLabelUnique(SpawnedActor, SpawnData.ActorName.ToString(), &ImportContext.ActorLabels);
			ImportContext.ActorLabels.Add(SpawnedActor->GetActorLabel());
		}


		ModifiedActor = SpawnedActor;
	}

	PrimToActorMap.Add(SpawnData.ActorPrim, ModifiedActor);

	return ModifiedActor;
	*/
	return nullptr;
}


TSubclassOf<AActor> UGLTFPrimResolver::FindActorClass(FGLTFSceneImportContext& ImportContext, const FActorSpawnData& SpawnData) const
{
	TSubclassOf<AActor> ActorClass = nullptr;

	FString ActorClassName = SpawnData.ActorClassName;
	FName ActorClassFName = *ActorClassName;

	// Attempt to use the fully qualified path first.  If not use the expensive slow path.
	{
		ActorClass = LoadClass<AActor>(nullptr, *ActorClassName, nullptr);
	}

	if (!ActorClass)
	{
		UObject* TestObject = nullptr;
		TArray<FAssetData> AssetDatas;

		AssetRegistry->GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetDatas);

		UClass* TestClass = nullptr;
		for (const FAssetData& AssetData : AssetDatas)
		{
			if (AssetData.AssetName == ActorClassFName)
			{
				TestClass = Cast<UBlueprint>(AssetData.GetAsset())->GeneratedClass;
				break;
			}
		}

		if (TestClass && TestClass->IsChildOf<AActor>())
		{
			ActorClass = TestClass;
		}

		if (!ActorClass)
		{
			ImportContext.AddErrorMessage(
				EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindUnrealActorClass", "Could not find Unreal Actor Class '{0}' for USD prim '{1}'"),
					FText::FromString(ActorClassName),
					FText::FromString("")));// FText::FromString(USDToUnreal::ConvertString(SpawnData.ActorPrim->GetPrimPath()))));

		}
	}

	return ActorClass;
}

void UGLTFPrimResolver::FindPrimsToImport_Recursive(FGltfImportContext& ImportContext, tinygltf::Node* Prim, TArray<FGltfPrimToImport>& OutTopLevelPrims, FMatrix ParentMat)
{
	const FString PrimName = GLTFToUnreal::ConvertString(Prim->name);

	FMatrix local = GLTFToUnreal::ConvertMatrix(*Prim);
	ParentMat = local * ParentMat;
	
	if (Prim->mesh >= 0 && Prim->mesh < ImportContext.Model->meshes.size())
	{
		FGltfPrimToImport NewPrim;
		NewPrim.NumLODs = 0;
		NewPrim.Prim = Prim;

		NewPrim.LocalPrimTransform = local;
		NewPrim.WorldPrimTransform = ParentMat;

		OutTopLevelPrims.Add(NewPrim);
	}

	//if (!ImportContext.bFindUnrealAssetReferences && Prim->GetNumLODs() == 0)
	{
		// prim has no geometry data or LODs and does not define an unreal asset path (or we arent checking). Look at children
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
}

void UGLTFPrimResolver::FindActorsToSpawn_Recursive(FGLTFSceneImportContext& ImportContext, tinygltf::Node* Prim, tinygltf::Node* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas)
{
	/*
	TArray<FActorSpawnData>* SpawnDataArray = &OutSpawnDatas;

	UGLTFSceneImportOptions* ImportOptions = Cast<UGLTFSceneImportOptions>(ImportContext.ImportOptions);

	FString AssetPath;
	FName ActorClassName;
	if (Prim->HasTransform())
	{
		FActorSpawnData SpawnData;

		if (Prim->GetUnrealActorClass())
		{
			SpawnData.ActorClassName = USDToUnreal::ConvertString(Prim->GetUnrealActorClass());
		}

		if (Prim->GetUnrealAssetPath())
		{
			SpawnData.AssetPath = USDToUnreal::ConvertString(Prim->GetUnrealAssetPath());
		}

		if (Prim->HasGeometryData())
		{
			SpawnData.MeshPrim = Prim;
		}

		FName PrimName = USDToUnreal::ConvertName(Prim->GetPrimName());
		SpawnData.ActorName = PrimName;
		SpawnData.WorldTransform = USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform());
		SpawnData.AttachParentPrim = ParentPrim;
		SpawnData.ActorPrim = Prim;

		if (ImportOptions->ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ExistingActors.Contains(SpawnData.ActorName))
		{
			ImportContext.ActorsToDestroy.Add(SpawnData.ActorName);
		}

		OutSpawnDatas.Add(SpawnData);
	}

	if (!ImportContext.bFindUnrealAssetReferences || AssetPath.IsEmpty())
	{
		for (int32 ChildIdx = 0; ChildIdx < Prim->GetNumChildren(); ++ChildIdx)
		{
			IGltfPrim* Child = Prim->GetChild(ChildIdx);

			FindActorsToSpawn_Recursive(ImportContext, Child, Prim, *SpawnDataArray);
		}
	}
	*/
}


bool UGLTFPrimResolver::IsValidPathForImporting(const FString& TestPath) const
{
	return FPackageName::GetPackageMountPoint(TestPath) != NAME_None;
}

#undef LOCTEXT_NAMESPACE