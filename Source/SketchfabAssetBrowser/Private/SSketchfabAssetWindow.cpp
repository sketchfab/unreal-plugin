// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SSketchfabAssetWindow.h"
#include "ScopedSlowTask.h"
#include "SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "PackageTools.h"
#include "Engine/StaticMesh.h"
#include "SBox.h"
#include "SButton.h"
#include "SlateApplication.h"
#include "FileManager.h"

#include "Misc/FileHelper.h"

#include "SSplitter.h"
#include "SComboButton.h"
#include "MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetWindow"

DEFINE_LOG_CATEGORY(LogSketchfabAssetWindow);

SSketchfabAssetWindow::~SSketchfabAssetWindow()
{
}

void SSketchfabAssetWindow::Construct(const FArguments& InArgs)
{
	Window = InArgs._WidgetWindow;
	AssetData = InArgs._AssetData;

	TSharedRef<SVerticalBox> RootNode = SNew(SVerticalBox);

	if (InArgs._ThumbnailPool.IsValid())
	{
		FSlateDynamicImageBrush* Texture = NULL;
		InArgs._ThumbnailPool->AccessTexture(AssetData, AssetData.ThumbnailWidth, AssetData.ThumbnailHeight, &Texture);

		// The viewport for the rendered thumbnail, if it exists
		if (Texture)
		{
			RootNode->AddSlot()
			[
				SNew(SBox)
				.WidthOverride(AssetData.ThumbnailWidth)
				.HeightOverride(AssetData.ThumbnailHeight)
				[
					SNew(SImage).Image(Texture)
				]
			];
		}
	}

	ChildSlot
	[
		RootNode
	];
}

#undef LOCTEXT_NAMESPACE

