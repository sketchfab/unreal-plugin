// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SWebBrowser.h"

class SOAuthWebBrowser : public SWebBrowser
{
public:
	SLATE_BEGIN_ARGS(SOAuthWebBrowser)
	{}

	/** A reference to the parent window. */
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

	/** URL that the browser will initially navigate to. The URL should include the protocol, eg http:// */
	SLATE_ARGUMENT(FString, InitialURL)

	/** Called when the Url changes. */
	SLATE_EVENT(FOnTextChanged, OnUrlChanged)

	/** Called when a browser window close event is detected */
	SLATE_EVENT(FOnCloseWindowDelegate, OnCloseWindow)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
};
