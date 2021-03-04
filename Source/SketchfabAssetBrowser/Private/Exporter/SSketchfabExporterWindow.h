// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "SSketchfabWindow.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabExporterWindow, Log, All);

class SSketchfabExporterWindow : public SSketchfabWindow
{
public:

	DECLARE_DELEGATE(ExporterDelegate);
	ExporterDelegate& OnModelZipped() { return OnModelZippedDelegate; }

	SLATE_BEGIN_ARGS(SSketchfabExporterWindow){}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	// Window creation
	TSharedRef<SVerticalBox> CreateUploadArea();
	void CreateHorizontalField(TSharedRef<SVerticalBox> VBox, FString LText, TSharedRef<SCompoundWidget> Widget, float BPadding = 0.0f, bool AutoHeight = true);

	// Upload procedures
	ExporterDelegate OnModelZippedDelegate;
	FReply OnUploadButtonPressed();
	void Upload();
	float PlanUploadLimit(FString UserPlan);
	bool ValidateArchiveSize(FString path, FString userPlan);

	// Helpers
	void CleanUploadArtifacts();
	void OnUploadSuccess(const FSketchfabTask& InTask);
	void OnUploadFailed(const FSketchfabTask& InTask);

	// UI callbacks
	ECheckBoxState IsSelectedChecked() const;
	ECheckBoxState IsBakeChecked() const;
	ECheckBoxState IsDraftChecked() const;
	ECheckBoxState IsPrivateChecked() const;
	void OnSelectedCheckStateChanged(ECheckBoxState NewState);
	void OnBakeCheckStateChanged(ECheckBoxState NewState);
	void OnDraftCheckStateChanged(ECheckBoxState NewState);
	void OnPrivateCheckStateChanged(ECheckBoxState NewState);
	void OnTitleChanged(const FText& InText, ETextCommit::Type Type);
	void OnDescriptionChanged(const FText& InText, ETextCommit::Type Type);
	void OnTagsChanged(const FText& InText, ETextCommit::Type Type);
	void OnPasswordChanged(const FText& InText, ETextCommit::Type Type);
	FText GetUploadButtonText() const;
	bool IsPasswordEnabled() const;

private:
	
	// References to UI elements
	TSharedPtr<SEditableTextBox>          pTitleEditableBox;
	TSharedPtr<SMultiLineEditableTextBox> pDescriptionEditableBox;
	TSharedPtr<SEditableTextBox>          pTagsEditableBox;
	TSharedPtr<SEditableTextBox>          pPasswordEditableBox;
	TSharedPtr<SButton>                   pUploadButton;
	
	// UI elements values
	bool bDraft;
	bool bPrivate;
	bool bBakeMaterials;
	bool bSelectedOnly;
	FString modelTitle;
	FString modelDescription;
	FString modelTags;
	FString modelPassword;
	FString uploadedModelUri;

	// Upload artifacts
	FString GlbPath;
	FString ZipPath;
};

