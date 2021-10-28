// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFImporterPrivatePCH.h"
#include "Misc/Paths.h"
#include "UObject/ObjectMacros.h"
#include "UObject/GCObject.h"
#include "SKGLTFImporter.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

class FGLTFImporterModule : public IGLTFImporterModule, public FGCObject
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		GLTFImporter = NewObject<USKGLTFImporter>();
	}

	virtual void ShutdownModule() override
	{
		GLTFImporter = nullptr;
	}

	class USKGLTFImporter* GetImporter() override
	{
		return GLTFImporter;
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(GLTFImporter);
	}
private:
	USKGLTFImporter* GLTFImporter;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGLTFImporterModule, SketchfabGLTF)

