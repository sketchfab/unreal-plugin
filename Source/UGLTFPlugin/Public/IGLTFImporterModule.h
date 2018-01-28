// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class IGLTFImporterModule : public IModuleInterface
{

public:
	static inline IGLTFImporterModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IGLTFImporterModule >( "UGLTFPlugin" );
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "UGLTFPlugin" );
	}

	virtual class UGLTFImporter* GetImporter() = 0;
};

