// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SSketchfabAssetWindow.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "PackageTools.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"

#include "Misc/FileHelper.h"

#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SketchfabRESTClient.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetWindow"

DEFINE_LOG_CATEGORY(LogSketchfabAssetWindow);

SSketchfabAssetWindow::~SSketchfabAssetWindow()
{
}

void SSketchfabAssetWindow::Construct(const FArguments& InArgs)
{
	Window = InArgs._WidgetWindow;
	AssetData = InArgs._AssetData;
	AssetThumbnailPool = InArgs._ThumbnailPool;
	OnDownloadRequest = InArgs._OnDownloadRequest;

	TSharedRef<SVerticalBox> RootNode = SNew(SVerticalBox);

	RootNode->AddSlot()
	.Padding(2, 2, 2, 2)
	.AutoHeight()
	[
		SNew(SUniformGridPanel)
		.SlotPadding(10)
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(EHorizontalAlignment::HAlign_Left)
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock)
				.Text(FText::FromString(AssetData.ModelName.ToString()))
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(AssetData.AuthorName.ToString()))
			]
		]
		+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(EHorizontalAlignment::HAlign_Right)
			.Padding(0, 0, 0, 2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("SSketchfabAssetWindow_OpenOnSketchfab", "View on Sketchfab"))
				.OnClicked(this, &SSketchfabAssetWindow::ViewOnSketchfab)
			]
		]
	];





	if (AssetThumbnailPool.IsValid())
	{
		FDeferredCleanupSlateBrush* Texture = NULL;
		AssetThumbnailPool->AccessTexture(AssetData, AssetData.ThumbnailWidth, AssetData.ThumbnailHeight, &Texture);
		// The viewport for the rendered thumbnail, if it exists
		if (Texture)
		{
			RootNode->AddSlot()
			.AutoHeight()
			.MaxHeight(400)
			.Padding(0, 0, 0, 2)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				//.AutoWidth()
				//.HAlign(HAlign_Center)
				//.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.HeightOverride(576)
					[
						SNew(SScaleBox)
						//.HAlign(EHorizontalAlignment::HAlign_Center)
						//.VAlign(EVerticalAlignment::VAlign_Center)
						.StretchDirection(EStretchDirection::Both)
						.Stretch(EStretch::ScaleToFill)
						[
							SAssignNew(ModelImage,SImage).Image(Texture->GetSlateBrush())
						]
					]
				]
			];
		}
	}

	RootNode->AddSlot()
		.AutoHeight()
		.Padding(2, 0, 0, 2)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("SSketchfabAssetWindow_DownloadModel", "Download Model"))
			.OnClicked(this, &SSketchfabAssetWindow::DownloadModel)
			.IsEnabled(this, &SSketchfabAssetWindow::OnDownloadEnabled)
		];

	RootNode->AddSlot()
		.FillHeight(1.0)
		.Padding(2, 2, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SUniformGridPanel)
				.SlotPadding(10)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Model Information"))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						+ SUniformGridPanel::Slot(0, 0)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Vertex Count:"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						.HAlign(EHorizontalAlignment::HAlign_Right)
						[
							SAssignNew(VertexCountText, STextBlock)
							.Text(FText::FromString(""))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						+ SUniformGridPanel::Slot(0, 0)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Face Count:"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						.HAlign(EHorizontalAlignment::HAlign_Right)
						[
							SAssignNew(FaceCountText, STextBlock)
							.Text(FText::FromString(""))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						+ SUniformGridPanel::Slot(0, 0)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Animation:"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						.HAlign(EHorizontalAlignment::HAlign_Right)
						[
							SAssignNew(AnimatedText, STextBlock)
							.Text(FText::FromString(""))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						+ SUniformGridPanel::Slot(0, 0)
						.HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(STextBlock)
							.Text(FText::FromString("File Size:"))
						]
						+ SUniformGridPanel::Slot(1, 0)
						.HAlign(EHorizontalAlignment::HAlign_Right)
						[
							SAssignNew(DownloadSizeText, STextBlock)
							.Text(GetFileSize())
						]
					]
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(STextBlock)
						.Text(FText::FromString("License Information"))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 2)
					[
						SAssignNew(LicenceText, STextBlock)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 2)
					[
						SAssignNew(ExtraInfoText, STextBlock)
						.AutoWrapText(true)
					]
				]
			]
		];


	ChildSlot
	[
		RootNode
	];
}

void SSketchfabAssetWindow::SetModelInfo(const FSketchfabTask& InTask)
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.SetUseGrouping(false);
	FormattingOptions.SetMaximumFractionalDigits(2);

	if (InTask.TaskData.ModelVertexCount < 1000)
	{
		static const FText VertexCountBytes = LOCTEXT("VertexCountBytes", "{0}");
		VertexCountText->SetText(FText::Format(VertexCountBytes, FText::AsNumber(InTask.TaskData.ModelVertexCount)));
	}
	else
	{
		static const FText VertexCountKiloBytes = LOCTEXT("VertexCountKiloBytes", "{0}k");
		VertexCountText->SetText(FText::Format(VertexCountKiloBytes, FText::AsNumber(InTask.TaskData.ModelVertexCount / 1000.0, &FormattingOptions)));
	}

	if (InTask.TaskData.ModelFaceCount < 1000)
	{
		static const FText FaceCountBytes = LOCTEXT("FaceCountBytes", "{0}");
		FaceCountText->SetText(FText::Format(FaceCountBytes, FText::AsNumber(InTask.TaskData.ModelFaceCount)));
	}
	else
	{
		static const FText FaceCountKiloBytes = LOCTEXT("FaceCountKiloBytes", "{0}k");
		FaceCountText->SetText(FText::Format(FaceCountKiloBytes, FText::AsNumber(InTask.TaskData.ModelFaceCount / 1000.0, &FormattingOptions)));
	}


	if (InTask.TaskData.AnimationCount > 0)
	{
		AnimatedText->SetText(FText::FromString("Yes"));
	}
	else
	{
		AnimatedText->SetText(FText::FromString("No"));
	}

	LicenceText->SetText(FText::FromString(InTask.TaskData.LicenceType));
	ExtraInfoText->SetText(FText::FromString(InTask.TaskData.LicenceInfo));
}


void SSketchfabAssetWindow::SetThumbnail(const FSketchfabTask& InTask)
{
	FSketchfabAssetData dataAdjusted = AssetData;
	dataAdjusted.ThumbUID = FName(*InTask.TaskData.ThumbnailUID_1024);
	FDeferredCleanupSlateBrush* Texture = NULL;
	AssetThumbnailPool->AccessTexture(dataAdjusted, InTask.TaskData.ThumbnailWidth_1024, InTask.TaskData.ThumbnailHeight_1024, &Texture);


	if (ModelImage.IsValid() && Texture)
	{
		ModelImage->SetImage(Texture->GetSlateBrush());
	}
}

FText SSketchfabAssetWindow::GetFileSize()
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.SetUseGrouping(false);
	FormattingOptions.SetMaximumFractionalDigits(2);
	float modelSize = AssetData.ModelSize;

	if (modelSize < 1e3)
	{
		return FText::Format(FText::FromString("{0} Bytes"), FText::AsNumber(modelSize));
	}
	else if (modelSize >= 1e3 && modelSize < 1e6)
	{
		modelSize = modelSize / 1e3;
		return FText::Format(FText::FromString("{0} KB"), FText::AsNumber(modelSize, &FormattingOptions));
	}
	else if (modelSize >= 1e6 && modelSize < 1e9)
	{
		modelSize = modelSize / 1e6;
		return FText::Format(FText::FromString("{0} MB"), FText::AsNumber(modelSize, &FormattingOptions));
	}
	else
	{
		modelSize = modelSize / 1e9;
		return FText::Format(FText::FromString("{0} GB"), FText::AsNumber(modelSize, &FormattingOptions));
	}
}


FReply SSketchfabAssetWindow::DownloadModel()
{
	OnDownloadRequest.Execute(AssetData.ModelUID.ToString(), AssetData.ModelPublishedAt);
	if (Window.IsValid())
	{
		Window.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SSketchfabAssetWindow::ViewOnSketchfab()
{
	SSketchfabWindow::OpenUrlInBrowser("https://sketchfab.com/models/" + AssetData.ModelUID.ToString());
	return FReply::Handled();
}

bool SSketchfabAssetWindow::OnDownloadEnabled() const
{
	//TODO
	//If its already on disk then the button is not active
	//If the file is currently being downloaded or the user has pressed the download button then its not active.
	return true;
}


#undef LOCTEXT_NAMESPACE

