// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class IGLTFImporterModule : public IModuleInterface
{

public:
	static inline IGLTFImporterModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IGLTFImporterModule >( "SketchfabGLTF" );
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SketchfabGLTF" );
	}

	virtual class USKGLTFImporter* GetImporter() = 0;
};

