// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once 

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SSketchfabAssetSearchBox.h"
#include "SketchfabAssetThumbnail.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetWindow, Log, All);

class SSketchfabAssetWindow : public SCompoundWidget
{
public:
	~SSketchfabAssetWindow();

	SLATE_BEGIN_ARGS(SSketchfabAssetWindow)
	{}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_ARGUMENT(FSketchfabAssetData, AssetData)
	SLATE_ARGUMENT(TSharedPtr<FSketchfabAssetThumbnailPool>, ThumbnailPool)
	SLATE_END_ARGS()

public:
	//virtual bool SupportsKeyboardFocus() const override { return true; }
	//virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
	void Construct(const FArguments& InArgs);

private:
	void GetModelInfo();
	void GetThumbnail(const FSketchfabTaskData &data);
	void OnGetModelInfo(const FSketchfabTask& InTask);
	void OnGetThumbnail(const FSketchfabTask& InTask);
	void OnTaskFailed(const FSketchfabTask& InTask);



private:
	TWeakPtr< SWindow > Window;
	TSharedPtr<STextBlock> VertexCountText;
	TSharedPtr<STextBlock> FaceCountText;
	TSharedPtr<STextBlock> AnimatedText;
	TSharedPtr<STextBlock> LicenceText;
	TSharedPtr<STextBlock> ExtraInfoText;
	TSharedPtr<SImage> ModelImage;

	FSketchfabAssetData AssetData;

	TSharedPtr<FSketchfabAssetThumbnailPool> AssetThumbnailPool;
};

