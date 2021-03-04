// Copyright Epic Games, Inc. All Rights Reserved.

#include "GLTFExporterRuntimeModule.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGLTFExporterRuntime);

/**
 * glTF Exporter runtime module implementation (private)
 */
class FGLTFExporterRuntimeModule final : public IGLTFExporterRuntimeModule
{

public:
	virtual void StartupModule() override {}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGLTFExporterRuntimeModule, GLTFExporterRuntime);
