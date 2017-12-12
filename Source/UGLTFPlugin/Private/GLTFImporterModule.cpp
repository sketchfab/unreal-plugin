// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GLTFImporterPrivatePCH.h"
#include "Paths.h"
#include "UObject/ObjectMacros.h"
#include "GCObject.h"
#include "GLTFImporter.h"
#include "ISettingsModule.h"
//#include "GLTFImporterProjectSettings.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

class FGLTFImporterModule : public IGLTFImporterModule, public FGCObject
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		// Ensure base usd plugins are found and loaded
		/*
		FString BasePluginPath = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir() + FString(TEXT("Editor/USDImporter")));

#if PLATFORM_WINDOWS
		BasePluginPath /= TEXT("Resources/UsdResources/Windows/plugins");
#elif PLATFORM_LINUX
		BasePluginPath /= ("Resources/UsdResources/Linux/plugins");
#endif

		std::vector<std::string> PluginPaths;
		PluginPaths.push_back(TCHAR_TO_ANSI(*BasePluginPath));

		// Load any custom plugins the user may have
		const TArray<FDirectoryPath>& AdditionalPluginDirectories = GetDefault<UUSDImporterProjectSettings>()->AdditionalPluginDirectories;

		for (const FDirectoryPath& Directory : AdditionalPluginDirectories)
		{
			if (!Directory.Path.IsEmpty())
			{
				PluginPaths.push_back(TCHAR_TO_ANSI(*Directory.Path));
			}
		}

		UnrealUSDWrapper::Initialize(PluginPaths);
		*/

		GLTFImporter = NewObject<UGLTFImporter>();
	}

	virtual void ShutdownModule() override
	{
		GLTFImporter = nullptr;
	}

	class UGLTFImporter* GetImporter() override
	{
		return GLTFImporter;
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(GLTFImporter);
	}
private:
	UGLTFImporter* GLTFImporter;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGLTFImporterModule, GLTFImporter)

