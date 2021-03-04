// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/GLTFContainerBuilder.h"

class FGLTFWebBuilder : public FGLTFContainerBuilder
{
public:

	FGLTFWebBuilder(const FString& FilePath, const UGLTFExportOptions* ExportOptions, bool bSelectedActorsOnly);

	void Write(FArchive& Archive, FFeedbackContext* Context = GWarn);

private:

	void BundleWebViewer(const FString& ResourcesDir);
	void BundleLaunchHelper(const FString& ResourcesDir);

	static const TCHAR* GetLaunchHelperExecutable();

	static bool IsCustomExtension(EGLTFJsonExtension Value);
};
