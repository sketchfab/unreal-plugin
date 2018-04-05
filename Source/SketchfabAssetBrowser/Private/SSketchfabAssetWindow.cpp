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
		.FillHeight(1.0)
		.Padding(2, 2, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SUniformGridPanel)
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
					.Padding(0, 0, 0, 2)
					.AutoHeight()
					[
						SAssignNew(VertexCountText, STextBlock)
						.Text(FText::FromString("Vertex Count"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 2)
					[
						SAssignNew(FaceCountText, STextBlock)
						.Text(FText::FromString("Face Count"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 2)
					[
						SAssignNew(AnimatedText, STextBlock)
						.Text(FText::FromString("Animation"))
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

	GetModelInfo();
}

void SSketchfabAssetWindow::GetModelInfo()
{
	FSketchfabTaskData TaskData;
	TaskData.CacheFolder = GetSketchfabCacheDir();
	TaskData.ModelUID = AssetData.ModelUID.ToString();
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETMODELINFO);
	Task->OnModelInfo().BindRaw(this, &SSketchfabAssetWindow::OnGetModelInfo);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}


void SSketchfabAssetWindow::OnGetModelInfo(const FSketchfabTask& InTask)
{
	VertexCountText->SetText(FText::Format(LOCTEXT("SSketchfabAssetWindow_VertexCount", "Vertex Count: {0}"), FText::AsNumber(InTask.TaskData.ModelVertexCount)));
	FaceCountText->SetText(FText::Format(LOCTEXT("SSketchfabAssetWindow_FaceCount", "Face Count: {0}"), FText::AsNumber(InTask.TaskData.ModelFaceCount)));
	
	if (InTask.TaskData.AnimationCount > 0)
	{
		AnimatedText->SetText(LOCTEXT("SSketchfabAssetWindow_AnimatedText", "Animated: Yes"));
	}
	else
	{
		AnimatedText->SetText(LOCTEXT("SSketchfabAssetWindow_AnimatedText", "Animated: No"));
	}

	LicenceText->SetText(FText::FromString(InTask.TaskData.LicenceType));
	ExtraInfoText->SetText(FText::FromString(InTask.TaskData.LicenceInfo));

	if (!InTask.TaskData.ThumbnailUID_1024.IsEmpty())
	{
		GetThumbnail(InTask.TaskData);
	}
}


void SSketchfabAssetWindow::GetThumbnail(const FSketchfabTaskData &data)
{
	FString jpg = data.ThumbnailUID_1024 + ".jpg";
	FString FileName = data.CacheFolder / jpg;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*FileName))
	{
		FSketchfabTaskData TaskData = data;
		TaskData.ThumbnailUID = data.ThumbnailUID_1024;
		TaskData.ThumbnailURL = data.ThumbnailURL_1024;
		TaskData.StateLock = new FCriticalSection();

		TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
		Task->SetState(SRS_GETTHUMBNAIL);
		Task->OnThumbnailDownloaded().BindRaw(this, &SSketchfabAssetWindow::OnGetThumbnail);
		Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetWindow::OnTaskFailed);
		FSketchfabRESTClient::Get()->AddTask(Task);
	}
	else
	{
		FSketchfabAssetData dataAdjusted = AssetData;
		dataAdjusted.ThumbUID = FName(*data.ThumbnailUID_1024);
		FSlateDynamicImageBrush* Texture = NULL;
		AssetThumbnailPool->AccessTexture(dataAdjusted, data.ThumbnailWidth_1024, data.ThumbnailHeight_1024, &Texture);
		ModelImage->SetImage(Texture);
	}
}



void SSketchfabAssetWindow::OnGetThumbnail(const FSketchfabTask& InTask)
{
	FSketchfabAssetData dataAdjusted = AssetData;
	dataAdjusted.ThumbUID = FName(*InTask.TaskData.ThumbnailUID_1024);
	FSlateDynamicImageBrush* Texture = NULL;
	AssetThumbnailPool->AccessTexture(dataAdjusted, InTask.TaskData.ThumbnailWidth_1024, InTask.TaskData.ThumbnailHeight_1024, &Texture);
	ModelImage->SetImage(Texture);
}


void SSketchfabAssetWindow::OnTaskFailed(const FSketchfabTask& InTask)
{

}


#undef LOCTEXT_NAMESPACE

