// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once 

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SSketchfabAssetSearchBox.h"

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

public:
	//void CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID);
	void ForceCreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, const FString& ModelAssetUID, const FString& ThumbAssetUID);

private: 	

	enum ESortBy {
		SORTBY_Relevance,
		SORTBY_MostLiked,
		SORTBY_MostViewed,
		SORTBY_MostRecent
	};

	enum EMaxPolyCount {
		MAXPOLYCOUNT_ALL,
		MAXPOLYCOUNT_100,
		MAXPOLYCOUNT_1000,
		MAXPOLYCOUNT_10000,
		MAXPOLYCOUNT_100000,
		MAXPOLYCOUNT_1000000,
	};

	//Search Options
	TSharedRef<SWidget> MakeCategoriesMenu();
	TSharedRef<SWidget> MakeSortByMenu();
	TSharedRef<SWidget> MakeMaxPolyCountMenu();

	ECheckBoxState IsSearchAnimatedChecked() const;
	void OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState);

	ECheckBoxState IsSearchStaffPickedChecked() const;
	void OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState);


	TSharedRef<SWidget> CreateCheckBox(const FText& CheckBoxText, bool* CheckBoxChoice);

	// Callback for changing the checked state of a check box.
	void HandleCategoryCheckBoxCheckedStateChanged(ECheckBoxState NewState, bool* CheckBoxThatChanged);

	// Callback for determining whether a check box is checked.
	ECheckBoxState HandleCategoryCheckBoxIsChecked(bool* CheckBox) const;

	void HandleSortByTypeStateChanged(ECheckBoxState NewRadioState, ESortBy NewSortByType)
	{
		if (NewRadioState == ECheckBoxState::Checked)
		{
			SortByType = NewSortByType;
		}
	}

	ECheckBoxState HandleSortByTypeIsChecked(ESortBy NewSortByType) const
	{
		return (SortByType == NewSortByType)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}

	FText GetSortByText() const
	{
		switch (SortByType)
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
		return FText::FromString("Sort By");
	}

	void HandleMaxPolyCountStateChanged(ECheckBoxState NewRadioState, EMaxPolyCount NewMaxPolyCount)
	{
		if (NewRadioState == ECheckBoxState::Checked)
		{
			MaxPolyCount = NewMaxPolyCount;
		}
	}

	ECheckBoxState HandleMaxPolyCountIsChecked(EMaxPolyCount NewMaxPolyCount) const
	{
		return (MaxPolyCount == NewMaxPolyCount)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}

	FText GetMaxPolyCountText() const
	{
		switch (MaxPolyCount)
		{
		case MAXPOLYCOUNT_ALL:
		{
			return FText::FromString("All Models Sizes");
		}
		break;
		case MAXPOLYCOUNT_100:
		{
			return FText::FromString("100");
		}
		break;
		case MAXPOLYCOUNT_1000:
		{
			return FText::FromString("1,000");
		}
		break;
		case MAXPOLYCOUNT_10000:
		{
			return FText::FromString("10,000");
		}
		break;
		case MAXPOLYCOUNT_100000:
		{
			return FText::FromString("100,000");
		}
		break;
		case MAXPOLYCOUNT_1000000:
		{
			return FText::FromString("1,000,000");
		}
		break;
		}
		return FText::FromString("Max Poly Count");
	}

	bool bSearchAnimated;
	bool bSearchStaffPicked;
	TArray<TSharedPtr< struct FSketchfabCategory>> Categories;
	ESortBy SortByType;
	EMaxPolyCount MaxPolyCount;

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

private:
	TWeakPtr<SWindow> OAuthWindowPtr;

	TWeakPtr< SWindow > Window;

	TSharedPtr<SSketchfabAssetView> AssetViewPtr;

	/** The text box used to search for assets */
	TSharedPtr<SSketchfabAssetSearchBox> SearchBoxPtr;

	FString TagSearchText;

	FString CacheFolder;

	FString NextURL;

	FString LoggedInUser;

	TSet<FString> ModelsDownloading;
};

