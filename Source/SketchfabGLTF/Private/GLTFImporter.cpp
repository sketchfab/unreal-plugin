// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SKGLTFImporter.h"
#include "Misc/ScopedSlowTask.h"
#include "AssetSelection.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"
#include "SKGLTFConversionUtils.h"
#include "StaticMeshImporter.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "SKGLTFImporterProjectSettings.h"

#include "Misc/FileHelper.h"
#include "Materials/MaterialInterface.h"
#include "MaterialExpressionIO.h"
#include "Materials/Material.h"
#include "Factories/MaterialFactoryNew.h"
#include "Engine/Texture.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"

#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionVertexColor.h"

#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "ARFilter.h"
#include "Factories/MaterialImportHelpers.h"
#include "EditorFramework/AssetImportData.h"
#include "RawMesh.h"

#include "Modules/ModuleManager.h"
#include "SketchfabAssetBrowser/Public/ISketchfabAssetBrowserModule.h"

#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 25
// CreatePackage only uses one argument in 4.26+
#ifndef CreatePackage
#define CreatePackage(x) CreatePackage(nullptr, x)
#endif
#endif

DEFINE_LOG_CATEGORY(LogGLTFImport);


class SGLTFOptionsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGLTFOptionsWindow)
		: _ImportOptions(nullptr)
	{}

	SLATE_ARGUMENT(UObject*, ImportOptions)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		ImportOptions = InArgs._ImportOptions;
		Window = InArgs._WidgetWindow;
		bShouldImport = false;

		FString LicenseInfo = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").LicenseInfo;
		FText LicenseText = FText::FromString(LicenseInfo);

		TSharedPtr<SBox> DetailsViewBox;
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SAssignNew(DetailsViewBox, SBox)
				.MaxDesiredHeight(450.0f)
				.MinDesiredWidth(550.0f)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(LicenseText)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)

				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("GLTFOptionWindow_Import", "Import"))
					.OnClicked(this, &SGLTFOptionsWindow::OnImport)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("GLTFOptionWindow_Cancel", "Cancel"))
					.ToolTipText(LOCTEXT("GLTFOptionWindow_Cancel_ToolTip", "Cancels importing this GLTF file"))
					.OnClicked(this, &SGLTFOptionsWindow::OnCancel)
				]
			]
		];

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

		DetailsViewBox->SetContent(DetailsView.ToSharedRef());
		DetailsView->SetObject(ImportOptions);

	}

	virtual bool SupportsKeyboardFocus() const override { return true; }

	FReply OnImport()
	{
		bShouldImport = true;
		if (Window.IsValid())
		{
			Window.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}


	FReply OnCancel()
	{
		bShouldImport = false;
		if (Window.IsValid())
		{
			Window.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return bShouldImport;
	}

private:
	UObject* ImportOptions;
	TWeakPtr< SWindow > Window;
	bool bShouldImport;
};

USKGLTFImporter::USKGLTFImporter(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

UObject* USKGLTFImporter::ImportMeshes(FGLTFImportContext& ImportContext, const TArray<FGLTFPrimToImport>& PrimsToImport)
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ImportingGLTFMeshes", "Importing glTF Meshes"));
	SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;
	int32 MeshCount = 0;

	const FTransform& ConversionTransform = ImportContext.ConversionTransform;

	ESKGLTFMeshImportType MeshImportType = ImportContext.ImportOptions->MeshImportType;

	// Make unique names
	TMap<FString, int> ExistingNamesToCount;

	ImportContext.PathToImportAssetMap.Reserve(PrimsToImport.Num());

	bool singleMesh = ImportContext.ImportOptions->bMergeMeshes || (PrimsToImport.Num() == 1);
	UStaticMesh* singleStaticMesh = nullptr;

	FString FinalPackagePathName = ImportContext.GetImportPath();

	FRawMesh RawTriangles;
	bool UseRootName = true;
	for (const FGLTFPrimToImport& PrimToImport : PrimsToImport)
	{
		SlowTask.EnterProgressFrame(1.0f / PrimsToImport.Num(), FText::Format(LOCTEXT("ImportingGLTFMesh", "Importing Mesh {0} of {1}"), MeshCount + 1, PrimsToImport.Num()));

		FString NewPackageName;

		bool bShouldImport = false;

		// when importing only one mesh we just use the existing package and name created
		if (PrimsToImport.Num() > 1 || !singleMesh)
		{
			FString RawPrimName = GLTFToUnreal::ConvertString(PrimToImport.Prim->name);
			if (RawPrimName == "")
			{
				RawPrimName = "Mesh_" + FString::FromInt(MeshCount + 1);
			}

			FString MeshName = RawPrimName;
			if (UseRootName && singleMesh)
			{
				UseRootName = false;
				MeshName = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").CurrentModelName;
			}

			if (singleMesh)
			{
				// Make unique names
				int* ExistingCount = ExistingNamesToCount.Find(MeshName);
				if (ExistingCount)
				{
					MeshName += TEXT("_");
					MeshName.AppendInt(*ExistingCount);
					++(*ExistingCount);
				}
				else
				{
					ExistingNamesToCount.Add(MeshName, 1);
				}
			}

			MeshName = ObjectTools::SanitizeObjectName(MeshName);

			NewPackageName = UPackageTools::SanitizePackageName(FinalPackagePathName / MeshName);

			// Once we've already imported it we dont need to import it again
			if (!ImportContext.PathToImportAssetMap.Contains(NewPackageName))
			{
				UPackage* Package = CreatePackage(*NewPackageName);

				Package->FullyLoad();

				ImportContext.Parent = Package;
				ImportContext.ObjectName = MeshName;

				bShouldImport = true;
			}

		}
		else
		{
			bShouldImport = true;
		}

		if (singleMesh || PrimsToImport.Num() == 1)
		{
			FString MeshName = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").CurrentModelName;
			NewPackageName = UPackageTools::SanitizePackageName(FinalPackagePathName / MeshName);
			if (!ImportContext.PathToImportAssetMap.Contains(NewPackageName))
			{
				UPackage* Package = CreatePackage(*NewPackageName);
				Package->FullyLoad();
				ImportContext.Parent = Package;
			}	
			ImportContext.ObjectName = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").CurrentModelName;
		}

		if (bShouldImport)
		{
			if (!singleMesh)
			{
				RawTriangles.Empty();
			}

			UStaticMesh* NewMesh = nullptr;
			if (MeshImportType == ESKGLTFMeshImportType::StaticMesh)
			{
				NewMesh = FGLTFStaticMeshImporter::ImportStaticMesh(ImportContext, PrimToImport, RawTriangles, singleStaticMesh);
				if (singleMesh)
				{
					singleStaticMesh = NewMesh;
				}
				else
				{
					FGLTFStaticMeshImporter::commitRawMesh(NewMesh, RawTriangles);

					NewMesh->ImportVersion = EImportStaticMeshVersion::BeforeImportStaticMeshVersionWasAdded;
					NewMesh->CreateBodySetup();
					NewMesh->SetLightingGuid();
					NewMesh->PostEditChange();

					FAssetRegistryModule::AssetCreated(NewMesh);
					NewMesh->MarkPackageDirty();
					ImportContext.PathToImportAssetMap.Add(NewPackageName, NewMesh);
				}
				++MeshCount;
			}
		}
	}

	if (singleStaticMesh && singleMesh)
	{
		FGLTFStaticMeshImporter::commitRawMesh(singleStaticMesh, RawTriangles);
		FString MeshName = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").CurrentModelName;
		FString NewPackageName = UPackageTools::SanitizePackageName(FinalPackagePathName / MeshName);

		FAssetRegistryModule::AssetCreated(singleStaticMesh);
		singleStaticMesh->MarkPackageDirty();
		ImportContext.PathToImportAssetMap.Add(NewPackageName, singleStaticMesh);

		singleStaticMesh->ImportVersion = EImportStaticMeshVersion::BeforeImportStaticMeshVersionWasAdded;
		singleStaticMesh->CreateBodySetup();
		singleStaticMesh->SetLightingGuid();
		singleStaticMesh->PostEditChange();
	}

	// Return the first one on success.
	return ImportContext.PathToImportAssetMap.Num() ? ImportContext.PathToImportAssetMap.CreateIterator().Value() : nullptr;
}

bool USKGLTFImporter::ShowImportOptions(UObject& ImportOptions)
{
	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("GLTFImportSettings", "GLTF Import Options"))
		.SizingRule(ESizingRule::Autosized);


	TSharedPtr<SGLTFOptionsWindow> OptionsWindow;
	Window->SetContent
	(
		SAssignNew(OptionsWindow, SGLTFOptionsWindow)
		.ImportOptions(&ImportOptions)
		.WidgetWindow(Window)
	);

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	return OptionsWindow->ShouldImport();
}

tinygltf::Model* USKGLTFImporter::ReadGLTFFile(FGLTFImportContext& ImportContext, const FString& Filename)
{
	FString FilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Filename);
	FString CleanFilename = FPaths::GetCleanFilename(Filename);

	tinygltf::Model* Model = new tinygltf::Model();

	std::string err;
	std::string warn;
	std::string filename = TCHAR_TO_ANSI(*FilePath);
	tinygltf::TinyGLTF gltf;

	const FString Extension = FPaths::GetExtension(CleanFilename);

	bool success = false;
	if (Extension == TEXT("gltf"))
	{
		success = gltf.LoadASCIIFromFile(Model, &err, &warn, filename);
	}
	else if (Extension == TEXT("glb"))
	{
		success = gltf.LoadBinaryFromFile(Model, &err, &warn, filename);
	}

	if (!success)
	{
		delete Model;
		Model = nullptr;

		const char* Errors = err.c_str();
		if (Errors)
		{
			FString ErrorStr = GLTFToUnreal::ConvertString(Errors);
			ImportContext.AddErrorMessage(EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotImportGLTFFile", "Could not import GLTF file {0}\n {1}"), FText::FromString(CleanFilename), FText::FromString(ErrorStr)));
		}
	}

	return Model;
}

UTexture* USKGLTFImporter::ImportTexture(FGLTFImportContext& ImportContext, tinygltf::Image *img, EMaterialSamplerType samplerType, const char *MaterialProperty)
{
	if (!img)
	{
		return nullptr;
	}

	const FString &FileBasePath = ImportContext.BasePath;

	// create an unreal texture asset
	UTexture* UnrealTexture = NULL;

	FString AbsoluteFilename;
	if (img->uri.size() > 0)
	{
		AbsoluteFilename = GLTFToUnreal::ConvertString(img->uri);
	}
	else if (img->mimeType.size() > 0)
	{
		FString mimetype = GLTFToUnreal::ConvertString(img->mimeType);
		AbsoluteFilename = GLTFToUnreal::ConvertString(MaterialProperty);
		AbsoluteFilename = ImportContext.ObjectName + TEXT("_") + AbsoluteFilename + TEXT(".") + mimetype.Right(3);
	}

	FString Extension = FPaths::GetExtension(AbsoluteFilename).ToLower();
	// name the texture with file name
	FString TextureName = FPaths::GetBaseFilename(AbsoluteFilename);

	TextureName = ObjectTools::SanitizeObjectName(TextureName);

	// set where to place the textures
	FString BasePackageName = ImportContext.GetImportPath() / TextureName;
	BasePackageName = UPackageTools::SanitizePackageName(BasePackageName);

	UTexture* ExistingTexture = NULL;
	UPackage* TexturePackage = NULL;

	FString ObjectPath = BasePackageName + TEXT(".") + TextureName;
	ExistingTexture = LoadObject<UTexture>(NULL, *ObjectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);

	if (!ExistingTexture)
	{
		const FString Suffix(TEXT(""));

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString FinalPackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, TextureName);

		TexturePackage = CreatePackage(*FinalPackageName);
	}
	else
	{
		TexturePackage = ExistingTexture->GetOutermost();
	}

	FString FinalFilePath;
	if (IFileManager::Get().FileExists(*AbsoluteFilename))
	{
		FinalFilePath = AbsoluteFilename;
	}
	else if (IFileManager::Get().FileExists(*(FileBasePath / GLTFToUnreal::ConvertString(img->uri))))
	{
		FinalFilePath = FileBasePath / GLTFToUnreal::ConvertString(img->uri);
	}
	else if (IFileManager::Get().FileExists(*(FileBasePath / AbsoluteFilename)))
	{
		FinalFilePath = FileBasePath / AbsoluteFilename;
	}

	TArray<uint8> DataBinary;
	if (!FinalFilePath.IsEmpty())
	{
		FFileHelper::LoadFileToArray(DataBinary, *FinalFilePath);
	}
	else if (img->image.size() > 0)
	{
		if (img->bufferView >= 0 && img->bufferView < ImportContext.Model->bufferViews.size())
		{
			const tinygltf::BufferView &bufferView = ImportContext.Model->bufferViews[img->bufferView];
			if (bufferView.buffer >= 0 && bufferView.buffer < ImportContext.Model->buffers.size())
			{
				const tinygltf::Buffer &buffer = ImportContext.Model->buffers[bufferView.buffer];
				DataBinary.AddUninitialized(bufferView.byteLength);
				for (int32 a = 0; a < bufferView.byteLength; a++)
				{
					DataBinary[a] = buffer.data[bufferView.byteOffset + a];
				}
			}
		}
	}

	if (DataBinary.Num() > 0)
	{
		const uint8* PtrTexture = DataBinary.GetData();
		const auto &TextureFact = NewObject<UTextureFactory>();
		TextureFact->AddToRoot();

		TextureFact->SuppressImportOverwriteDialog();
		const TCHAR* TextureType = *Extension;

		if (samplerType == SAMPLERTYPE_Normal)
		{
			if (!ExistingTexture)
			{
				TextureFact->LODGroup = TEXTUREGROUP_WorldNormalMap;
				TextureFact->CompressionSettings = TC_Normalmap;
				TextureFact->bFlipNormalMapGreenChannel = false;
			}
		}

		UnrealTexture = (UTexture*)TextureFact->FactoryCreateBinary(
			UTexture2D::StaticClass(), TexturePackage, *TextureName,
			RF_Standalone | RF_Public, NULL, TextureType,
			PtrTexture, PtrTexture + DataBinary.Num(), GWarn);

		if (UnrealTexture != NULL)
		{
			if (samplerType == SAMPLERTYPE_LinearColor)
			{
				TextureFact->CompressionSettings = TC_VectorDisplacementmap;
				UnrealTexture->CompressionSettings = TC_VectorDisplacementmap;
				UnrealTexture->SRGB = false;
			}

			//Make sure the AssetImportData point on the texture file and not on the gltf files since the factory point on the gltf file
			UnrealTexture->AssetImportData->Update(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FinalFilePath));

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(UnrealTexture);

			// Set the dirty flag so this package will get saved later
			UnrealTexture->UpdateResource();

		}
		TextureFact->RemoveFromRoot();
	}

	return UnrealTexture;
}

void CreateAddExpression(UMaterial* UnrealMaterial, FExpressionInput& MaterialInput, UMaterialExpression* Expression1, UMaterialExpression* Expression2, ColorChannel colorChannel)
{
	if (!UnrealMaterial || !Expression1 || !Expression2)
		return;

	UMaterialExpressionAdd* AddExpression = NewObject<UMaterialExpressionAdd>(UnrealMaterial);

	if (!AddExpression)
		return;

	UnrealMaterial->Expressions.Add(AddExpression);
	AddExpression->A.Expression = Expression1;
	AddExpression->B.Expression = Expression2;
	MaterialInput.Expression = AddExpression;

	if (colorChannel != ColorChannel_All)
	{
		TArray<FExpressionOutput> Outputs = AddExpression->B.Expression->GetOutputs();
		FExpressionOutput* Output = nullptr;
		if ((Outputs.Num() == 5) || (Outputs.Num() == 6))
		{
			switch (colorChannel)
			{
			case ColorChannel_Red:
				Output = &Outputs[1];
				break;
			case ColorChannel_Green:
				Output = &Outputs[2];
				break;
			case ColorChannel_Blue:
				Output = &Outputs[3];
				break;
			case ColorChannel_Alpha:
				Output = &Outputs[4];
				break;
			case ColorChannel_All:
			default:
				Output = &Outputs[0];
				break;
			}
		}
		else
		{
			Output = Outputs.GetData();
		}

		AddExpression->B.Mask = Output->Mask;
		AddExpression->B.MaskR = Output->MaskR;
		AddExpression->B.MaskG = Output->MaskG;
		AddExpression->B.MaskB = Output->MaskB;
		AddExpression->B.MaskA = Output->MaskA;
	}
}

void USKGLTFImporter::CreateUnrealMaterial(FGLTFImportContext& ImportContext, tinygltf::Material *Mat, TArray<UMaterialInterface*>& OutMaterials)
{
	// Make sure we have a parent
	if (!ImportContext.Parent)
	{
		return;
	}

	FString MaterialFullName = ObjectTools::SanitizeObjectName(GLTFToUnreal::ConvertString(Mat->name));
	FString BasePackageName = UPackageTools::SanitizePackageName(ImportContext.GetImportPath() / MaterialFullName);

	//This ensures that if the object name is the same as the material name, then the package for the material will be different.
	BasePackageName = BasePackageName + TEXT(".") + MaterialFullName;

	const FString Suffix(TEXT(""));
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString FinalPackageName;
	FString OutAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, OutAssetName);

	UPackage* Package = CreatePackage(*FinalPackageName);

	// create an unreal material asset
	auto MaterialFactory = NewObject<UMaterialFactoryNew>();

	UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, *MaterialFullName, RF_Standalone | RF_Public, NULL, GWarn);
	if (UnrealMaterial != NULL)
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(UnrealMaterial);

		UnrealMaterial->TwoSided = Mat->doubleSided;

		if (Mat->alphaMode == "BLEND")
		{
			UnrealMaterial->BlendMode = BLEND_Translucent;
			UnrealMaterial->TranslucencyLightingMode = TLM_Surface;
		}
		else if (Mat->alphaMode == "MASK")
		{
			UnrealMaterial->BlendMode = BLEND_Masked;
			UnrealMaterial->OpacityMaskClipValue = Mat->alphaCutoff;
		}

		// Set the dirty flag so this package will get saved later
		Package->SetDirtyFlag(true);
		
		FScopedSlowTask MaterialProgress(10.0, LOCTEXT("ImportingGLTFMaterial", "Creating glTF Material Nodes"));

		FVector2D location(-300, -260);

		SharedTextureMap texMap;

		bool usingPBR = true;

		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "baseColorTexture", TextureType_PBR,  UnrealMaterial->BaseColor, SAMPLERTYPE_Color, location);
		if (CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "diffuseTexture", TextureType_SPEC, UnrealMaterial->BaseColor, SAMPLERTYPE_Color, location))
			usingPBR = false;


		if (usingPBR)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Roughness, SAMPLERTYPE_LinearColor, location, ColorChannel_Green);
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Metallic, SAMPLERTYPE_LinearColor, location, ColorChannel_Blue);
		}
		else
		{
			// KHR_materials_pbrSpecularGlossiness
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "specularGlossinessTexture", TextureType_SPEC, UnrealMaterial->Specular, SAMPLERTYPE_LinearColor, location, ColorChannel_All);
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "specularGlossinessTexture", TextureType_SPEC, UnrealMaterial->Roughness, SAMPLERTYPE_LinearColor, location, ColorChannel_Alpha);
		}

		// KHR_materials_clearcoat
		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "clearcoatTexture", TextureType_PBR, UnrealMaterial->ClearCoat, SAMPLERTYPE_LinearColor, location, ColorChannel_All);
		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "clearcoatRoughnessTexture", TextureType_PBR, UnrealMaterial->ClearCoatRoughness, SAMPLERTYPE_LinearColor, location, ColorChannel_All);

		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "emissiveTexture", TextureType_DEFAULT, UnrealMaterial->EmissiveColor, SAMPLERTYPE_Color, location);
		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "normalTexture", TextureType_DEFAULT, UnrealMaterial->Normal, SAMPLERTYPE_Normal, location);
		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "occlusionTexture", TextureType_DEFAULT, UnrealMaterial->AmbientOcclusion, SAMPLERTYPE_LinearColor, location, ColorChannel_Red);


		const char *opactityMaterial = "baseColorTexture";
		TextureType opactityType = TextureType_PBR;
		if (!usingPBR)
		{
			opactityMaterial = "diffuseTexture";
			opactityType = TextureType_SPEC;
		}

		if (UnrealMaterial->BlendMode == BLEND_Translucent)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, opactityMaterial, opactityType, UnrealMaterial->Opacity, SAMPLERTYPE_Color, location, ColorChannel_Alpha);
		}
		else if (UnrealMaterial->BlendMode == BLEND_Masked)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, opactityMaterial, opactityType, UnrealMaterial->OpacityMask, SAMPLERTYPE_Color, location, ColorChannel_Alpha);
		}

		// For the Unlit extension, add the emissive and basecolor together
		if (Mat->extensions.find("KHR_materials_unlit") != Mat->extensions.end())
		{
			UnrealMaterial->SetShadingModel(EMaterialShadingModel::MSM_Unlit);
			UMaterialExpression* BaseColorExpression = UnrealMaterial->BaseColor.Expression;
			UMaterialExpression* EmissiveColorExpression = UnrealMaterial->EmissiveColor.Expression;
			if (BaseColorExpression)
			{
				if (EmissiveColorExpression)
				{
					// Add the base color and the emissive color together
					CreateAddExpression(UnrealMaterial, UnrealMaterial->EmissiveColor, BaseColorExpression, EmissiveColorExpression, ColorChannel_All);
				}
				else
				{
					// Link the base color to the material's emissive color input
					UnrealMaterial->EmissiveColor.Expression = BaseColorExpression;
				}
			}
		}

		if (UnrealMaterial)
		{
			// let the material update itself if necessary
			UnrealMaterial->PreEditChange(NULL);
			UnrealMaterial->PostEditChange();

			OutMaterials.Add(UnrealMaterial);
		}
	}
}

void USKGLTFImporter::CreateMultiplyExpression(UMaterial* UnrealMaterial, FExpressionInput& MaterialInput, UMaterialExpression *ExpressionFactor, UMaterialExpression *UnrealTextureExpression, ColorChannel colorChannel)
{
	if (!UnrealMaterial || !ExpressionFactor || !UnrealTextureExpression)
		return;

	UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
	if (!MultiplyExpression)
		return;

	UnrealMaterial->Expressions.Add(MultiplyExpression);
	MultiplyExpression->A.Expression = ExpressionFactor;
	MultiplyExpression->B.Expression = UnrealTextureExpression;
	MaterialInput.Expression = MultiplyExpression;

	if (colorChannel != ColorChannel_All)
	{
		TArray<FExpressionOutput> Outputs = MultiplyExpression->B.Expression->GetOutputs();
		FExpressionOutput* Output = nullptr;
		if ( (Outputs.Num() == 5) || (Outputs.Num() == 6) )
		{
			switch (colorChannel)
			{
			case ColorChannel_Red:
				Output = &Outputs[1];
				break;
			case ColorChannel_Green:
				Output = &Outputs[2];
				break;
			case ColorChannel_Blue:
				Output = &Outputs[3];
				break;
			case ColorChannel_Alpha:
				Output = &Outputs[4];
				break;
			case ColorChannel_All:
			default:
				Output = &Outputs[0];
				break;
			}
		}
		else
		{
			Output = Outputs.GetData();
		}

		MultiplyExpression->B.Mask = Output->Mask;
		MultiplyExpression->B.MaskR = Output->MaskR;
		MultiplyExpression->B.MaskG = Output->MaskG;
		MultiplyExpression->B.MaskB = Output->MaskB;
		MultiplyExpression->B.MaskA = Output->MaskA;
	}
}

UMaterialExpressionOneMinus* CreateOneMinusExpression(UMaterial* UnrealMaterial, UMaterialExpression *UnrealTextureExpression, ColorChannel colorChannel)
{
	if (!UnrealMaterial || !UnrealTextureExpression)
		return nullptr;

	UMaterialExpressionOneMinus* OneMinusExpression = NewObject<UMaterialExpressionOneMinus>(UnrealMaterial);
	if (!OneMinusExpression)
		return nullptr;

	UnrealMaterial->Expressions.Add(OneMinusExpression);
	OneMinusExpression->Input.Expression = UnrealTextureExpression;

	TArray<FExpressionOutput> Outputs = UnrealTextureExpression->GetOutputs();
	FExpressionOutput* Output = nullptr;
	if ((Outputs.Num() == 5) || (Outputs.Num() == 6))
	{
		switch (colorChannel)
		{
		case ColorChannel_Red:
			Output = &Outputs[1];
			break;
		case ColorChannel_Green:
			Output = &Outputs[2];
			break;
		case ColorChannel_Blue:
			Output = &Outputs[3];
			break;
		case ColorChannel_Alpha:
			Output = &Outputs[4];
			break;
		case ColorChannel_All:
		default:
			Output = &Outputs[0];
			break;
		}
	}
	else
	{
		Output = Outputs.GetData();
	}

	OneMinusExpression->Input.Mask = Output->Mask;
	OneMinusExpression->Input.MaskR = Output->MaskR;
	OneMinusExpression->Input.MaskG = Output->MaskG;
	OneMinusExpression->Input.MaskB = Output->MaskB;
	OneMinusExpression->Input.MaskA = Output->MaskA;

	return OneMinusExpression;
}

void USKGLTFImporter::UseVertexColors(UMaterial* UnrealMaterial)
{
	UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(UnrealMaterial);
	UMaterialExpression* BaseColorExpression = UnrealMaterial->BaseColor.Expression;
	if (BaseColorExpression)
	{
		// Add the base color and the emissive color together
		CreateMultiplyExpression(UnrealMaterial, UnrealMaterial->BaseColor, BaseColorExpression, VertexColorExpression, ColorChannel_All);
	}
	else
	{
		UnrealMaterial->BaseColor.Expression = VertexColorExpression;
	}
}

void USKGLTFImporter::AttachOutputs(FExpressionInput& MaterialInput, ColorChannel colorChannel)
{
	if (MaterialInput.Expression)
	{
		TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
		FExpressionOutput* Output = nullptr;

		if ((Outputs.Num() == 5) || (Outputs.Num() == 6))
		{
			switch (colorChannel)
			{
			case ColorChannel_Red:
				Output = &Outputs[1];
				break;
			case ColorChannel_Green:
				Output = &Outputs[2];
				break;
			case ColorChannel_Blue:
				Output = &Outputs[3];
				break;
			case ColorChannel_Alpha:
				Output = &Outputs[4];
				break;
			case ColorChannel_All:
			default:
				Output = &Outputs[0];
				break;
			}
		}
		else
		{
			Output = Outputs.GetData();
		}

		MaterialInput.Mask = Output->Mask;
		MaterialInput.MaskR = Output->MaskR;
		MaterialInput.MaskG = Output->MaskG;
		MaterialInput.MaskB = Output->MaskB;
		MaterialInput.MaskA = Output->MaskA;

	}
}

tinygltf::Value* parseExtension(tinygltf::Material* mat, std::string name) {
	std::map<std::string, tinygltf::Value> map;
	if (mat->extensions.find(name) != mat->extensions.end()) {
		tinygltf::Value* ext = &((mat->extensions.find(name))->second);
		if (ext->IsObject()) {
			return ext;
		}
	}
	return nullptr;
}

bool USKGLTFImporter::CreateAndLinkExpressionForMaterialProperty(
	FScopedSlowTask &MaterialProgress,
	FGLTFImportContext& ImportContext,
	tinygltf::Material *mat,
	UMaterial* UnrealMaterial,
	SharedTextureMap &texMap,
	const char* MaterialProperty,
	TextureType texType,
	FExpressionInput& MaterialInput,
	EMaterialSamplerType samplerType,
	FVector2D& Location,
	ColorChannel colorChannel)
{
	MaterialProgress.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("ImportingGLTFMaterial", "Creating Material Node {0}"), FText::FromString(GLTFToUnreal::ConvertString(MaterialProperty))));
	bool bCreated = false;

	tinygltf::Value* specGlossExtension = parseExtension(mat, "KHR_materials_pbrSpecularGlossiness");
	tinygltf::Value* clearcoatExtension = parseExtension(mat, "KHR_materials_clearcoat");
	tinygltf::Value* transmissionExtension = parseExtension(mat, "KHR_materials_transmission");

	if (clearcoatExtension)
		UnrealMaterial->SetShadingModel(EMaterialShadingModel::MSM_ClearCoat);

	enum PBRTYPE
	{
		PBRTYPE_Undefined,
		PBRTYPE_Color,
		PBRTYPE_Metallic,
		PBRTYPE_Roughness,
		PBRTYPE_Emissive,
		PBRTYPE_Specular,
		PBRTYPE_Glossiness,
		PBRTYPE_Diffuse,
		PBRTYPE_Opacity,
		PBRTYPE_Clearcoat,
		PBRTYPE_ClearcoatRoughness
	};

	PBRTYPE pbrType = PBRTYPE_Undefined;

	UMaterialExpressionVectorParameter *baseColorFactor = nullptr;
	UMaterialExpressionScalarParameter* baseColorOpacityFactor = nullptr;
	UMaterialExpressionScalarParameter *metallicFactor = nullptr;
	UMaterialExpressionScalarParameter *roughnessFactor = nullptr;
	UMaterialExpressionVectorParameter *emissiveFactor = nullptr;
	UMaterialExpressionVectorParameter *specularFactor = nullptr;
	UMaterialExpressionVectorParameter *diffuseFactor = nullptr;
	UMaterialExpressionScalarParameter* clearcoatFactor = nullptr;
	UMaterialExpressionScalarParameter* clearcoatRoughnessFactor = nullptr;
	UMaterialExpressionScalarParameter *glossinessFactor = nullptr;

	UMaterialExpressionTextureSample* UnrealTextureExpression = nullptr;

	tinygltf::Value texture;
	tinygltf::TextureInfo textureInfo;
	tinygltf::NormalTextureInfo normalTextureInfo;
	tinygltf::OcclusionTextureInfo occlusionTextureInfo;

	if (strcmp(MaterialProperty, "diffuseTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_Diffuse;
		if (specGlossExtension) {
			texture = specGlossExtension->Get("diffuseTexture");
			tinygltf::Value _diffuseFactor = specGlossExtension->Get("diffuseFactor");

			if (_diffuseFactor.Type() && _diffuseFactor.ArrayLen() == 4) {
				diffuseFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (diffuseFactor)
				{
					if (diffuseFactor->CanRenameNode())
					{
						diffuseFactor->SetEditableName(GLTFToUnreal::ConvertString("diffuseFactor"));
					}
					UnrealMaterial->Expressions.Add(diffuseFactor);
					diffuseFactor->DefaultValue.R = _diffuseFactor.Get(0).GetNumberAsDouble();
					diffuseFactor->DefaultValue.G = _diffuseFactor.Get(1).GetNumberAsDouble();
					diffuseFactor->DefaultValue.B = _diffuseFactor.Get(2).GetNumberAsDouble();
					diffuseFactor->DefaultValue.A = _diffuseFactor.Get(3).GetNumberAsDouble();
					if (!texture.Type())
					{
						MaterialInput.Expression = diffuseFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "diffuseTexture") == 0 && colorChannel == ColorChannel_Alpha) //Using alpha from diffuse texture, or diffuseFactor, for Opacity.
	{
		pbrType = PBRTYPE_Opacity;
		if (specGlossExtension) {
			texture = specGlossExtension->Get("diffuseTexture");
			tinygltf::Value _diffuseFactor = specGlossExtension->Get("diffuseFactor");

			if (_diffuseFactor.Type() && _diffuseFactor.ArrayLen() == 4) {
				baseColorOpacityFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (baseColorOpacityFactor)
				{
					if (baseColorOpacityFactor->CanRenameNode())
					{
						baseColorOpacityFactor->SetEditableName(GLTFToUnreal::ConvertString("diffuseOpacityFactor"));
					}
					UnrealMaterial->Expressions.Add(baseColorOpacityFactor);
					baseColorOpacityFactor->DefaultValue = _diffuseFactor.Get(3).GetNumberAsDouble();
					if (!texture.Type())
					{
						MaterialInput.Expression = baseColorOpacityFactor;
						AttachOutputs(MaterialInput, ColorChannel_Alpha);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "specularGlossinessTexture") == 0 && colorChannel == ColorChannel_All) //RGB is used for specular channel
	{
		pbrType = PBRTYPE_Specular;
		if (specGlossExtension) {
			texture = specGlossExtension->Get("specularGlossinessTexture");
			tinygltf::Value _specularFactor = specGlossExtension->Get("specularFactor");

			if (_specularFactor.Type() && _specularFactor.ArrayLen() == 3)
			{
				specularFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (specularFactor)
				{
					if (specularFactor->CanRenameNode())
					{
						specularFactor->SetEditableName(GLTFToUnreal::ConvertString("specularFactor"));
					}

					UnrealMaterial->Expressions.Add(specularFactor);
					specularFactor->DefaultValue.R = _specularFactor.Get(0).GetNumberAsDouble();
					specularFactor->DefaultValue.G = _specularFactor.Get(1).GetNumberAsDouble();
					specularFactor->DefaultValue.B = _specularFactor.Get(2).GetNumberAsDouble();
					specularFactor->DefaultValue.A = 1.0;
					if (!texture.Type())
					{
						MaterialInput.Expression = specularFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
					int a = 3;
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "specularGlossinessTexture") == 0 && colorChannel == ColorChannel_Alpha) //Alpha is used for roughness channel with an additional negation node (1 - value).
	{
		pbrType = PBRTYPE_Glossiness;

		if (specGlossExtension) {
			texture = specGlossExtension->Get("specularGlossinessTexture");
			tinygltf::Value _glossinessFactor = specGlossExtension->Get("glossinessFactor");

			if (_glossinessFactor.Type())
			{
				glossinessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (glossinessFactor)
				{
					if (glossinessFactor->CanRenameNode())
					{
						glossinessFactor->SetEditableName(GLTFToUnreal::ConvertString("glossinessFactor"));
					}

					UnrealMaterial->Expressions.Add(glossinessFactor);
					glossinessFactor->DefaultValue = _glossinessFactor.GetNumberAsDouble();

					//If there is no specularTexture then we just use this color by itself and hook it up directly to the material
					if (!texture.Type())
					{
						MaterialInput.Expression = glossinessFactor;
						AttachOutputs(MaterialInput, ColorChannel_Alpha);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "baseColorTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_Color;

		textureInfo = mat->pbrMetallicRoughness.baseColorTexture;
		std::vector<double> _baseColorFactor = mat->pbrMetallicRoughness.baseColorFactor;

		if (_baseColorFactor.size() == 4)
		{
			baseColorFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
			if (baseColorFactor)
			{
				if (baseColorFactor->CanRenameNode())
				{
					baseColorFactor->SetEditableName(GLTFToUnreal::ConvertString("baseColorFactor"));
				}

				UnrealMaterial->Expressions.Add(baseColorFactor);
				baseColorFactor->DefaultValue.R = _baseColorFactor[0];
				baseColorFactor->DefaultValue.G = _baseColorFactor[1];
				baseColorFactor->DefaultValue.B = _baseColorFactor[2];
				baseColorFactor->DefaultValue.A = _baseColorFactor[3];
				if (textureInfo.index == -1)
				{
					MaterialInput.Expression = baseColorFactor;
					AttachOutputs(MaterialInput, ColorChannel_All);
					return true;
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "baseColorTexture") == 0 && colorChannel == ColorChannel_Alpha)
	{
		pbrType = PBRTYPE_Opacity;

		textureInfo = mat->pbrMetallicRoughness.baseColorTexture;
		std::vector<double> _baseColorFactor = mat->pbrMetallicRoughness.baseColorFactor;

		if (_baseColorFactor.size() == 4)
		{
			baseColorOpacityFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
			if (baseColorOpacityFactor)
			{
				if (baseColorOpacityFactor->CanRenameNode())
				{
					baseColorOpacityFactor->SetEditableName(GLTFToUnreal::ConvertString("baseColorOpacityFactor"));
				}

				UnrealMaterial->Expressions.Add(baseColorOpacityFactor);
				baseColorOpacityFactor->DefaultValue = _baseColorFactor[3];
				if (textureInfo.index == -1)
				{
					MaterialInput.Expression = baseColorOpacityFactor;
					AttachOutputs(MaterialInput, ColorChannel_Alpha);
					return true;
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "metallicRoughnessTexture") == 0 && colorChannel == ColorChannel_Blue) //Metallic comes from the blue channel of metallicRoughnessTexture
	{
		pbrType = PBRTYPE_Metallic;

		textureInfo = mat->pbrMetallicRoughness.metallicRoughnessTexture;
		double _metallicFactor = mat->pbrMetallicRoughness.metallicFactor;

		metallicFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
		if (metallicFactor)
		{
			if (metallicFactor->CanRenameNode())
			{
				metallicFactor->SetEditableName(GLTFToUnreal::ConvertString("metallicFactor"));
			}

			UnrealMaterial->Expressions.Add(metallicFactor);
			metallicFactor->DefaultValue = _metallicFactor;
			if (textureInfo.index == -1)
			{
				MaterialInput.Expression = metallicFactor;
				AttachOutputs(MaterialInput, ColorChannel_All);
				return true;
			}
		}
	}
	else if (strcmp(MaterialProperty, "metallicRoughnessTexture") == 0 && colorChannel == ColorChannel_Green) //Roughness comes from the green channel of metallicRoughnessTexture
	{
		pbrType = PBRTYPE_Roughness;

		textureInfo = mat->pbrMetallicRoughness.metallicRoughnessTexture;
		double _roughnessFactor = mat->pbrMetallicRoughness.roughnessFactor;

		roughnessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
		if (roughnessFactor)
		{
			if (roughnessFactor->CanRenameNode())
			{
				roughnessFactor->SetEditableName(GLTFToUnreal::ConvertString("roughnessFactor"));
			}

			UnrealMaterial->Expressions.Add(roughnessFactor);
			roughnessFactor->DefaultValue = _roughnessFactor;
			if (textureInfo.index == -1)
			{
				MaterialInput.Expression = roughnessFactor;
				AttachOutputs(MaterialInput, ColorChannel_Green);
				return true;
			}
		}
	}
	else if (strcmp(MaterialProperty, "emissiveTexture") == 0)
	{
		pbrType = PBRTYPE_Emissive;

		textureInfo = mat->emissiveTexture;
		std::vector<double> _emissiveFactor = mat->emissiveFactor;

		if (_emissiveFactor.size() == 3)
		{
			emissiveFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
			if (emissiveFactor)
			{
				if (emissiveFactor->CanRenameNode())
				{
					emissiveFactor->SetEditableName(GLTFToUnreal::ConvertString("emissiveFactor"));
				}

				UnrealMaterial->Expressions.Add(emissiveFactor);
				emissiveFactor->DefaultValue.R = _emissiveFactor[0];
				emissiveFactor->DefaultValue.G = _emissiveFactor[1];
				emissiveFactor->DefaultValue.B = _emissiveFactor[2];
				emissiveFactor->DefaultValue.A = 1.0;
				if (textureInfo.index == -1)
				{
					MaterialInput.Expression = emissiveFactor;
					AttachOutputs(MaterialInput, ColorChannel_All);
					return true;
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "normalTexture") == 0) 
	{
		normalTextureInfo = mat->normalTexture;
	}
	else if (strcmp(MaterialProperty, "occlusionTexture") == 0) 
	{
		occlusionTextureInfo = mat->occlusionTexture;
	}
	else if (strcmp(MaterialProperty, "clearcoatTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_Clearcoat;
		if (clearcoatExtension) {
			texture = clearcoatExtension->Get("clearcoatTexture");
			tinygltf::Value _clearcoatFactor = clearcoatExtension->Get("clearcoatFactor");

			if (_clearcoatFactor.Type() && _clearcoatFactor.GetNumberAsDouble()!=0) {
				clearcoatFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (clearcoatFactor)
				{
					if (clearcoatFactor->CanRenameNode())
					{
						clearcoatFactor->SetEditableName(GLTFToUnreal::ConvertString("clearcoatFactor"));
					}
					UnrealMaterial->Expressions.Add(clearcoatFactor);
					clearcoatFactor->DefaultValue = _clearcoatFactor.GetNumberAsDouble();
					if (!texture.Type())
					{
						MaterialInput.Expression = clearcoatFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "clearcoatRoughnessTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_ClearcoatRoughness;
		if (clearcoatExtension) {
			texture = clearcoatExtension->Get("clearcoatRoughnessTexture");
			tinygltf::Value _clearcoatRoughnessFactor = clearcoatExtension->Get("clearcoatRoughnessFactor");

			if (_clearcoatRoughnessFactor.Type() && _clearcoatRoughnessFactor.IsNumber()) {
				clearcoatRoughnessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (clearcoatRoughnessFactor)
				{
					if (clearcoatRoughnessFactor->CanRenameNode())
					{
						clearcoatRoughnessFactor->SetEditableName(GLTFToUnreal::ConvertString("clearcoatRoughnessFactor"));
					}
					UnrealMaterial->Expressions.Add(clearcoatRoughnessFactor);
					clearcoatRoughnessFactor->DefaultValue = _clearcoatRoughnessFactor.GetNumberAsDouble();
					if (!texture.Type())
					{
						MaterialInput.Expression = clearcoatRoughnessFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}


	// Find the image and add it to the material
	if (texture.Type() || textureInfo.index!=-1 || normalTextureInfo.index!=-1 || occlusionTextureInfo.index != -1)
	{
		float scaleValue=1.0, strengthValue=1.0;
		int32 texCoordValue, textureIndex;
		if (textureInfo.index != -1) {
			scaleValue = 1.0;
			strengthValue = 1.0;
			texCoordValue = textureInfo.texCoord;
			textureIndex =  textureInfo.index;
		}
		else if (normalTextureInfo.index != -1)
		{
			scaleValue = normalTextureInfo.scale;
			texCoordValue = normalTextureInfo.texCoord;
			textureIndex = normalTextureInfo.index;
		}
		else if (occlusionTextureInfo.index != -1)
		{
			strengthValue = occlusionTextureInfo.strength;
			texCoordValue = occlusionTextureInfo.texCoord;
			textureIndex = occlusionTextureInfo.index;
		}
		else {
			scaleValue    = texture.Get("scale").Type()    ? texture.Get("scale").GetNumberAsDouble()    : 1.0;
			strengthValue = texture.Get("strength").Type() ? texture.Get("strength").GetNumberAsDouble() : 1.0;
			texCoordValue = texture.Get("texCoord").Type() ? texture.Get("texCoord").GetNumberAsInt()    : 0;
			textureIndex  = texture.Get("index").Type()    ? texture.Get("index").GetNumberAsInt()       : 0;
		}

		if (textureIndex != -1)
		{
			if (textureIndex >= 0 && textureIndex < ImportContext.Model->textures.size())
			{
				int32 source = ImportContext.Model->textures[textureIndex].source;

				if (source >= 0 && source < ImportContext.Model->images.size())
				{
					tinygltf::Image& img = ImportContext.Model->images[source];

					//See if we have already loaded in this image
					SharedTexture* sharedMap = texMap.Find(source);
					if (sharedMap)
					{
						if (sharedMap->texCoords == texCoordValue)
						{
							UnrealTextureExpression = sharedMap->expression;
							MaterialInput.Expression = UnrealTextureExpression;
							bCreated = true;
						}
					}

					if (!bCreated)
					{
						UTexture* UnrealTexture = ImportTexture(ImportContext, &img, samplerType, MaterialProperty);
						if (UnrealTexture)
						{
							float ScaleU = 1.0;
							float ScaleV = 1.0;

							// and link it to the material
							UnrealTextureExpression = NewObject<UMaterialExpressionTextureSample>(UnrealMaterial);
							if (UnrealTextureExpression)
							{
								UnrealTextureExpression->Desc = FPaths::GetBaseFilename(GLTFToUnreal::ConvertString(img.uri));
								UnrealTextureExpression->bCommentBubbleVisible = true;
								UnrealMaterial->Expressions.Add(UnrealTextureExpression);
								MaterialInput.Expression = UnrealTextureExpression;
								UnrealTextureExpression->Texture = UnrealTexture;
								UnrealTextureExpression->SamplerType = samplerType;
								UnrealTextureExpression->MaterialExpressionEditorX = FMath::TruncToInt(Location.X);
								UnrealTextureExpression->MaterialExpressionEditorY = FMath::TruncToInt(Location.Y);

								if ((texCoordValue != 0 && texCoordValue != INDEX_NONE) || ScaleU != 1.0f || ScaleV != 1.0f)
								{
									// Create a texture coord node for the texture sample
									UMaterialExpressionTextureCoordinate* MyCoordExpression = NewObject<UMaterialExpressionTextureCoordinate>(UnrealMaterial);
									if (MyCoordExpression)
									{
										UnrealMaterial->Expressions.Add(MyCoordExpression);
										MyCoordExpression->CoordinateIndex = (texCoordValue >= 0) ? texCoordValue : 0;
										MyCoordExpression->UTiling = ScaleU;
										MyCoordExpression->VTiling = ScaleV;
										UnrealTextureExpression->Coordinates.Expression = MyCoordExpression;
										MyCoordExpression->MaterialExpressionEditorX = FMath::TruncToInt(Location.X - 175);
										MyCoordExpression->MaterialExpressionEditorY = FMath::TruncToInt(Location.Y);
									}
								}

								Location.Y += 240;

								SharedTexture sharedTex;
								sharedTex.texCoords = texCoordValue;
								sharedTex.expression = UnrealTextureExpression;
								texMap.Add(source, sharedTex);

								bCreated = true;
							}
						}
					}
				}
			}

			// Special case for normals (since there is no normalFactor like other channels)
			// normals have a scale.
			if (strcmp(MaterialProperty, "normalTexture") == 0)
			{
				UMaterialExpressionScalarParameter* scaleFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (scaleFactor)
				{
					if (scaleFactor->CanRenameNode())
					{
						scaleFactor->SetEditableName(GLTFToUnreal::ConvertString("normalTextureScale"));
					}

					UnrealMaterial->Expressions.Add(scaleFactor);
					scaleFactor->DefaultValue = scaleValue;
					CreateMultiplyExpression(UnrealMaterial, MaterialInput, scaleFactor, UnrealTextureExpression, ColorChannel_All);
				}
			}

			// Special case for occlusion (since there is no occlusionFactor like other channels)
			// occlusion has a strength.
			else if (strcmp(MaterialProperty, "occlusionTexture") == 0)
			{
				UMaterialExpressionScalarParameter* strengthFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (strengthFactor)
				{
					if (strengthFactor->CanRenameNode())
					{
						strengthFactor->SetEditableName(GLTFToUnreal::ConvertString("occlusionTextureStrength"));
					}

					UnrealMaterial->Expressions.Add(strengthFactor);
					strengthFactor->DefaultValue = strengthValue;

					CreateMultiplyExpression(UnrealMaterial, MaterialInput, strengthFactor, UnrealTextureExpression, ColorChannel_Red);

					colorChannel = ColorChannel_All;
				}
			}

			switch (pbrType)
			{
				case PBRTYPE_Color:		CreateMultiplyExpression(UnrealMaterial, MaterialInput, baseColorFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Roughness:	CreateMultiplyExpression(UnrealMaterial, MaterialInput, roughnessFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Metallic:	CreateMultiplyExpression(UnrealMaterial, MaterialInput, metallicFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Emissive:	CreateMultiplyExpression(UnrealMaterial, MaterialInput, emissiveFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Diffuse:	CreateMultiplyExpression(UnrealMaterial, MaterialInput, diffuseFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Specular:	CreateMultiplyExpression(UnrealMaterial, MaterialInput, specularFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Clearcoat: CreateMultiplyExpression(UnrealMaterial, MaterialInput, clearcoatFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_ClearcoatRoughness: CreateMultiplyExpression(UnrealMaterial, MaterialInput, clearcoatRoughnessFactor, UnrealTextureExpression, colorChannel); break;
				case PBRTYPE_Glossiness:
				{
					//Add the OneMinus node to invert the glossiness channel for non prb materials
					UMaterialExpressionOneMinus* OneMinus = CreateOneMinusExpression(UnrealMaterial, UnrealTextureExpression, colorChannel);
					if (glossinessFactor)
					{
						CreateMultiplyExpression(UnrealMaterial, MaterialInput, glossinessFactor, OneMinus, colorChannel);
					}
					else
					{
						MaterialInput.Expression = OneMinus;
					}
					break;
				}
				case PBRTYPE_Opacity: CreateMultiplyExpression(UnrealMaterial, MaterialInput, baseColorOpacityFactor, UnrealTextureExpression, ColorChannel_Alpha); break;
				default: break;
			}

			// Parse KHR_texture_transform
			tinygltf::ExtensionMap* textureExtensions = nullptr;
			if (textureInfo.index != -1)
				textureExtensions = &textureInfo.extensions;
			else if(normalTextureInfo.index != -1)
				textureExtensions = &normalTextureInfo.extensions;
			else if (occlusionTextureInfo.index != -1)
				textureExtensions = &occlusionTextureInfo.extensions;
			if (textureExtensions && textureExtensions->find("KHR_texture_transform") != textureExtensions->end()) {
				tinygltf::Value ext = (textureExtensions->find("KHR_texture_transform"))->second;
				if (ext.Has("scale")) {
					tinygltf::Value scale = ext.Get("scale");
					if (scale.IsArray() && scale.ArrayLen() == 2) {
						float scaleU = scale.Get(0).GetNumberAsDouble();
						float scaleV = scale.Get(1).GetNumberAsDouble();
						UMaterialExpressionTextureCoordinate* UVScaleExpression = NewObject<UMaterialExpressionTextureCoordinate>(UnrealMaterial);
						UnrealMaterial->Expressions.Add(UVScaleExpression);
						UVScaleExpression->UTiling = scaleU;
						UVScaleExpression->VTiling = scaleV;
						UVScaleExpression->ConnectExpression(UnrealTextureExpression->GetInput(0), 0);
					}
				}				
			}
		}
	}

	return bCreated;
}

int32 USKGLTFImporter::CreateNodeMaterials(FGLTFImportContext &ImportContext, TArray<UMaterialInterface*>& OutMaterials)
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ImportingGLTFMaterials", "Importing glTF Materials"));
	SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;

	int32 MaterialCount = ImportContext.Model->materials.size();

	float step = 1.0f / MaterialCount;

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		SlowTask.EnterProgressFrame(step, FText::Format(LOCTEXT("ImportingGLTFMaterial", "Importing Material {0} of {1}"), MaterialIndex + 1, MaterialCount));
		CreateUnrealMaterial(ImportContext, &ImportContext.Model->materials[MaterialIndex], OutMaterials);
	}
	return MaterialCount;
}


//======================================================================================================

void FGLTFImportContext::Init(UObject* InParent, const FString& InName, const FString &InBasePath, tinygltf::Model* InModel)
{
	Parent = InParent;
	ObjectName = InName;
	BasePath = InBasePath;
	ImportPathName = InParent->GetOutermost()->GetName();

	// Path should not include the filename
	ImportPathName.RemoveFromEnd(FString(TEXT("/")) + InName);

	ImportObjectFlags = RF_Public | RF_Standalone | RF_Transactional;

	TSubclassOf<USKGLTFPrimResolver> ResolverClass = GetDefault<USKGLTFImporterProjectSettings>()->CustomPrimResolver;
	if (!ResolverClass)
	{
		ResolverClass = USKGLTFPrimResolver::StaticClass();
	}

	PrimResolver = NewObject<USKGLTFPrimResolver>(GetTransientPackage(), ResolverClass);
	PrimResolver->Init();

	// A matrix that converts Y up right handed coordinate system to Z up left handed (unreal)
	ConversionTransform =
		FTransform(FMatrix
		(
			FPlane(1, 0, 0, 0),
			FPlane(0, 0, 1, 0),
			FPlane(0, -1, 0, 0),
			FPlane(0, 0, 0, 1)
		));

	Model = InModel;
	bApplyWorldTransform = true;

	// In some cases, we need to stop parsing tangents and normals
	disableTangents = false;
	disableNormals = false;
}

void FGLTFImportContext::AddErrorMessage(EMessageSeverity::Type MessageSeverity, FText ErrorMessage)
{
	TokenizedErrorMessages.Add(FTokenizedMessage::Create(MessageSeverity, ErrorMessage));
}

void FGLTFImportContext::DisplayErrorMessages(bool bAutomated)
{
	if (!bAutomated)
	{
		//Always clear the old message after an import or re-import
		const TCHAR* LogTitle = TEXT("GLTFImport");
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		TSharedPtr<class IMessageLogListing> LogListing = MessageLogModule.GetLogListing(LogTitle);
		LogListing->SetLabel(FText::FromString("GLTF Import"));
		LogListing->ClearMessages();

		if (TokenizedErrorMessages.Num() > 0)
		{
			LogListing->AddMessages(TokenizedErrorMessages);
			MessageLogModule.OpenMessageLog(LogTitle);
		}
	}
	else
	{
		for (const TSharedRef<FTokenizedMessage>& Message : TokenizedErrorMessages)
		{
			UE_LOG(LogGLTFImport, Error, TEXT("%s"), *Message->ToText().ToString());
		}
	}
}

void FGLTFImportContext::ClearErrorMessages()
{
	TokenizedErrorMessages.Empty();
}

FString FGLTFImportContext::GetImportPath(FString DirectoryName)
{
	FString PackagePathName = ImportPathName;
	if (bImportInNewFolder)
	{
		FString FolderName = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser").CurrentModelName;
		FolderName = FolderName.IsEmpty() ? "ImportedAsset" : FolderName;
		PackagePathName += "/" + FolderName;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(PackagePathName);
	}
	return PackagePathName;
}

#undef LOCTEXT_NAMESPACE

