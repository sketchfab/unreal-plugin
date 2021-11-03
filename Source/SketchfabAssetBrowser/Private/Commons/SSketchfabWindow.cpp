// Copyright 2018 Sketchfab, Inc. All Rights Reserved.
#pragma once

#include "SSketchfabWindow.h"

#define LOCTEXT_NAMESPACE "SketchfabWindow"

#ifndef _WIN32
// Used for running platform specific commands
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
#endif

void SSketchfabWindow::OpenUrlInBrowser(FString Url)
{
	// Linux and Apple don't have an override for OsExecute
	#ifdef _WIN32
	bool success = FPlatformMisc::OsExecute(TEXT("open"), *Url);
	#else
	#ifdef __APPLE__
	std::string command = "open " + std::string(TCHAR_TO_UTF8(*Url));
	exec(command.c_str());
	#endif
	#ifdef __linux__
	std::string command = "xdg-open " + std::string(TCHAR_TO_UTF8(*Url));
	exec(command.c_str());
	#endif
	#endif
}

FString SSketchfabWindow::GetSketchfabCacheDir()
{
	return FPaths::EngineUserDir() + TEXT("SketchfabCache/");
}


EVisibility SSketchfabWindow::GetNewVersionButtonVisibility() const
{
	if (!LatestPluginVersion.IsEmpty() && LatestPluginVersion != CurrentPluginVersion)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Hidden;
	}
}

FText SSketchfabWindow::GetCurrentVersionText() const
{
	FString versionState;
	if(LatestPluginVersion.IsEmpty())
	{
		versionState = "";
	}
	else if (CurrentPluginVersion == LatestPluginVersion)
	{
		versionState = "(up to date)";
	}
	else
	{
		versionState = "(outdated)";
	}

	return FText::FromString("Version " + CurrentPluginVersion + " " + versionState);
}

FText SSketchfabWindow::GetLoginText() const
{
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		FString text = "Logged in as " + LoggedInUserDisplayName + " " + LoggedInUserAccountType;
        return FText::FromString(text);
	}
	else
	{
		return LOCTEXT("SSketchfabWindow_GetLoginText", "You're not logged in");
	}
}

FText SSketchfabWindow::GetLoginButtonText() const
{
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		return LOCTEXT("SSketchfabWindow_OnLogin", "Logout");
	}
	else
	{
		return LOCTEXT("SSketchfabWindow_OnLogin", "Login");
	}
}




FReply SSketchfabWindow::OnLogin()
{
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		Token = "";
		LoggedInUserName = "";
		LoggedInUserDisplayName = "";

		TSharedPtr<IWebBrowserCookieManager> cookieMan = IWebBrowserModule::Get().GetSingleton()->GetCookieManager();
		cookieMan->DeleteCookies("sketchfab.com");

		//FString url = "https://sketchfab.com/logout";
		//DoLoginLogout(url);
		return FReply::Handled();
	}
	else
	{
		FString url = "https://sketchfab.com/oauth2/authorize/?state=123456789&response_type=token&client_id=lZklEgZh9G5LcFqiCUUXTzXyNIZaO7iX35iXODqO";
		DoLoginLogout(url);
		return FReply::Handled();
	}

}

FReply SSketchfabWindow::OnUpgradeToPro()
{
	{
		OpenUrlInBrowser("https://sketchfab.com/plans?utm_source=unreal-plugin&utm_medium=plugin&utm_campaign=download-api-pro-cta");
		return FReply::Handled();
	}
}

void SSketchfabWindow::DoLoginLogout(const FString &url)
{
	if (OAuthWindowPtr.IsValid())
	{
		if (!isOauthWindowClosed)
		{
			OAuthWindowPtr->BringToFront();
			return;
		}
		else
		{
			OAuthWindowPtr.Reset();
		}
	}


	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	isOauthWindowClosed = false;
	OAuthWindowPtr = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetBrowser_LoginWindow", "Sketchfab Login Window"))
		.ClientSize(FVector2D(625, 800));

	OAuthWindowPtr->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &SSketchfabWindow::OnOAuthWindowClosed));

	TSharedPtr<SOAuthWebBrowser> OAuthBrowser;
	OAuthWindowPtr->SetContent
	(
		SAssignNew(OAuthBrowser, SOAuthWebBrowser)
		.ParentWindow(OAuthWindowPtr)
		.InitialURL(url)
		.OnUrlChanged(this, &SSketchfabWindow::OnUrlChanged)
	);

	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	FSlateApplication::Get().AddWindowAsNativeChild(OAuthWindowPtr.ToSharedRef(), CurrentWindow.ToSharedRef());
}

void SSketchfabWindow::OnOAuthWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	//FIXME (AurL): this is a dirty hack to fix OAuth window. There should be a better way to handle it's creation/destruction
	// Symptoms: Calling RequestDestroyWindow and then reseting the TSharedPtr here causes crash (data deletion occurs after call to this function)
	// Using DestroyWindowImmediately() doesn't crash but the OAuth window is still grabbing the focus and parent windows widgets are not accessible in its area
	isOauthWindowClosed = true;
}

void SSketchfabWindow::OnUrlChanged(const FText &url)
{
	FString data = url.ToString();

	//If successfully logging in and access given based on the client_id then my redirect URL will be sent here along with
	//an authorization code. I then get the code from the URL and use that to request an Access Token, which will have an expiration time.
	//I then store this Access Token and the expiration time and use the token for all further communication with the server.

	//https://sketchfab.com/developers/oauth#registering-your-app

	//BEFORE the token expires I can use the "refresh token", which comes with the access token, to obtain a new access token before it expires.

	//https://sketchfab.com/oauth2/success#access_token=DsFGAAK86kLQ1gfUjQQD6gLPhNP6lm&token_type=Bearer&state=123456789&expires_in=36000&scope=read+write


	FString leftBlock = TEXT("https://sketchfab.com/oauth2/success#");
	if (data.Contains(leftBlock))
	{
		FString params = data.RightChop(leftBlock.Len());

		FString accesstoken;
		FString tokentype;
		FString state;
		FString expiresin;
		FString scope;

		TArray<FString> parameters;
		if (params.ParseIntoArray(parameters, TEXT("&"), true) > 0)
		{
			for (int a = 0; a < parameters.Num(); a++)
			{
				const FString &param = parameters[a];

				FString left, right;
				if (param.Split(TEXT("="), &left, &right))
				{
					if (left == "access_token")
						accesstoken = right;
					else if (left == "token_type")
						tokentype = right;
					else if (left == "state")
						state = right;
					else if (left == "expires_in")
						expiresin = right;
					else if (left == "scope")
						scope = right;
				}
			}
		}

		if (accesstoken.Len() != 0)
		{
			Token = accesstoken;
			GetUserData();
			GetUserOrgs();
			if (OAuthWindowPtr.IsValid())
			{
				OAuthWindowPtr->RequestDestroyWindow();
			}
		}
		return;
	}

	leftBlock = TEXT("https://sketchfab.com/?logged_out=1");
	if (data.Contains(leftBlock))
	{
		if (OAuthWindowPtr.IsValid())
		{
			OAuthWindowPtr->RequestDestroyWindow();
			OAuthWindowPtr = NULL;
		}
	}
	return;
}

FReply SSketchfabWindow::OnLogout()
{
	Token = "";
	LoggedInUserName = "";
	LoggedInUserDisplayName = "";
	UsesOrgProfile = false;
	CurrentOrgString = "";
	CurrentProjectString = "";
	OrgIndex = 0;
	ProjectIndex = 0;

	TSharedPtr<IWebBrowserCookieManager> cookieMan = IWebBrowserModule::Get().GetSingleton()->GetCookieManager();
	cookieMan->DeleteCookies("sketchfab.com");

	return FReply::Handled();
}

bool SSketchfabWindow::OnLoginEnabled() const
{
	if (Token.IsEmpty() || LoggedInUserDisplayName.IsEmpty())
	{
		return true;
	}
	return false;
}

bool SSketchfabWindow::OnLogoutEnabled() const
{
	return !OnLoginEnabled();
}

bool SSketchfabWindow::IsUserPro() const
{
	return OnLogoutEnabled() && IsLoggedUserPro;
}

FReply SSketchfabWindow::OnCancel()
{
	if (Window.IsValid())
	{
		Window.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}



TSharedRef<SVerticalBox> SSketchfabWindow::CreateLoginArea(bool isBrowser) {

	OrgIndex = 0;
	CurrentOrgString = TEXT("None");

	TSharedRef<SVerticalBox> loginArea = SNew(SVerticalBox);

	FString desc;
	if (isBrowser) {
		desc = "1. Login to your Sketchfab account.\n2. Select a model and click Download Selected to download (double click to view model information)\n3. After downloading you can import by click+dragging it into the content browser.";
	}
	else {
		desc = "1. Login to your Sketchfab account.\n2. Select a static mesh to export  in the current level.\n3. Fill the model information in the form below.\n4. Press \"Upload to Sketchfab\" to upload your model.";
	}

	loginArea->AddSlot()
	.AutoHeight()
	.Padding(0, 0, 0, 2)
	[
		SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[

			SNew(SHorizontalBox)

			// Left side (use organization profile)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			//.MaxWidth(200.0f)
			//.FillWidth(1.0f)
			//.Padding(12.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SVerticalBox)
				.Visibility(this, &SSketchfabWindow::GetOrgCheckboxVisibility)
				
				// Checkbox
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SCheckBox)
					//.Visibility(this, &SSketchfabWindow::GetOrgCheckboxVisibility)
					.OnCheckStateChanged(this, &SSketchfabWindow::OnUseOrgProfileCheckStateChanged)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Search_Animated", "Use organization profile ?"))
					]
				]

				// Org dropdown
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SAssignNew(OrgsComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&OrgComboList)
					.OnGenerateWidget(this, &SSketchfabWindow::GenerateOrgComboItem)
					.OnSelectionChanged(this, &SSketchfabWindow::HandleOrgComboChanged)
					.Visibility(this, &SSketchfabWindow::GetOrgDropdownVisibility)
					[
						SNew(STextBlock)
						.Text(this, &SSketchfabWindow::GetOrgComboText)
					]
				]
			]

		
			// Right side (login text and button)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			//.MaxWidth(250.0f)
			//.FillWidth(0.5f)
			//.Padding(12.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)

					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Margin(FMargin(0.0f, 5.0f, 0.0f, 0.0f))
						.Text(this, &SSketchfabWindow::GetLoginText)
						.MinDesiredWidth(200.0f)
					]

					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(this, &SSketchfabWindow::GetLoginButtonText)
						.OnClicked(this, &SSketchfabWindow::OnLogin)
					]
				]
			]
		]
	];

	return loginArea;
}


void SSketchfabWindow::OnUseOrgProfileCheckStateChanged(ECheckBoxState NewState) {
	UsesOrgProfile = (NewState == ECheckBoxState::Checked);
	OnOrgChanged();
}
bool SSketchfabWindow::UsesOrgProfileChecked() const {
	return UsesOrgProfile;
}

// Orgs
TSharedRef<SWidget> SSketchfabWindow::GenerateOrgComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}

void SSketchfabWindow::HandleOrgComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < OrgComboList.Num(); i++)
	{
		if (Item == OrgComboList[i])
		{
			CurrentOrgString = *Item.Get();
			OrgIndex = i;
			
			if(Orgs[i].Projects.Num() > 0)
				GetOrgsProjects(&(Orgs[i]));

			UpdateAvailableProjects();
		}
	}

	OnOrgChanged();

	GenerateProjectComboItems();
}

FText SSketchfabWindow::GetOrgComboText() const
{
	return FText::FromString(CurrentOrgString);
}


// Projects
TSharedRef<SWidget> SSketchfabWindow::GenerateProjectComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}
void SSketchfabWindow::HandleProjectComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < ProjectComboList.Num(); i++)
	{
		if (Item == ProjectComboList[i])
		{
			CurrentProjectString = *Item.Get();
			ProjectIndex = i;
		}
	}
}
FText SSketchfabWindow::GetProjectComboText() const
{
	return FText::FromString(CurrentProjectString);
}
EVisibility SSketchfabWindow::GetProjectDropdownVisibility() const
{
	return ( UsesOrgProfile && (LoggedInUserName !="") ) ? EVisibility::Visible : EVisibility::Collapsed;
}



EVisibility SSketchfabWindow::GetOrgCheckboxVisibility() const
{
	return ( Orgs.Num() > 0 && (LoggedInUserName != "") ) ? EVisibility::Visible : EVisibility::Collapsed;
}
EVisibility SSketchfabWindow::GetOrgDropdownVisibility() const
{
	return ((Orgs.Num() > 0) && (UsesOrgProfile) && (LoggedInUserName != "")) ? EVisibility::Visible : EVisibility::Collapsed;
}


TSharedRef<SVerticalBox> SSketchfabWindow::CreateFooterArea(bool isBrowser) {
	TSharedRef<SVerticalBox> FooterNode = SNew(SVerticalBox);

	// Version check
	FooterNode->AddSlot()
	[
		SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Left)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				.Visibility(this, &SSketchfabWindow::ShouldDisplayClearCache)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("SSketchfabAssetBrowserWindow_ClearCache", "Clear Cache"))
					.OnClicked(this, &SSketchfabWindow::OnClearCache)
				]
			]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.FillWidth(2.f)
			.Padding(2)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.Padding(2.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Visibility(this, &SSketchfabWindow::GetNewVersionButtonVisibility)
					.Text(FText::FromString("Update plugin"))
					.OnClicked(this, &SSketchfabWindow::GetLatestPluginVersion)
				]

				+ SHorizontalBox::Slot()
				.Padding(2.0f)
				.HAlign(HAlign_Fill)
				.FillWidth(1.2f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.Margin(FMargin(10.0, 4.0, 10.0, 2.0))
					.Text(this, &SSketchfabWindow::GetCurrentVersionText)
				]
			]
		]
	];

	return FooterNode;
}

EVisibility SSketchfabWindow::ShouldDisplayClearCache() const
{
	return EVisibility::Hidden;
}

FReply SSketchfabWindow::CheckLatestPluginVersion()
{
	FSketchfabTaskData TaskData;
	TaskData.StateLock = new FCriticalSection();
	TaskData.CacheFolder = CacheFolder;

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_CHECK_LATEST_VERSION);
	Task->OnCheckLatestVersion().BindRaw(this, &SSketchfabWindow::OnCheckLatestVersion);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabWindow::OnTaskFailed);
	Task->CheckLatestPluginVersion();
	FSketchfabRESTClient::Get()->AddTask(Task);

	return FReply::Handled();
}

FReply SSketchfabWindow::GetLatestPluginVersion()
{
	OpenUrlInBrowser("https://github.com/sketchfab/Sketchfab-Unreal/releases/latest");
	return FReply::Handled();
}


EVisibility SSketchfabWindow::ShouldDisplayUpgradeToPro() const
{
	return LoggedInUserName != "" && !IsLoggedUserPro ? EVisibility::Visible : EVisibility::Collapsed;
}


//=====================================================
// Direct REST API Calls
//=====================================================


void SSketchfabWindow::GetUserData()
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETUSERDATA);
	Task->OnUserData().BindRaw(this, &SSketchfabWindow::OnUserData);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabWindow::GetUserOrgs()
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETUSERORGS);
	Task->OnUserOrgs().BindRaw(this, &SSketchfabWindow::OnUserOrgs);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabWindow::GetOrgsProjects(FSketchfabOrg* org)
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();
	TaskData.org = org;

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETORGSPROJECTS);
	Task->OnOrgsProjects().BindRaw(this, &SSketchfabWindow::OnOrgsProjects);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

//=====================================================
// Callback API calls
//=====================================================

void SSketchfabWindow::OnTaskFailed(const FSketchfabTask& InTask)
{
}

void SSketchfabWindow::OnUserData(const FSketchfabTask& InTask)
{
	LoggedInUserName = InTask.TaskData.UserSession.UserName;
	LoggedInUserDisplayName = InTask.TaskData.UserSession.UserDisplayName;
	if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_PRO)
	{
		LoggedInUserAccountType = "(PRO)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_PREMIUM)
	{
		LoggedInUserAccountType = "(PREMIUM)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_BUSINESS)
	{
		LoggedInUserAccountType = "(BIZ)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_ENTERPRISE)
	{
		LoggedInUserAccountType = "(ENT)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_PLUS)
	{
		LoggedInUserAccountType = "(PLUS)";
		IsLoggedUserPro = false;
	}
	else
	{
		LoggedInUserAccountType = "(BASIC)";
		IsLoggedUserPro = false;
	}
}


void SSketchfabWindow::OnUserOrgs(const FSketchfabTask& InTask)
{
	Orgs = InTask.TaskData.Orgs;

	if (Orgs.Num() > 0) {

		OrgComboList.Empty();
		OrgIndex = 0;
		CurrentOrgString = Orgs[0].name;
		
		for (auto& org : Orgs) {
			GetOrgsProjects(&org);
			OrgComboList.Add(MakeShared<FString>(org.name));
		}

		OrgsComboBox->RefreshOptions();
		GenerateProjectComboItems();
	}
	
}

void SSketchfabWindow::UpdateAvailableProjects() {
	/*
	ProjectComboList.Empty();
	ProjectIndex = 0;
	ProjectComboList.Add(MakeShared<FString>("All organization"));
	for (auto& project : Orgs[OrgIndex].Projects)
	{
		ProjectComboList.Add(MakeShared<FString>(project.name));
	}
	ProjectComboBox->RefreshOptions();
	*/
}

void SSketchfabWindow::OnOrgsProjects(const FSketchfabTask& InTask)
{
	(InTask.TaskData.org)->Projects = InTask.TaskData.Projects;
	GenerateProjectComboItems();
}

void SSketchfabWindow::OnCheckLatestVersion(const FSketchfabTask& InTask)
{
	LatestPluginVersion = InTask.TaskData.LatestPluginVersion;
}

void SSketchfabWindow::initWindow() {
	//IE C:/Users/user/AppData/Local/UnrealEngine/4.18/SketchfabCache/
	CacheFolder = GetSketchfabCacheDir();
	CheckLatestPluginVersion();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CacheFolder))
	{
		PlatformFile.CreateDirectory(*CacheFolder);
	}

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Sketchfab");
	FPluginDescriptor descriptor = Plugin->GetDescriptor();
	CurrentPluginVersion = descriptor.VersionName;
}






void SPopUpWindow::Construct(const FArguments& InArgs)
{
	Window = InArgs._WidgetWindow;
	SubTitle = InArgs._SubTitle;
	ContentString = InArgs._ContentString;
	OKString = InArgs._OKString;
	CancelString = InArgs._CancelString;

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[

			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(2, 2, 2, 10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(SubTitle))
		.Justification(ETextJustify::Center)
		.AutoWrapText(true)
		.MinDesiredWidth(400.0f)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(2)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f))
		.Padding(FMargin(2))
		[
			SNew(SBox)
			.MinDesiredHeight(200.0f)
		.MinDesiredWidth(400.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
		.Text(FText::FromString(ContentString))
		//.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
		]
		]
		]
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(2)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(2)

		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.Text(FText::FromString(OKString))
		.OnClicked(this, &SPopUpWindow::OnOK)
		]

	+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.Text(FText::FromString(CancelString))
		.OnClicked(this, &SPopUpWindow::OnCancel)
		]
		]
		];
}
FReply SPopUpWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnCancel();
	}
	return FReply::Unhandled();
}
FReply SPopUpWindow::Close()
{
	if (Window.IsValid())
	{
		Window.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}
FReply SPopUpWindow::OnOK() {
	confirmed = true;
	return Close();
}
FReply SPopUpWindow::OnCancel() {
	confirmed = false;
	return Close();
}
bool SPopUpWindow::Confirmed() const
{
	return confirmed;
}





TSharedPtr<SPopUpWindow> SSketchfabWindow::CreatePopUp(FString title, FString subtitle, FString content, FString ok, FString cancel) {

	TSharedPtr<SPopUpWindow> PopUpWindow;

	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString(title))
		.SizingRule(ESizingRule::Autosized);

	NewWindow->SetContent
	(
		SAssignNew(PopUpWindow, SPopUpWindow)
		.WidgetWindow(NewWindow)
		.SubTitle(subtitle)
		.ContentString(content)
		.OKString(ok)
		.CancelString(cancel)
	);

	// Get the parent window
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	// Open the pop-up
	FSlateApplication::Get().AddModalWindow(NewWindow, ParentWindow, false);

	return PopUpWindow;
}



#undef LOCTEXT_NAMESPACE

