// Copyright Epic Games, Inc. All Rights Reserved.

#include "SKGLTFExporterRuntimeModule.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGLTFExporterRuntime);

/**
 * glTF Exporter runtime module implementation (private)
 */
class FSKGLTFExporterRuntimeModule final : public ISKGLTFExporterRuntimeModule
{

public:
	virtual void StartupModule() override {}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSKGLTFExporterRuntimeModule, SKGLTFExporterRuntime);
