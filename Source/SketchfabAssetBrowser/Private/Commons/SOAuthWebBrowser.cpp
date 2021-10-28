#include "SOAuthWebBrowser.h"

#define LOCTEXT_NAMESPACE "SOAuthWebBrowser"

void SOAuthWebBrowser::Construct(const FArguments& InArgs)
{
	SWebBrowser::Construct(SWebBrowser::FArguments()
		.ParentWindow(InArgs._ParentWindow)
		.InitialURL(InArgs._InitialURL)
		.OnUrlChanged(InArgs._OnUrlChanged)
		.OnCloseWindow(InArgs._OnCloseWindow)
		.ShowControls(false)
		.ShowAddressBar(false));
}

#undef LOCTEXT_NAMESPACE
