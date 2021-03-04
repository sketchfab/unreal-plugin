// Copyright Epic Games, Inc. All Rights Reserved.

#include "Json/GLTFJsonRoot.h"
#include "GLTFExporterModule.h"
#include "Interfaces/IPluginManager.h"
#include "Runtime/Launch/Resources/Version.h"

FGLTFJsonAsset::FGLTFJsonAsset()
	: Version(TEXT("2.0"))
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(GLTFEXPORTER_MODULE_NAME);
	const FPluginDescriptor& PluginDescriptor = Plugin->GetDescriptor();

	// TODO: add configuration option to include detailed engine and plugin version info?

	Generator = TEXT(EPIC_PRODUCT_NAME) TEXT(" ") VERSION_STRINGIFY(ENGINE_MAJOR_VERSION) TEXT(" ") + PluginDescriptor.FriendlyName;
}
