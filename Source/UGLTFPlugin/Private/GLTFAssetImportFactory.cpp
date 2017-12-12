// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GLTFAssetImportFactory.h"
#include "GLTFImporter.h"
#include "IGLTFImporterModule.h"
#include "ActorFactories/ActorFactoryStaticMesh.h"
#include "ScopedTimers.h"
#include "GLTFImportOptions.h"
#include "Engine/StaticMesh.h"
#include "Paths.h"
#include "JsonObjectConverter.h"

void FGLTFAssetImportContext::Init(UObject* InParent, const FString& InName, const FString& InBasePath, class tinygltf::Model* InModel)
{
	FGltfImportContext::Init(InParent, InName, InBasePath, InModel);
}


UGLTFAssetImportFactory::UGLTFAssetImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UStaticMesh::StaticClass();

	ImportOptions = ObjectInitializer.CreateDefaultSubobject<UGLTFImportOptions>(this, TEXT("GLTFImportOptions"));

	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("gltf;GL Transmission Format (ASCII)"));
	Formats.Add(TEXT("glb;GL Transmission Format (Binary)"));
}

UObject* UGLTFAssetImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UObject* ImportedObject = nullptr;

	UGLTFImporter* GLTFImporter = IGLTFImporterModule::Get().GetImporter();

	if (IsAutomatedImport() || GLTFImporter->ShowImportOptions(*ImportOptions))
	{
		tinygltf::Model* Model = GLTFImporter->ReadGLTFFile(ImportContext, Filename);
		if (Model)
		{
			ImportContext.Init(InParent, InName.ToString(), FPaths::GetPath(Filename), Model);
			ImportContext.ImportOptions = ImportOptions;
			ImportContext.bApplyWorldTransformToGeometry = ImportOptions->bApplyWorldTransformToGeometry;

			if (1) //ImportOptions->bImportMaterials)
			{
				GLTFImporter->CreateNodeMaterials(ImportContext, ImportContext.Materials);
			}
			/*
			else if (ImportOptions->bImportTextures)
			{
				GLTFImporter->ImportTexturesFromNode(ImportContext, Model, ImportContext.RootPrim);
			}
			*/

			TArray<FGltfPrimToImport> PrimsToImport;
			ImportContext.PrimResolver->FindPrimsToImport(ImportContext, PrimsToImport);
			ImportedObject = GLTFImporter->ImportMeshes(ImportContext, PrimsToImport);


			// Just return the first one imported
			ImportedObject = ImportContext.PathToImportAssetMap.Num() > 0 ? ImportContext.PathToImportAssetMap.CreateConstIterator().Value() : nullptr;

			delete Model;
		}

		ImportContext.DisplayErrorMessages(IsAutomatedImport());
	}
	else
	{
		bOutOperationCanceled = true;
	}

	return ImportedObject;
}

bool UGLTFAssetImportFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if (Extension == TEXT("gltf") || Extension == TEXT("glb"))
	{
		return true;
	}

	return false;
}

void UGLTFAssetImportFactory::CleanUp()
{
	ImportContext = FGLTFAssetImportContext();
	//UnrealUSDWrapper::CleanUp();
}

void UGLTFAssetImportFactory::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, ImportOptions->GetClass(), ImportOptions, 0, CPF_InstancedReference);
}
