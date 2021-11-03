#pragma once

#include "ISketchfabAssetBrowser.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SOAuthWebBrowser.h"
#include "Widgets/Input/SComboBox.h"
#include "ContentBrowserModule.h"

#include "Misc/ScopedSlowTask.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "PackageTools.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"

#include "Misc/FileHelper.h"
#include "Engine/Texture.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"

#include "EditorFramework/AssetImportData.h"

#include "Widgets/Layout/SSplitter.h"
#include "SketchfabRESTClient.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetToolsModule.h"
#include "IWebBrowserCookieManager.h"
#include "WebBrowserModule.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

#include "Interfaces/IPluginManager.h"

class SPopUpWindow : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPopUpWindow)
	{}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_ARGUMENT(FString, SubTitle)
	SLATE_ARGUMENT(FString, ContentString)
	SLATE_ARGUMENT(FString, OKString)
	SLATE_ARGUMENT(FString, CancelString)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	void Open();
	FReply Close();
	FReply OnOK();
	FReply OnCancel();
	bool Confirmed() const;

private:

	TWeakPtr< SWindow > Window;
	FString SubTitle;
	FString ContentString;
	FString OKString;
	FString CancelString;
	bool confirmed;
};

class SSketchfabWindow : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSketchfabWindow)
	{}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	void initWindow();
	~SSketchfabWindow(){FSketchfabRESTClient::Get()->Shutdown();}
	virtual bool SupportsKeyboardFocus() const override { return true; }

	// Login / Logout
	FReply OnLogin();
	FReply OnLogout();
	bool OnLoginEnabled() const;
	bool OnLogoutEnabled() const;
	bool IsUserPro() const;
	bool UsesOrgProfileChecked() const;
	FText GetLoginText() const;
	FText GetLoginButtonText() const;

	// Plugin version
	FReply CheckLatestPluginVersion();
	FReply GetLatestPluginVersion();

	// PRO ?
	FReply OnUpgradeToPro();
	
	// User data
	void GetUserData();
	void GetUserOrgs();
	void GetOrgsProjects(FSketchfabOrg* org);


	FReply OnCancel();
	
	FString Token;
	FString GetSketchfabCacheDir();

	void DoLoginLogout(const FString &url);

	TSharedRef<SVerticalBox> CreateLoginArea(bool isBrowser);
	TSharedRef<SVerticalBox> CreateFooterArea(bool isBrowser);

	EVisibility ShouldDisplayUpgradeToPro() const;

	EVisibility GetNewVersionButtonVisibility() const;
	FText GetCurrentVersionText() const;
	void OnCheckLatestVersion(const FSketchfabTask& InTask);

	void OnUrlChanged(const FText &url);
	void OnUserData(const FSketchfabTask& InTask);
	void OnUserOrgs(const FSketchfabTask& InTask);
	void OnOrgsProjects(const FSketchfabTask& InTask);
	void OnTaskFailed(const FSketchfabTask& InTask);
	void OnOAuthWindowClosed(const TSharedRef<SWindow>& InWindow);

	virtual EVisibility ShouldDisplayClearCache() const;
	virtual EVisibility GetOrgDropdownVisibility() const;
	virtual EVisibility GetOrgCheckboxVisibility() const;

	void OnUseOrgProfileCheckStateChanged(ECheckBoxState NewState);
	
	virtual FReply OnClearCache() { return FReply::Handled(); };
	virtual void OnOrgChanged() {};

	static void OpenUrlInBrowser(FString Url);
	TSharedPtr<SPopUpWindow> CreatePopUp(FString title, FString subtitle, FString content, FString ok, FString cancel);

	TWeakPtr<SWindow> Window;

	FString CacheFolder;

	TSharedPtr<SWindow> OAuthWindowPtr;
	bool isOauthWindowClosed = false;

	FString CurrentPluginVersion;
	FString LatestPluginVersion;

	FString LoggedInUserDisplayName;
	FString LoggedInUserName;
	FString LoggedInUserAccountType;
	bool IsLoggedUserPro;

	bool IsMemberOfAnOrg;
	bool UsesOrgProfile;

	int WindowWidth;
	int WindowHeight;

	TArray<FSketchfabOrg> Orgs;

	void UpdateAvailableProjects();

	// Org dropdown
	TSharedRef<SWidget> GenerateOrgComboItem(TSharedPtr<FString> InItem);
	void HandleOrgComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetOrgComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> OrgsComboBox;
	TArray<TSharedPtr<FString>> OrgComboList;
	int32 OrgIndex;
	FString CurrentOrgString;


	// Virtual function to react on org
	virtual EVisibility GetProjectDropdownVisibility() const;
	TSharedRef<SWidget> GenerateProjectComboItem(TSharedPtr<FString> InItem);
	void HandleProjectComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetProjectComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ProjectsComboBox;
	TArray<TSharedPtr<FString>> ProjectComboList;
	int32 ProjectIndex;
	FString CurrentProjectString;

	virtual void GenerateProjectComboItems() {};

};
