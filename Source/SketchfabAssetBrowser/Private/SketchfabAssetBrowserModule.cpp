// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetBrowserPrivatePCH.h"
#include "Paths.h"
#include "UObject/ObjectMacros.h"
#include "GCObject.h"
#include "SketchfabAssetBrowser.h"
#include "ISettingsModule.h"

#include "Engine.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

class FSketchfabAssetBrowserModule : public ISketchfabAssetBrowserModule, public FGCObject
{
public:

	//Notes:
	//For menu example see: class FFunctionalTestingEditorModule : public IFunctionalTestingEditorModule

	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		AssetBrowser = NewObject<USketchfabAssetBrowser>();

		// make an extension to add the Orion function menu
		Extender = MakeShareable(new FExtender());
		Extender->AddMenuExtension(
			"General",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FSketchfabAssetBrowserModule::AddMenuEntry));

		// add the menu extension to the editor
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(Extender);

		//FModuleManager::Get().OnModulesChanged().AddRaw(this, &FFunctionalTestingEditorModule::OnModulesChanged);
	}

	virtual void ShutdownModule() override
	{
		AssetBrowser = nullptr;
	}

	class USketchfabAssetBrowser* GetAssetBrowser() override
	{
		return AssetBrowser;
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(AssetBrowser);
	}

private:
	void AddMenuEntry(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("Sketchfab", LOCTEXT("Sketchfab", "Sketchfab"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AssetBrowser", "Asset Browser"),
			LOCTEXT("Tooltip", "Launch the Sketchfab Asset Browser."),
			FSlateIcon(),//FSlateIcon(FEditorStyle::GetStyleSetName(), "AutomationTools.MenuIcon"),
			FUIAction(FExecuteAction::CreateRaw(this, &FSketchfabAssetBrowserModule::MenuCallbackOpenBrowser)));
		MenuBuilder.EndSection();
	}

	void MenuCallbackOpenBrowser() {
		UE_LOG(LogSketchfabAssetBrowser, Log, TEXT("MenuCallbackOpenBrowser"));

		if (AssetBrowser)
		{
			AssetBrowser->ShowWindow();
		}
	}

private:
	USketchfabAssetBrowser* AssetBrowser;
	TSharedPtr<FExtender> Extender;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSketchfabAssetBrowserModule, AssetBrowser)

