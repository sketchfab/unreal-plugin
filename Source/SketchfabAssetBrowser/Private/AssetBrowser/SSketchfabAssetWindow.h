// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SSketchfabAssetSearchBox.h"
#include "SketchfabAssetThumbnail.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetWindow, Log, All);

DECLARE_DELEGATE_TwoParams(FOnDownloadRequest, const FString &, const FDateTime &);

class SSketchfabAssetWindow : public SCompoundWidget
{
public:
	~SSketchfabAssetWindow();

	SLATE_BEGIN_ARGS(SSketchfabAssetWindow)
	{}

	SLATE_EVENT(FOnDownloadRequest, OnDownloadRequest)

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_ARGUMENT(FSketchfabAssetData, AssetData)
	SLATE_ARGUMENT(TSharedPtr<FSketchfabAssetThumbnailPool>, ThumbnailPool)
	SLATE_END_ARGS()

public:
	//virtual bool SupportsKeyboardFocus() const override { return true; }
	//virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
	void Construct(const FArguments& InArgs);
	void SetModelInfo(const FSketchfabTask& InTask);
	void SetThumbnail(const FSketchfabTask& InTask);
	FText GetFileSize();

	FReply DownloadModel();
	FReply ViewOnSketchfab();
	bool OnDownloadEnabled() const;

private:
	TWeakPtr< SWindow > Window;
	TSharedPtr<STextBlock> VertexCountText;
	TSharedPtr<STextBlock> FaceCountText;
	TSharedPtr<STextBlock> AnimatedText;
	TSharedPtr<STextBlock> LicenceText;
	TSharedPtr<STextBlock> ExtraInfoText;
	TSharedPtr<STextBlock> DownloadSizeText;
	TSharedPtr<SImage> ModelImage;

	float DownloadSize;
	FString ModelURL;

	FSketchfabAssetData AssetData;
	FOnDownloadRequest OnDownloadRequest;

	TSharedPtr<FSketchfabAssetThumbnailPool> AssetThumbnailPool;
};

