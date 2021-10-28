// Copyright Epic Games, Inc. All Rights Reserved.

#include "Json/SKGLTFJsonRoot.h"
#include "SKGLTFExporterModule.h"
#include "Interfaces/IPluginManager.h"
#include "Runtime/Launch/Resources/Version.h"

FGLTFJsonAsset::FGLTFJsonAsset()
	: Version(TEXT("2.0"))
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Sketchfab");
	const FPluginDescriptor& PluginDescriptor = Plugin->GetDescriptor();

	Generator = TEXT(EPIC_PRODUCT_NAME) TEXT(" ") VERSION_STRINGIFY(ENGINE_MAJOR_VERSION) TEXT(".") VERSION_STRINGIFY(ENGINE_MINOR_VERSION)
		TEXT(" ") + PluginDescriptor.FriendlyName + TEXT(" ") + PluginDescriptor.VersionName;
}
