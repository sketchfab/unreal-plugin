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
#include "SketchfabRESTClient.h"
#include "SScaleBox.h"

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
		.AutoHeight()
		.Padding(2, 2, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(AssetData.ModelName.ToString()))
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
		];
	RootNode->AddSlot()
		.AutoHeight()
		.Padding(2, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(AssetData.AuthorName.ToString()))
		];



	if (AssetThumbnailPool.IsValid())
	{
		FSlateDynamicImageBrush* Texture = NULL;
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
							SAssignNew(ModelImage,SImage).Image(Texture)
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
							.Text(FText::FromString(""))
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
	VertexCountText->SetText(FText::AsNumber(InTask.TaskData.ModelVertexCount));
	FaceCountText->SetText(FText::AsNumber(InTask.TaskData.ModelFaceCount));
	
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

	DownloadSize = InTask.TaskData.ModelSize / 1000.0f;
	DownloadSizeText->SetText(FText::Format(FText::FromString("{0} KB"), FText::AsNumber(DownloadSize)));
}


void SSketchfabAssetWindow::SetThumbnail(const FSketchfabTask& InTask)
{
	FSketchfabAssetData dataAdjusted = AssetData;
	dataAdjusted.ThumbUID = FName(*InTask.TaskData.ThumbnailUID_1024);
	FSlateDynamicImageBrush* Texture = NULL;
	AssetThumbnailPool->AccessTexture(dataAdjusted, InTask.TaskData.ThumbnailWidth_1024, InTask.TaskData.ThumbnailHeight_1024, &Texture);

	if (ModelImage.IsValid() && Texture)
	{
		ModelImage->SetImage(Texture);
	}
}

FReply SSketchfabAssetWindow::DownloadModel()
{
	OnDownloadRequest.Execute(AssetData.ModelUID.ToString());
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

