// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "SSketchfabWindow.h"

#include "SSketchfabAssetView.h"
#include "SSketchfabAssetSearchBox.h"
#include "SSketchfabAssetWindow.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowserWindow, Log, All);

class SSketchfabAssetBrowserWindow : public SSketchfabWindow
{
public:

	SLATE_BEGIN_ARGS(SSketchfabAssetBrowserWindow)
	{}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)

	SLATE_END_ARGS()

public:
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
	void Construct(const FArguments& InArgs);

	bool hasMoreResults() const;

	FReply OnNext();
	FReply OnDownloadSelected();

	FReply OnVisitStore();

	//Search Box
	FReply OnSearchPressed();
	bool SetSearchBoxText(const FText& InSearchText);
	void OnSearchBoxChanged(const FText& InSearchText);
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	//Login
	FText GetMyModelText() const;

	//Browser Window
	void OnUrlChanged(const FText &url);

	//AssetView
	void OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);
	void OnAssetsSelected(const FSketchfabAssetData &SelectedAsset);
	//TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<..>& SelectedAssets);

public:
	void Search();
	void GetCategories();
	void GetModelLicence(const FString &ModelUID);

private:
	void Search(const FString &url);
	void DownloadModel(const FString &ModelUID, const FDateTime &ModelPublishedAt);
	void ShowModelWindow(const FSketchfabAssetData& AssetData);
	void GetModelSize(const FString &ModelUID);

	FString GetModelZipFileName(const FString &ModelUID);

private:
	//Search Options
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

	enum ESearchDomain {
		SEARCHDOMAIN_Default,
		SEARCHDOMAIN_Own,
		SEARCHDOMAIN_Store,
		SEARCHDOMAIN_UNDEFINED,
	};

	void setDefaultParams();
	bool IsSearchMyModelsAvailable() const;
	ECheckBoxState IsSearchMyModelsChecked() const;
	void OnSearchMyModelsCheckStateChanged(ECheckBoxState NewState);

	EVisibility ShouldDisplayEmptyResults() const;
	EVisibility ShouldDisplayVisitStore() const;

	ECheckBoxState IsSearchAnimatedChecked() const;
	void OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState);

	ECheckBoxState IsSearchStaffPickedChecked() const;
	void OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState);

	bool bSearchAnimated;
	bool bSearchStaffPicked;

	// Search domain
	void GenerateSearchDomainList();
	void UpdateSearchDomainList();
	TSharedRef<SWidget> GenerateSearchDomainComboItem(TSharedPtr<FString> InItem);
	void HandleSearchDomainComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetSearchDomainComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SearchDomainComboBox;
	TArray<TSharedPtr<FString>> SearchDomainComboList;
	int32 SearchDomainIndex;
	FString CurrentSearchDomainString;
	EVisibility ShouldDisplayFilters() const;

	// Sort By
	TSharedRef<SWidget> GenerateSortByComboItem(TSharedPtr<FString> InItem);
	void HandleSortByComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetSortByComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SortByComboBox;
	TArray<TSharedPtr<FString>> SortByComboList;
	int32 SortByIndex;
	FString CurrentSortByString;

	// Category
	TSharedRef<SWidget> GenerateCategoryComboItem(TSharedPtr<FString> InItem);
	void HandleCategoryComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetCategoryComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> CategoriesComboBox;
	TArray<FSketchfabCategory> Categories;
	TArray<TSharedPtr<FString>> CategoryComboList;
	int32 CategoryIndex;
	FString CurrentCategoryString;

	// Face Count
	TSharedRef<SWidget> GenerateFaceCountComboItem(TSharedPtr<FString> InItem);
	void HandleFaceCountComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetFaceCountComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FaceCountComboBox;
	TArray<TSharedPtr<FString>> FaceCountComboList;
	int32 FaceCountIndex;
	FString CurrentFaceCountString;

	virtual void GenerateProjectComboItems() {};
	bool ShouldDownloadFile(const FString &FileName, const FDateTime &ModelPublishedAt);

private:

	//SketchfabRESTClient callbacks
	void OnSearch(const FSketchfabTask& InTask);
	void OnThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnModelLink(const FSketchfabTask& InTask);
	void OnModelDownloaded(const FSketchfabTask& InTask);
	void OnModelDownloadProgress(const FSketchfabTask& InTask);
	void OnUserThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnCategories(const FSketchfabTask& InTask);
	void OnGetModelSize(const FSketchfabTask& InTask);
	void OnDownloadFailed(const FSketchfabTask& InTask);

	//virtual void OnUseOrgProfileCheckStateChanged(ECheckBoxState NewState);
	virtual void OnOrgChanged();

	virtual EVisibility ShouldDisplayClearCache() const;
	virtual FReply OnClearCache();

	//Call for the Asset Window
	void GetModelInfo(const FString &ModelUID);
	void GetBigThumbnail(const FSketchfabTaskData &data);

	//Callbacks for the Asset Window
	void OnGetModelInfo(const FSketchfabTask& InTask);
	void OnGetBigThumbnail(const FSketchfabTask& InTask);

	// The Asset Window is telling the main window to download a file
	void OnDownloadRequest(const FString &ModelUID, const FString &ModelPublishedAt);

private:
	bool OnContentBrowserDrop(const FAssetViewDragAndDropExtender::FPayload &PayLoad);
	bool OnContentBrowserDragOver(const FAssetViewDragAndDropExtender::FPayload &PayLoad);
	bool OnContentBrowserDragLeave(const FAssetViewDragAndDropExtender::FPayload &PayLoad);

private:

	TSharedPtr<SSketchfabAssetWindow> AssetWindow;

	TSharedPtr<STextBlock> FaceCountText;

	TSharedPtr<SSketchfabAssetView> AssetViewPtr;

	/** The text box used to search for assets */
	TSharedPtr<SSketchfabAssetSearchBox> SearchBoxPtr;

	FString QuerySearchText;

	FString NextURL;

	int32 NResults;

	TSet<FString> ModelsDownloading;
	TMap<FString, uint64> ModelSizesMap;
};

