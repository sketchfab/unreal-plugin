// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class ZIPUTILITY_API FZipUtilityModule : public IModuleInterface
{
public:
	
	//CHN:
	//Maybe we need to use a public interface to fetch the module methods for C++? get a reference to zipfile etc

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline FZipUtilityModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FZipUtilityModule >("ZipUtility");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ZipUtility");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};