// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

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

	// For now we won't show the import options and just import the full mesh each time
	// When we require more options that are useful then show this dialog and also add new parameters to it.
	//if (IsAutomatedImport() || GLTFImporter->ShowImportOptions(*ImportOptions))
	if (1)
	{
		tinygltf::Model* Model = GLTFImporter->ReadGLTFFile(ImportContext, Filename);
		if (Model)
		{
			ImportContext.Init(InParent, InName.ToString(), FPaths::GetPath(Filename), Model);
			ImportContext.ImportOptions = ImportOptions;
			ImportContext.bApplyWorldTransformToGeometry = ImportOptions->bApplyWorldTransformToGeometry;

			if (ImportOptions->bImportMaterials)
			{
				GLTFImporter->CreateNodeMaterials(ImportContext, ImportContext.Materials);
			}

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
}

void UGLTFAssetImportFactory::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, ImportOptions->GetClass(), ImportOptions, 0, CPF_InstancedReference);
}
