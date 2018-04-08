// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once 

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SSketchfabAssetSearchBox.h"
#include "SSketchfabAssetWindow.h"
#include "SOAuthWebBrowser.h"

//#include "../WebBrowser/Public/IWebBrowserWindow.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowserWindow, Log, All);

class SSketchfabAssetBrowserWindow : public SCompoundWidget
{
public:
	~SSketchfabAssetBrowserWindow();

	SLATE_BEGIN_ARGS(SSketchfabAssetBrowserWindow)
	{}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)

	SLATE_END_ARGS()

public:
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
	void Construct(const FArguments& InArgs);

	//Main Window
	FReply OnLogin();
	FReply OnLogout();

	bool OnLoginEnabled() const;
	bool OnLogoutEnabled() const;

	FReply OnCancel();
	FReply OnNext();
	FReply OnDownloadSelected();
	FReply OnClearCache();
	
	//Search Box
	FReply OnSearchPressed();
	bool SetSearchBoxText(const FText& InSearchText);
	void OnSearchBoxChanged(const FText& InSearchText);
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	//Login Button 
	FText GetLoginButtonText() const;

	//Browser Window
	void OnUrlChanged(const FText &url);

	//AssetView 
	void OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FSketchfabAssetData>& SelectedAssets);

public:
	void Search();
	void GetUserData();
	void GetCategories();

private:
	void Search(const FString &url);
	void DownloadModel(const FString &ModelUID);
	void ShowModelWindow(const FSketchfabAssetData& AssetData);
	void DoLoginLogout(const FString &url);

private: 	

	enum ESortBy {
		SORTBY_Relevance,
		SORTBY_MostLiked,
		SORTBY_MostViewed,
		SORTBY_MostRecent,
		SORTBY_UNDEFINED,
	};

	enum EFaceCount {
		FACECOUNT_ALL,
		FACECOUNT_0_10,
		FACECOUNT_10_50,
		FACECOUNT_50_100,
		FACECOUNT_100_250,
		FACECOUNT_250,
		FACECOUNT_UNDEFINED,
	};

	//Search Options
	TSharedRef<SWidget> MakeCategoriesMenu();
	TSharedRef<SWidget> MakeSortByMenu();
	TSharedRef<SWidget> MakeFaceCountMenu();
	void AddFaceCountWidget(FMenuBuilder &MenuBuilder, EFaceCount fc);
	void AddSortByWidget(FMenuBuilder &MenuBuilder, ESortBy fc);
	void AddCategoryWidget(FMenuBuilder &MenuBuilder, int32 CategoryIndex);

	ECheckBoxState IsSearchAnimatedChecked() const;
	void OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState);

	ECheckBoxState IsSearchStaffPickedChecked() const;
	void OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState);

	void HandleCategoryStateChanged(ECheckBoxState NewRadioState, int32 NewCategoryIndex)
	{
		if (NewRadioState == ECheckBoxState::Checked)
		{
			CategoryIndex = NewCategoryIndex;
		}
		CategoryText->SetText(GetCategoryText());
	}

	ECheckBoxState HandleCategoryIsChecked(int32 NewCategoryIndex) const
	{
		return (CategoryIndex == NewCategoryIndex)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}

	FText GetCategoryText(int32 CustomIndex = -2) const
	{
		int32 val = CategoryIndex;
		if (CustomIndex != -2)
		{
			val = CustomIndex;
		}
		
		if (val >= 0 && val < Categories.Num())
		{
			return FText::FromString(Categories[val].name);
		}
		
		return FText::FromString("All");
	}

	void HandleSortByTypeStateChanged(ECheckBoxState NewRadioState, ESortBy NewSortByType)
	{
		if (NewRadioState == ECheckBoxState::Checked)
		{
			SortByType = NewSortByType;
		}
		SortByText->SetText(GetSortByText());
	}

	ECheckBoxState HandleSortByTypeIsChecked(ESortBy NewSortByType) const
	{
		return (SortByType == NewSortByType)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}

	FText GetSortByText(ESortBy custom = SORTBY_UNDEFINED) const
	{
		ESortBy val = SortByType;
		if (custom != SORTBY_UNDEFINED)
		{
			val = custom;
		}
		switch (val)
		{
		case SORTBY_Relevance:
		{
			return FText::FromString("Relevance");
		}
		break;
		case SORTBY_MostLiked:
		{
			return FText::FromString("Most Liked");
		}
		break;
		case SORTBY_MostViewed:
		{
			return FText::FromString("Most Viewed");
		}
		break;
		case SORTBY_MostRecent:
		{
			return FText::FromString("Most Recent");
		}
		break;
		}
		return FText::FromString("");
	}

	void HandleFaceCountStateChanged(ECheckBoxState NewRadioState, EFaceCount NewFaceCount)
	{
		if (NewRadioState == ECheckBoxState::Checked)
		{
			FaceCount = NewFaceCount;
		}
		FaceCountText->SetText(GetFaceCountText());
	}

	ECheckBoxState HandleFaceCountChecked(EFaceCount NewFaceCount) const
	{
		return (FaceCount == NewFaceCount)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}

	FText GetFaceCountText(EFaceCount custom = FACECOUNT_UNDEFINED) const
	{
		EFaceCount val = FaceCount;
		if (custom != FACECOUNT_UNDEFINED)
		{
			val = custom;
		}

		switch (val)
		{
		case FACECOUNT_ALL:
		{
			return FText::FromString("All");
		}
		break;
		case FACECOUNT_0_10:
		{
			return FText::FromString("Up to 10k");
		}
		break;
		case FACECOUNT_10_50:
		{
			return FText::FromString("10k to 50k");
		}
		break;
		case FACECOUNT_50_100:
		{
			return FText::FromString("50k to 100k");
		}
		break;
		case FACECOUNT_100_250:
		{
			return FText::FromString("100k to 250k");
		}
		break;
		case FACECOUNT_250:
		{
			return FText::FromString("250k+");
		}
		break;
		}
		return FText::FromString("");
	}

	bool bSearchAnimated;
	bool bSearchStaffPicked;
	TArray<FSketchfabCategory> Categories;
	ESortBy SortByType;
	EFaceCount FaceCount;
	int32 CategoryIndex;

private:
	FString Token;

	//SketchfabRESTClient callbacks
	void OnSearch(const FSketchfabTask& InTask);
	void OnThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnModelLink(const FSketchfabTask& InTask);
	void OnModelDownloaded(const FSketchfabTask& InTask);
	void OnModelDownloadProgress(const FSketchfabTask& InTask);
	void OnUserData(const FSketchfabTask& InTask);
	void OnUserThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnCategories(const FSketchfabTask& InTask);

	void OnTaskFailed(const FSketchfabTask& InTask);
	void OnDownloadFailed(const FSketchfabTask& InTask);

	//Call for the Asset Window
	void GetModelInfo(const FString &ModelUID);
	void GetBigThumbnail(const FSketchfabTaskData &data);

	//Callbacks for the Asset Window
	void OnGetModelInfo(const FSketchfabTask& InTask);
	void OnGetBigThumbnail(const FSketchfabTask& InTask);

	// The Asset Window is telling the main window to download a file
	void OnDownloadRequest(const FString &ModelUID);

	void OnOAuthWindowClosed(const TSharedRef<SWindow>& InWindow);

private:
	TWeakPtr<SWindow> Window;

	TSharedPtr<SWindow> OAuthWindowPtr;
	TSharedPtr<SSketchfabAssetWindow> AssetWindow;

	TSharedPtr<STextBlock> FaceCountText;
	TSharedPtr<STextBlock> SortByText;
	TSharedPtr<STextBlock> CategoryText;

	TSharedPtr<SSketchfabAssetView> AssetViewPtr;

	/** The text box used to search for assets */
	TSharedPtr<SSketchfabAssetSearchBox> SearchBoxPtr;

	FString QuerySearchText;

	FString CacheFolder;

	FString NextURL;

	FString LoggedInUser;

	TSet<FString> ModelsDownloading;
};

