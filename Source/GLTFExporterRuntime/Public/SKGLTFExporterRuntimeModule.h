// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGLTFExporterRuntime, Log, All);

/**
 * The public interface of the GLTFExporterRuntime module
 */
class ISKGLTFExporterRuntimeModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to IGLTFExporterRuntime
	 *
	 * @return Returns IGLTFExporterRuntime singleton instance, loading the module on demand if needed
	 */
	static ISKGLTFExporterRuntimeModule& Get()
	{
		return FModuleManager::LoadModuleChecked<ISKGLTFExporterRuntimeModule>("SKGLTFExporterRuntime");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SKGLTFExporterRuntime");
	}
};
