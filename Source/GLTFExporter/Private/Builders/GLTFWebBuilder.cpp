// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFWebBuilder.h"
#include "Builders/SKGLTFFileUtility.h"
#include "Builders/SKGLTFZipUtility.h"

FGLTFWebBuilder::FGLTFWebBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions, bool bSelectedActorsOnly)
	: FGLTFContainerBuilder(FilePath, ExportOptions, bSelectedActorsOnly)
{
}

void FGLTFWebBuilder::Write(FArchive& Archive, FFeedbackContext* Context)
{
	CompleteAllTasks(Context);

	if (bIsGlbFile)
	{
		WriteGlb(Archive);
	}
	else
	{
		WriteJson(Archive);
	}

	if (ExportOptions->bBundleWebViewer)
	{
		const FString ResourcesDir = FGLTFFileUtility::GetPluginDir() / TEXT("Resources");
		BundleWebViewer(ResourcesDir);
		BundleLaunchHelper(ResourcesDir);
	}
	else
	{
		const TSet<ESKGLTFJsonExtension> CustomExtensions = GetCustomExtensionsUsed();
		if (CustomExtensions.Num() > 0)
		{
			const FString ExtensionsString = FString::JoinBy(CustomExtensions, TEXT(", "),
				[](ESKGLTFJsonExtension Extension)
			{
				return FGLTFJsonUtility::ToString(Extension);
			});

			AddWarningMessage(FString::Printf(TEXT("Export uses some extensions that may only be supported in Unreal's bundled glTF viewer: %s"), *ExtensionsString));
		}
	}
}

void FGLTFWebBuilder::BundleWebViewer(const FString& ResourcesDir)
{
	const FString ArchiveFile = ResourcesDir / TEXT("GLTFWebViewer.zip");

	if (!FPaths::FileExists(ArchiveFile))
	{
		AddWarningMessage(FString::Printf(TEXT("No web viewer archive found at %s"), *ArchiveFile));
		return;
	}

	if (!FGLTFZipUtility::ExtractAllFiles(ArchiveFile, DirPath))
	{
		AddErrorMessage(FString::Printf(TEXT("Failed to extract web viewer files from %s"), *ArchiveFile));
		return;
	}

	const FString IndexFile = DirPath / TEXT("index.json");
	TSharedPtr<FJsonObject> JsonIndexRoot;

	if (!FGLTFFileUtility::ReadJsonFile(IndexFile, JsonIndexRoot))
	{
		AddWarningMessage(FString::Printf(TEXT("Failed to read web viewer index at %s"), *IndexFile));
		return;
	}

	const FString FileName = FPaths::GetCleanFilename(FilePath);
	const FString Name = FPaths::GetBaseFilename(FileName);

	// TODO: simplify the index.json to avoid unnecessary json fields
	TSharedPtr<FJsonObject> JsonAssetObject = MakeShared<FJsonObject>();
	JsonAssetObject->SetStringField(TEXT("name"), Name);
	JsonAssetObject->SetStringField(TEXT("filePath"), TEXT("../../") + FileName); // TODO: change root to mean the index.html folder (not viewer/playcanvas/)
	JsonAssetObject->SetStringField(TEXT("blobFileName"), FileName);
	JsonIndexRoot->SetArrayField(TEXT("assets"), { MakeShared<FJsonValueObject>(JsonAssetObject) });

	if (!FGLTFFileUtility::WriteJsonFile(IndexFile, JsonIndexRoot.ToSharedRef()))
	{
		AddWarningMessage(FString::Printf(TEXT("Failed to write web viewer index at %s"), *IndexFile));
	}
}

void FGLTFWebBuilder::BundleLaunchHelper(const FString& ResourcesDir)
{
	const FString ArchiveFile = ResourcesDir / TEXT("GLTFLaunchHelper.zip");

	if (!FPaths::FileExists(ArchiveFile))
	{
		AddWarningMessage(FString::Printf(TEXT("No launch helper archive found at %s"), *ArchiveFile));
		return;
	}

	const FString ExecutableName = TEXT("GLTFLaunchHelper.exe");

	if (!FGLTFZipUtility::ExtractOneFile(ArchiveFile, ExecutableName, DirPath))
	{
		AddErrorMessage(FString::Printf(TEXT("Failed to extract launch helper file (%s) from %s"), *ExecutableName, *ArchiveFile));
		return;
	}

	const FString ExecutableFile = DirPath / ExecutableName;

	if (!FGLTFFileUtility::SetExecutable(*ExecutableFile, true))
	{
		AddWarningMessage(FString::Printf(TEXT("Failed to make launch helper file executable at %s"), *ExecutableFile));
	}
}
