// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#define SKGLTFEXPORTER_MODULE_NAME TEXT("SKGLTFExporter")

DECLARE_LOG_CATEGORY_EXTERN(LogGLTFExporter, Log, All);

/**
 * The public interface of the GLTFExporter module
 */
class ISKGLTFExporterModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to IGLTFExporter
	 *
	 * @return Returns IGLTFExporter singleton instance, loading the module on demand if needed
	 */
	static ISKGLTFExporterModule& Get()
	{
		return FModuleManager::LoadModuleChecked<ISKGLTFExporterModule>(SKGLTFEXPORTER_MODULE_NAME);
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded(SKGLTFEXPORTER_MODULE_NAME);
	}
};
