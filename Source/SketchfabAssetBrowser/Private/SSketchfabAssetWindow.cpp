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

	RootNode->AddSlot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(AssetData.ModelName.ToString()))
		];
	RootNode->AddSlot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(AssetData.AuthorName.ToString()))
		];



	if (InArgs._ThumbnailPool.IsValid())
	{
		FSlateDynamicImageBrush* Texture = NULL;
		InArgs._ThumbnailPool->AccessTexture(AssetData, AssetData.ThumbnailWidth, AssetData.ThumbnailHeight, &Texture);
		// The viewport for the rendered thumbnail, if it exists
		if (Texture)
		{
			RootNode->AddSlot()
			.AutoHeight()
			.Padding(0, 0, 0, 2)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage).Image(Texture)
				]
			];
		}
	}

	RootNode->AddSlot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
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
					.Padding(0,2)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Model Information"))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(VertexCountText, STextBlock)
						.Text(FText::FromString("Vertex Count"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(FaceCountText, STextBlock)
						.Text(FText::FromString("Face Count"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
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
					.Padding(0, 2)
					[
						SNew(STextBlock)
						.Text(FText::FromString("License Information"))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(LicenceText, STextBlock)
						.Text(FText::FromString("Creative Common Attribution"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(ExtraInfoText, STextBlock)
						.Text(FText::FromString("More information here..."))
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
}

void SSketchfabAssetWindow::OnTaskFailed(const FSketchfabTask& InTask)
{

}


#undef LOCTEXT_NAMESPACE

