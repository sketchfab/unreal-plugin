// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SKGLTFExportOptions.h"
#include "InputCoreTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

class SButton;

class SKGLTFEXPORTER_API SGLTFExportOptionsWindow : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SGLTFExportOptionsWindow)
		: _ExportOptions(nullptr)
		, _BatchMode()
		{}

		SLATE_ARGUMENT( USKGLTFExportOptions*, ExportOptions )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
		SLATE_ARGUMENT( FText, FullPath )
		SLATE_ARGUMENT( bool, BatchMode )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

	FReply OnExport();

	FReply OnExportAll();

	FReply OnCancel();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	bool ShouldExport() const;

	bool ShouldExportAll() const;

	SGLTFExportOptionsWindow();

	static void ShowDialog(USKGLTFExportOptions* ExportOptions, const FString& FullPath, bool bBatchMode, bool& bOutOperationCanceled, bool& bOutExportAll);

private:

	FReply OnResetToDefaultClick() const;

	USKGLTFExportOptions* ExportOptions;
	TSharedPtr<class IDetailsView> DetailsView;
	TWeakPtr<SWindow> WidgetWindow;
	TSharedPtr<SButton> ImportButton;
	bool bShouldExport;
	bool bShouldExportAll;
};
