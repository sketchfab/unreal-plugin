// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFAssetImportFactory.h"
#include "SKGLTFImporter.h"
#include "IGLTFImporterModule.h"
#include "ActorFactories/ActorFactoryStaticMesh.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "SKGLTFImportOptions_SKETCHFAB.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
//#include "ZipFileFunctionLibrary.h"

#include "SKGLTFZipUtility.h"

void FGLTFAssetImportContext::Init(UObject* InParent, const FString& InName, const FString& InBasePath, class tinygltf::Model* InModel)
{
	FGLTFImportContext::Init(InParent, InName, InBasePath, InModel);
}


USKGLTFAssetImportFactory::USKGLTFAssetImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UStaticMesh::StaticClass();

	ImportOptions = ObjectInitializer.CreateDefaultSubobject<USKGLTFImportOptions_SKETCHFAB>(this, TEXT("GLTFImportOptions"));

	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("gltf;GL Transmission Format (ASCII)"));
	Formats.Add(TEXT("glb;GL Transmission Format (Binary)"));
	Formats.Add(TEXT("zip;GL Transmission Format (Zip)"));
}

UObject* USKGLTFAssetImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UObject* ImportedObject = nullptr;

	USKGLTFImporter* GLTFImporter = IGLTFImporterModule::Get().GetImporter();

	if (IsAutomatedImport() || GLTFImporter->ShowImportOptions(*ImportOptions))
	{
		bool deleteZippedData = false;
		FString gltfFile = Filename;
		FString destDir;
		const FString Extension = FPaths::GetExtension(Filename);
		if (Extension == TEXT("zip"))
		{
			destDir = FPaths::GetPath(Filename);
			FString fileString = FPaths::GetBaseFilename(Filename) + FString("__temp");
			destDir /= fileString;

			// Find first gltf and glb file inside the archive
			FString ZippedGltfFile;
			TArray<FString> ArchiveFiles = FGLTFZipUtility::GetAllFiles(Filename);
			for (const FString& File : ArchiveFiles)
			{
				if (FPaths::GetExtension(File) == TEXT("gltf") || FPaths::GetExtension(File) == TEXT("glb"))
				{
					ZippedGltfFile = File;
					break;
				}
			}
			if (ZippedGltfFile.IsEmpty())
			{
				return nullptr;
			}

			gltfFile = destDir / ZippedGltfFile;
			IPlatformFile& FS = IPlatformFile::GetPlatformPhysical();
			if (!FS.CreateDirectory(*destDir))
			{
				return nullptr;
			}

			if (!FGLTFZipUtility::ExtractAllFiles(Filename, destDir))
			{
				return nullptr;
			}

			deleteZippedData = true;
		}

		tinygltf::Model* Model = GLTFImporter->ReadGLTFFile(ImportContext, gltfFile);
		if (Model)
		{
			ImportContext.Init(InParent, InName.ToString(), FPaths::GetPath(gltfFile), Model);
			ImportContext.ImportOptions = ImportOptions;
			ImportContext.bApplyWorldTransform = ImportOptions->bApplyWorldTransform;
			ImportContext.bImportInNewFolder = ImportOptions->bImportInNewFolder;

			if (ImportOptions->bImportMaterials)
			{
				GLTFImporter->CreateNodeMaterials(ImportContext, ImportContext.Materials);
			}

			TArray<FGLTFPrimToImport> PrimsToImport;
			ImportContext.PrimResolver->FindPrimsToImport(ImportContext, PrimsToImport);
			ImportedObject = GLTFImporter->ImportMeshes(ImportContext, PrimsToImport);

			ImportedObject->PreEditChange(NULL);
			ImportedObject->PostEditChange();


			// Just return the first one imported
			ImportedObject = ImportContext.PathToImportAssetMap.Num() > 0 ? ImportContext.PathToImportAssetMap.CreateConstIterator().Value() : nullptr;

			delete Model;
		}

		if (deleteZippedData)
		{
			IPlatformFile& FS = IPlatformFile::GetPlatformPhysical();
			if (!FS.DeleteDirectoryRecursively(*destDir))
			{
				return nullptr;
			}
		}

		ImportContext.DisplayErrorMessages(IsAutomatedImport());
	}
	else
	{
		bOutOperationCanceled = true;
	}

	return ImportedObject;
}

bool USKGLTFAssetImportFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if (Extension == TEXT("gltf") || Extension == TEXT("glb"))
	{
		return true;
	}

	bZipDone = false;
	bFoundGLTFInZip = false;
	ZipFileName = "";

	if (Extension == TEXT("zip"))
	{
		//if (!UZipFileFunctionLibrary::ListFilesInArchive(Filename, this))
		//	return false;

		return bFoundGLTFInZip;
	}

	return false;
}

void USKGLTFAssetImportFactory::CleanUp()
{
	ImportContext = FGLTFAssetImportContext();
}

void USKGLTFAssetImportFactory::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, ImportOptions->GetClass(), ImportOptions, 0, CPF_InstancedReference);
}


//=====================================================================================================
// IZipUtilityInterface overrides
/*
void USKGLTFAssetImportFactory::OnProgress_Implementation(const FString& archive, float percentage, int32 bytes)
{

}

void USKGLTFAssetImportFactory::OnDone_Implementation(const FString& archive, EZipUtilityCompletionState CompletionState)
{
	bZipDone = true;
}

void USKGLTFAssetImportFactory::OnStartProcess_Implementation(const FString& archive, int32 bytes)
{

}

void USKGLTFAssetImportFactory::OnFileDone_Implementation(const FString& archive, const FString& file)
{

}


void USKGLTFAssetImportFactory::OnFileFound_Implementation(const FString& archive, const FString& file, int32 size)
{
	const FString Extension = FPaths::GetExtension(file);
	if (Extension == TEXT("gltf") || Extension == TEXT("glb"))
	{
		ZipFileName = file;
		bFoundGLTFInZip = true;
	}
}
*/