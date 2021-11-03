// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SSketchfabExporterWindow.h"

#include "SKGLTFExportOptions.h"
#include "Exporters/SKGLTFExporter.h"
#include "Exporters/SKGLTFLevelExporter.h"
#include "Exporters/Exporter.h"

#include "AssetExportTask.h"
#include "EditorLevelLibrary.h"
#include "Editor/EditorEngine.h"

//#include "ZipFileFunctionLibrary.h"

#include "GameFramework/Actor.h"
#include "UObject/Object.h"
#include "UObject/GCObjectScopeGuard.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/SComboBox.h"

#include "SKGLTFZipUtility.h"


#define LOCTEXT_NAMESPACE "SketchfabExporter"
DEFINE_LOG_CATEGORY(LogSketchfabExporterWindow);


// Window creation routines
void SSketchfabExporterWindow::Construct(const FArguments& InArgs)
{
	initWindow();
	InitComboBoxes();
	Window = InArgs._WidgetWindow;

	TSharedRef<SVerticalBox> LoginNode  = CreateLoginArea(0);
	TSharedRef<SVerticalBox> UploadNode = CreateUploadArea();
	TSharedRef<SVerticalBox> FooterNode = CreateFooterArea(0);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			LoginNode
		]

		+ SVerticalBox::Slot()
		.Padding(20.0f, 10.0f, 20.0f, 20.0f)
		[
			UploadNode
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			FooterNode
		]
	];

	GenerateProjectComboItems();
}
TSharedRef<SVerticalBox> SSketchfabExporterWindow::CreateUploadArea() {

	// Upload area
	TSharedRef<SVerticalBox> UploadNode = SNew(SVerticalBox)
		.IsEnabled(this, &SSketchfabWindow::OnLogoutEnabled);
	// Add some padding on the top (will allow to center the upload fields)
	UploadNode->AddSlot();

	// Text boxes (title, description, tags)
	TSharedRef<SEditableTextBox> ModelTitleEditableBox = SNew(SEditableTextBox)
		.Justification(ETextJustify::Left)
		.OnTextCommitted(this, &SSketchfabExporterWindow::OnTitleChanged)
		.HintText(LOCTEXT("STextBlockCenterExamplsdsddgdfgdfgeLabsdfel", "Enter the name of your model... (max 48 characters)"));
	TSharedRef<SMultiLineEditableTextBox> ModelDescriptionEditableBox = SNew(SMultiLineEditableTextBox)
		.Justification(ETextJustify::Left)
		.OnTextCommitted(this, &SSketchfabExporterWindow::OnDescriptionChanged)
		.HintText(LOCTEXT("STextBlockCenterExamplsdsddgdfgdfgdfgdfgeLabsdfel", "Enter a description for your model...\nMarkdown syntax is supported\n(max 1024 characters)\n "));
	TSharedRef<SEditableTextBox> ModelTagsEditableBox = SNew(SEditableTextBox)
		.Justification(ETextJustify::Left)
		.OnTextCommitted(this, &SSketchfabExporterWindow::OnTagsChanged)
		.HintText(LOCTEXT("STextBlockCenterExamplsdsddgdfgdfgeLabsdfcvbcvbel", "Enter whitespace separated tags for your model... (max 42 tags)"));

	// Check boxes (selected, bake, draft, private)
	TSharedRef<SCheckBox> SelectionCheckBox = SNew(SCheckBox)
		.IsChecked(this, &SSketchfabExporterWindow::IsSelectedChecked)
		.OnCheckStateChanged(this, &SSketchfabExporterWindow::OnSelectedCheckStateChanged)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SSketchfabExporterWindow_Selected", "Export selected objects only"))
		];
	TSharedRef<SCheckBox> BakingCheckBox = SNew(SCheckBox)
		.IsChecked(this, &SSketchfabExporterWindow::IsBakeChecked)
		.OnCheckStateChanged(this, &SSketchfabExporterWindow::OnBakeCheckStateChanged)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SSketchfabExporterWindow_Bake", "Bake material nodes to textures"))
		];
	TSharedRef<SCheckBox> DraftCheckBox = SNew(SCheckBox)
		.IsChecked(this, &SSketchfabExporterWindow::IsDraftChecked)
		.OnCheckStateChanged(this, &SSketchfabExporterWindow::OnDraftCheckStateChanged)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SSketchfabExporterWindow_Draft", "Draft (don't publish immediately)"))
		];
	TSharedRef<SCheckBox> PrivateCheckBox = SNew(SCheckBox)
		.IsChecked(this, &SSketchfabExporterWindow::IsPrivateChecked)
		.OnCheckStateChanged(this, &SSketchfabExporterWindow::OnPrivateCheckStateChanged)
		.IsEnabled(this, &SSketchfabWindow::IsUserPro)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SSketchfabExporterWindow_Private", "Private (Pro+ users only)"))
		];

	// Password (for private only)
	TSharedRef<SEditableTextBox> PasswordEditableBox = SNew(SEditableTextBox)
		.Justification(ETextJustify::Left)
		.OnTextCommitted(this, &SSketchfabExporterWindow::OnPasswordChanged)
		.IsEnabled(this, &SSketchfabExporterWindow::IsPasswordEnabled)
		.HintText(FText::FromString("Enter a password for your model (optional)"));

	// Project selector for org uploads
	TSharedRef<STextComboBox> ProjectComboBox = SNew(STextComboBox)
		.OptionsSource(&ProjectComboList)
		.IsEnabled(this, &SSketchfabWindow::UsesOrgProfileChecked)
		.OnSelectionChanged(this, &SSketchfabExporterWindow::HandleProjectComboChanged);
		//.OnGenerateWidget(this, &SSketchfabWindow::GenerateProjectComboItem)
		//.Visibility(this, );
		/*[
			SNew(STextBlock)
			.Text(this, &SSketchfabWindow::GetProjectComboText)
		];*/
		//.IsEnabled(this, &SSketchfabExporterWindow::IsBakeCheckedBool)

	// Upload button
	TSharedRef<SButton> UploadButton = SNew(SButton)
		.OnClicked(this, &SSketchfabExporterWindow::OnUploadButtonPressed)
		.ContentPadding(20.0f)
		.IsEnabled(this, &SSketchfabWindow::OnLogoutEnabled)
		[
			SNew(STextBlock)
			.Text(this, &SSketchfabExporterWindow::GetUploadButtonText) //FText::FromString("Login to upload"))
			.Justification(ETextJustify::Center)
			.MinDesiredWidth(250.0f)
		];

	// Baking resolution
	TSharedRef<STextComboBox> BakingResolutionComboBox = SNew(STextComboBox)
		.OptionsSource(&ResolutionComboList)
		.IsEnabled(this, &SSketchfabExporterWindow::IsBakeCheckedBool)
		.OnSelectionChanged(this, &SSketchfabExporterWindow::HandleBakingResolutionComboChanged);

	// Add some tooltips to the check boxes
	SelectionCheckBox->SetToolTipText(FText::FromString("Checked:   Only upload the objects currently selected in the world\nUnchecked: Upload all visible objects in the world"));
	BakingCheckBox->SetToolTipText(FText::FromString("Recommended if custom material nodes are used"));
	DraftCheckBox->SetToolTipText(FText::FromString("Checked:   Your model will be uploaded as a draft, and will need to be published manually\nUnchecked: Your model will be published directly"));
	PrivateCheckBox->SetToolTipText(FText::FromString("For PRO+ users, make this model private"));

	// Adding widgets to the layout
	CreateHorizontalField(UploadNode, "Model Title", ModelTitleEditableBox);
	CreateHorizontalField(UploadNode, "Model Description", ModelDescriptionEditableBox, 0.f, false);
	CreateHorizontalField(UploadNode, "Model Tags", ModelTagsEditableBox, 25.0f);
	CreateHorizontalField(UploadNode, "", BakingCheckBox);
	CreateHorizontalField(UploadNode, "Baking resolution", BakingResolutionComboBox, 25.0f);
	CreateHorizontalField(UploadNode, "", SelectionCheckBox);
	CreateHorizontalField(UploadNode, "", DraftCheckBox);
	CreateHorizontalField(UploadNode, "", PrivateCheckBox);
	CreateHorizontalField(UploadNode, "", PasswordEditableBox, 25.0f);

	CreateHorizontalField(UploadNode, "Upload to Project", ProjectComboBox);
	CreateHorizontalField(UploadNode, "", UploadButton);

	// Keep track of texts and button (to alter parameters)
	pTitleEditableBox       = ModelTitleEditableBox;
	pDescriptionEditableBox = ModelDescriptionEditableBox;
	pTagsEditableBox        = ModelTagsEditableBox;
	pPasswordEditableBox    = PasswordEditableBox;
	pUploadButton           = UploadButton;
	pProjectComboBox        = ProjectComboBox;

	// Add some padding on the bottom
	UploadNode->AddSlot();

	return UploadNode;
}
void SSketchfabExporterWindow::CreateHorizontalField(TSharedRef<SVerticalBox> VBox, FString LText, TSharedRef<SCompoundWidget> Widget, float BPadding, bool AutoHeight) {

	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Right)
		.Padding(20.0f, 0.0f, 20.0f, 0.0f)
		[
			SNew(STextBlock)
			.MinDesiredWidth(100.0f)
			.Justification(ETextJustify::Right)
			.Text(FText::FromString(LText))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(3)
		.HAlign(HAlign_Fill)
		.Padding(2)
		[
			Widget
		];

		if (AutoHeight) VBox->AddSlot().FillHeight(1.0f).AutoHeight().Padding(0, 0, 0, BPadding)[HBox];
		else            VBox->AddSlot().FillHeight(5.0f).Padding(0, 0, 0, BPadding)[HBox];
}

void SSketchfabExporterWindow::GenerateProjectComboItems() {

	if(ProjectComboList.Num() > 0)
		ProjectComboList.Empty();
	
	if (Orgs.Num() > 0) {
		for (auto& project : Orgs[OrgIndex].Projects)
		{
			ProjectComboList.Add(MakeShared<FString>(project.name));
		}
		if (Orgs[OrgIndex].Projects.Num() > 0)
		{
			ProjectIndex = 0;
			CurrentProjectString = Orgs[OrgIndex].Projects[0].name;
		}
		pProjectComboBox->RefreshOptions();
	}
}

void SSketchfabExporterWindow::InitComboBoxes(){
	BakingResolutionIndex = (int32)RES_1024;
	for (int32 i = 0; i < (int32)RES_UNDEFINED; i++)
	{
		switch (i)
		{
			case RES_128:  {ResolutionComboList.Add(MakeShared<FString>(TEXT("128x128")));}   break;
			case RES_256:  {ResolutionComboList.Add(MakeShared<FString>(TEXT("256x256")));}   break;
			case RES_512:  {ResolutionComboList.Add(MakeShared<FString>(TEXT("512x512")));}   break;
			case RES_1024: {ResolutionComboList.Add(MakeShared<FString>(TEXT("1024x1024")));} break;
			case RES_2048: {ResolutionComboList.Add(MakeShared<FString>(TEXT("2048x2048")));} break;
			case RES_4096: {ResolutionComboList.Add(MakeShared<FString>(TEXT("4096x4096")));} break;
		}
	}
}
void SSketchfabExporterWindow::HandleBakingResolutionComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < ResolutionComboList.Num(); i++)
	{
		if (Item == ResolutionComboList[i])
		{
			BakingResolutionIndex = i;
		}
	}
}
void SSketchfabExporterWindow::HandleProjectComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo) {
	for (int32 i = 0; i < ProjectComboList.Num(); i++)
	{
		if (Item == ProjectComboList[i])
		{
			ProjectIndex = i;
		}
	}
}

// UI callbacks
ECheckBoxState SSketchfabExporterWindow::IsSelectedChecked() const
{
	return bSelectedOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
ECheckBoxState SSketchfabExporterWindow::IsBakeChecked() const
{
	return bBakeMaterials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
bool SSketchfabExporterWindow::IsBakeCheckedBool() const
{
	return bBakeMaterials;
}
ECheckBoxState SSketchfabExporterWindow::IsDraftChecked() const
{
	return bDraft ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
ECheckBoxState SSketchfabExporterWindow::IsPrivateChecked() const
{
	return bPrivate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
void SSketchfabExporterWindow::OnSelectedCheckStateChanged(ECheckBoxState NewState)
{
	bSelectedOnly = (NewState == ECheckBoxState::Checked);
}
void SSketchfabExporterWindow::OnBakeCheckStateChanged(ECheckBoxState NewState)
{
	bBakeMaterials = (NewState == ECheckBoxState::Checked);
}
void SSketchfabExporterWindow::OnDraftCheckStateChanged(ECheckBoxState NewState)
{
	bDraft = (NewState == ECheckBoxState::Checked);
}
void SSketchfabExporterWindow::OnPrivateCheckStateChanged(ECheckBoxState NewState)
{
	bPrivate = (NewState == ECheckBoxState::Checked);
	if (!bPrivate)
	{
		pPasswordEditableBox->SetText(FText::FromString(""));
		modelPassword = "";
	}
}
void SSketchfabExporterWindow::OnTitleChanged(const FText& InText, ETextCommit::Type Type) {
	FString clipped = InText.ToString().Left(48);
	pTitleEditableBox->SetText(FText::FromString(clipped));
	modelTitle = clipped;
}
void SSketchfabExporterWindow::OnDescriptionChanged(const FText& InText, ETextCommit::Type Type) {
	FString clipped = InText.ToString().Left(1024);
	pDescriptionEditableBox->SetText(FText::FromString(clipped));
	modelDescription = clipped;
}
void SSketchfabExporterWindow::OnTagsChanged(const FText& InText, ETextCommit::Type Type) {
	FString clipped = InText.ToString().Left(2048);
	bool endsWithWS = clipped.EndsWith(" ");

	TArray<FString> splitArray;
	clipped.ParseIntoArrayWS(splitArray);
	int32 nElements = FMath::Min(splitArray.Num(), 42);
	splitArray.SetNum(nElements);
	for (int i = 0; i < nElements; i++) {
		splitArray[i] = splitArray[i].Left(48);
	}
	clipped = FString::Join(splitArray, TEXT(" "));

	if(endsWithWS){
		clipped += " ";
	}

	pTagsEditableBox->SetText(FText::FromString(clipped));
	modelTags = clipped;
}
void SSketchfabExporterWindow::OnPasswordChanged(const FText& InText, ETextCommit::Type Type) {
	FString clipped = InText.ToString().Replace(TEXT(" "), TEXT("")).Left(48);
	pPasswordEditableBox->SetText(FText::FromString(clipped));
	modelPassword = clipped;
}
FText SSketchfabExporterWindow::GetUploadButtonText() const {
	return OnLogoutEnabled() ? FText::FromString("Upload") : FText::FromString("Login to upload");
}

// Upload functions
void zipSuccessPlaceHolder() { UE_LOG(LogSketchfabExporterWindow, Log, TEXT("Zip creation successful")); return; }
FReply SSketchfabExporterWindow::OnUploadButtonPressed()
{
	// Init the file paths
	GlbPath = CacheFolder + "sketchfabExport.glb";
	ZipPath = CacheFolder + "sketchfabExport.glb.zip";

	// Get the Editor world to run the level exporter on
	UWorld* CurrentWorld = 	GEditor->GetEditorWorldContext().World();

	// Prepare the GLTFExporter object and the file to export to
	UExporter* Exporter = UExporter::FindExporter(CurrentWorld, TEXT("gltf"));

	if (Exporter)
	{
		// Get the GlTF Exporter object from the identified exporter
		USKGLTFLevelExporter* gltfExp = Cast<USKGLTFLevelExporter>(Exporter);
		gltfExp->CurrentFilename = GlbPath;
		gltfExp->SetShowExportOption(false);

		// Set the exporter options (linked to Sketchfab Export)
		USKGLTFExportOptions* opts = gltfExp->GetExportOptions();
		opts->ResetToDefault();
		// Hidden models
		opts->bExportHiddenInGame = false; // Hidden meshes not exported
		opts->BakeMaterialInputs = bBakeMaterials ? ESKGLTFMaterialBakeMode::UseMeshData : ESKGLTFMaterialBakeMode::Disabled; // Don't bake the material inputs
		// Behaviour and Epic web additional options
		opts->DefaultMaterialBakeSize = static_cast<ESKGLTFMaterialBakeSizePOT>(BakingResolutionIndex + 7); // Align on ESKGLTFMaterialBakeSizePOT::POT_128
		opts->bExportPreviewMesh = false;
		opts->bBundleWebViewer = false;
		opts->bShowFilesWhenDone = false;
		// Materials and Mesh data
		opts->bExportUnlitMaterials = true;
		opts->bExportClearCoatMaterials = true;
		opts->bExportExtraBlendModes = false; // Managed by a custom EPIC extension
		opts->bExportVertexColors = true;
		opts->bExportVertexSkinWeights = true;
		// Variants and quantizations
		opts->bExportVariantSets = false;
		opts->bExportMeshVariants = false;
		opts->bExportVisibilityVariants = false;
		opts->bExportMeshQuantization = false; // We don't support Draco compression
		// Various exports (lights, cameras, skyspheres...)
		opts->TextureHDREncoding = ESKGLTFTextureHDREncoding::RGBM;
		opts->bExportLightmaps = false;
		opts->ExportLights = 0;
		opts->bExportCameras = false;
		opts->bExportCameraControls = false;
		opts->bExportAnimationHotspots = false;
		opts->bExportHDRIBackdrops = false;
		opts->bExportSkySpheres = false;

		// Create the Export Task and set its parameters
		gltfExp->ExportTask = NewObject<UAssetExportTask>();
		UAssetExportTask* task = gltfExp->ExportTask;
		// Link the basic parameters
		task->Options = opts;
		task->Object = CurrentWorld;
		task->Exporter = gltfExp;
		task->Filename = GlbPath;
		// Link the runtime parameters
		task->bSelected = bSelectedOnly;
		task->bReplaceIdentical = true;
		task->bPrompt = false;
		task->bUseFileArchive = false;
		task->bWriteEmptyFiles = false;
		task->bAutomated = true;

		// Run the actual export
		bool GltfSuccess = USKGLTFExporter::RunAssetExportTask(task);
		if (GltfSuccess) {
			UE_LOG(LogSketchfabExporterWindow, Log, TEXT("Export successful to %s"), *GlbPath);
		}
		else {
			UE_LOG(LogSketchfabExporterWindow, Error, TEXT("Failed to export %s"), *GlbPath);
		}
	}

	if (FGLTFZipUtility::CompressDirectory(ZipPath, GlbPath)) {
		UE_LOG(LogSketchfabExporterWindow, Error, TEXT("Failure encountered while zipping the file"));
		TSharedPtr<SPopUpWindow> popup = CreatePopUp
		(
			"Archive error",
			"Your model did not compress correctly",
			"The plugin encountered an error during the compression of your scene.\n"
			"Please check that UE has writing permissions in \n"
			+ FPaths::GetPath(ZipPath),
			"Open directory",
			"Back to Unreal Engine"
		);
		if (popup->Confirmed())
		{
			OpenUrlInBrowser(FPaths::GetPath(ZipPath).Replace(TEXT("/"), TEXT("\\")));
		}
		CleanUploadArtifacts();
		return FReply::Handled();
	}

	// Check that the archive size is ok for the user's plan
	bool ZipSizeOK = ValidateArchiveSize(ZipPath, LoggedInUserAccountType);
	if (!ZipSizeOK) {
		int64 FileSize = FPlatformFileManager::Get().GetPlatformFile().FileSize(*ZipPath);
		FString FileSizeAsString = FString::SanitizeFloat(FileSize / 1.e6);
		FString LimitAsString = FString::SanitizeFloat(PlanUploadLimit(LoggedInUserAccountType) / 1.e6);
		UE_LOG(LogSketchfabExporterWindow, Error, TEXT("Zip file too big (%s MB) for your plan %s"), *FileSizeAsString, *LoggedInUserAccountType);
		TSharedPtr<SPopUpWindow> popup = CreatePopUp
		(
			"Archive size",
			"The created archive is too big for your plan",
			"The zip archive of your scene is " + FileSizeAsString + "MB.\n"
				"Current plan: " + LoggedInUserAccountType + "\n"
				"Upload limit: " + LimitAsString + " MB\n"
				"You can try to simplify your scene (smaller textures, reduced polycount), or upgrade your Sketchfab account.\n\n"
				"Until this pop-up is closed, the exported model is available for inspection (https://gltf-viewer.donmccurdy.com/):\n" + GlbPath,
			"Upgrade plan",
			"Back to Unreal Engine"
		);
		if (popup->Confirmed())
		{
			OpenUrlInBrowser("https://sketchfab.com/plans");
		}
		CleanUploadArtifacts();
		return FReply::Handled();
	}

	// Make the API calls to upload the file
	Upload();

	// Return something
	return FReply::Handled();
}
void SSketchfabExporterWindow::Upload(){
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.FileToUpload = ZipPath;
	TaskData.StateLock = new FCriticalSection();

	// Custom options
	TaskData.ModelName        = modelTitle;
	TaskData.ModelDescription = modelDescription;
	TaskData.ModelTags        = modelTags;
	TaskData.ModelPassword    = modelPassword;
	TaskData.UsesOrgProfile   = UsesOrgProfile;
	TaskData.OrgUID           = UsesOrgProfile ? Orgs[OrgIndex].uid : "";
	TaskData.ProjectUID       = UsesOrgProfile ? Orgs[OrgIndex].Projects[ProjectIndex].uid : "";
	TaskData.Draft         = bDraft;
	TaskData.Private       = bPrivate;
	TaskData.BakeMaterials = bBakeMaterials;

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_UPLOADMODEL);
	Task->OnModelUploaded().BindRaw(this, &SSketchfabExporterWindow::OnUploadSuccess);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabExporterWindow::OnUploadFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);

	TSharedPtr<SPopUpWindow> popup = CreatePopUp
	(
		"Upload in progress",
		"Your model is uploading",
		"Your model is currently being uploaded to Sketchfab.\n"
			"Please do not close UE Editor until another pop-up informs you of the status of the upload.\n",
		"OK",
		""
	);
}
void SSketchfabExporterWindow::OnUploadFailed(const FSketchfabTask& InTask)
{
	UE_LOG(LogSketchfabExporterWindow, Error, TEXT("Upload failed"));
	TSharedPtr<SPopUpWindow> popup = CreatePopUp
	(
		"Upload failure",
		"The upload to Sketchfab has failed",
		"See the output log for more info (Window -> Developer Tools -> Output Log)\n"
			"If you wish to submit a report, please attach the content of the output log.\n",
		"Submit a report",
		"Back to Unreal Engine"
	);
	if (popup->Confirmed())
	{
		OpenUrlInBrowser("https://help.sketchfab.com/hc/en-us/requests/new");
	}
	CleanUploadArtifacts();
}
void SSketchfabExporterWindow::OnUploadSuccess(const FSketchfabTask& InTask)
{
	FString modelUrl = "https://sketchfab.com/models/" + InTask.TaskData.uid;
	TSharedPtr<SPopUpWindow> popup = CreatePopUp
	(
		"Upload successful",
		"Your model has finished uploading",
		"Your model is being processed on Sketchfab:\n" + modelUrl,
		"Open on Sketchfab",
		"Back to Unreal Engine"
	);
	if (popup->Confirmed()) 
	{
		OpenUrlInBrowser(modelUrl);
	}

	CleanUploadArtifacts();
}

// Upload helpers
void SSketchfabExporterWindow::CleanUploadArtifacts() {
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FileManager.DeleteFile(*GlbPath);
	FileManager.DeleteFile(*ZipPath);
}
float SSketchfabExporterWindow::PlanUploadLimit(FString UserPlan) {
	if (UserPlan == "(BASIC)")
		return 100.e6;
	else if (UserPlan == "(PRO)")
		return 200.e6;
	else if (UserPlan == "(PREMIUM)" || UserPlan == "(BIZ)" || UserPlan == "(ENT)")
		return 500.e6;
	UE_LOG(LogSketchfabExporterWindow, Error, TEXT("Account type %s not recognized"), *LoggedInUserAccountType);
	return 0.;
}
bool SSketchfabExporterWindow::ValidateArchiveSize(FString path, FString userPlan) {
	return FPlatformFileManager::Get().GetPlatformFile().FileSize(*path) <= PlanUploadLimit(userPlan);
}
bool SSketchfabExporterWindow::IsPasswordEnabled() const {
	return IsUserPro() && IsPrivateChecked() == ECheckBoxState::Checked;
}

#undef LOCTEXT_NAMESPACE

