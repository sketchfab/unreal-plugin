// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "GLTFImporterPrivatePCH.h"
#include "Misc/Paths.h"
#include "UObject/ObjectMacros.h"
#include "UObject/GCObject.h"
#include "GLTFImporter.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

class FGLTFImporterModule : public IGLTFImporterModule, public FGCObject
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
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

