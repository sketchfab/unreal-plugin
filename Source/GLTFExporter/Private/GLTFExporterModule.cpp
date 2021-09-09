// Copyright Epic Games, Inc. All Rights Reserved.

#include "SKGLTFExporterModule.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGLTFExporter);

/**
 * glTF Exporter module implementation (private)
 */
class FSKGLTFExporterModule final : public ISKGLTFExporterModule
{

public:
	virtual void StartupModule() override {}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FSKGLTFExporterModule, SKGLTFExporter);
