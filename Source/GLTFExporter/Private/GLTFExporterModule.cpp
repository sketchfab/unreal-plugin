// Copyright Epic Games, Inc. All Rights Reserved.

#include "GLTFExporterModule.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGLTFExporter);

/**
 * glTF Exporter module implementation (private)
 */
class FGLTFExporterModule final : public IGLTFExporterModule
{

public:
	virtual void StartupModule() override {}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGLTFExporterModule, GLTFExporter);
