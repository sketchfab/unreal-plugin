// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GLTFImporter.h"
#include "ScopedSlowTask.h"
#include "AssetSelection.h"
#include "SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"
#include "GLTFConversionUtils.h"
#include "StaticMeshImporter.h"
#include "Engine/StaticMesh.h"
#include "SBox.h"
#include "SButton.h"
#include "SlateApplication.h"
#include "FileManager.h"
#include "GLTFImporterProjectSettings.h"

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
#include "Materials/MaterialExpressionOneMinus.h"

#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "ARFilter.h"
#include "Factories/MaterialImportHelpers.h"
#include "EditorFramework/AssetImportData.h"
#include "RawMesh.h" 


#define LOCTEXT_NAMESPACE "GLTFImportPlugin"

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

UGLTFImporter::UGLTFImporter(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

UObject* UGLTFImporter::ImportMeshes(FGltfImportContext& ImportContext, const TArray<FGltfPrimToImport>& PrimsToImport)
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ImportingGLTFMeshes", "Importing glTF Meshes"));
	SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;
	int32 MeshCount = 0;

	const FTransform& ConversionTransform = ImportContext.ConversionTransform;

	EGltfMeshImportType MeshImportType = ImportContext.ImportOptions->MeshImportType;

	// Make unique names
	TMap<FString, int> ExistingNamesToCount;

	ImportContext.PathToImportAssetMap.Reserve(PrimsToImport.Num());

	const FString& ContentDirectoryLocation = ImportContext.ImportPathName;

	bool singleMesh = !ImportContext.ImportOptions->bGenerateUniquePathPerMesh;
	UStaticMesh* singleStaticMesh = nullptr;

	FRawMesh RawTriangles;
	for (const FGltfPrimToImport& PrimToImport : PrimsToImport)
	{
		FString FinalPackagePathName = ContentDirectoryLocation;
		SlowTask.EnterProgressFrame(1.0f / PrimsToImport.Num(), FText::Format(LOCTEXT("ImportingGLTFMesh", "Importing Mesh {0} of {1}"), MeshCount + 1, PrimsToImport.Num()));

		FString NewPackageName;

		bool bShouldImport = false;

		// when importing only one mesh we just use the existing package and name created
		if (PrimsToImport.Num() > 1 || ImportContext.ImportOptions->bGenerateUniquePathPerMesh)
		{
			FString RawPrimName = GLTFToUnreal::ConvertString(PrimToImport.Prim->name);
			if (RawPrimName == "")
			{
				RawPrimName = "Mesh_" + FString::FromInt(MeshCount + 1);
			}

			FString MeshName = RawPrimName;

			if (ImportContext.ImportOptions->bGenerateUniquePathPerMesh)
			{
				/*
				FString USDPath = GLTFToUnreal::ConvertString(PrimToImport.Prim->GetPrimPath());
				USDPath.RemoveFromStart(TEXT("/"));
				USDPath.RemoveFromEnd(RawPrimName);
				FinalPackagePathName /= USDPath;
				*/
			}
			else
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

			NewPackageName = PackageTools::SanitizePackageName(FinalPackagePathName / MeshName);

			// Once we've already imported it we dont need to import it again
			if (!ImportContext.PathToImportAssetMap.Contains(NewPackageName))
			{
				UPackage* Package = CreatePackage(nullptr, *NewPackageName);

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

		if (bShouldImport)
		{
			if (!singleMesh)
			{
				RawTriangles.Empty();
			}

			UStaticMesh* NewMesh = ImportSingleMesh(ImportContext, MeshImportType, PrimToImport, RawTriangles, singleStaticMesh);
			if (NewMesh)
			{
				if (singleMesh)
				{
					singleStaticMesh = NewMesh;
				}
				else
				{
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
		FString FinalPackagePathName = ContentDirectoryLocation;
		FString MeshName = ObjectTools::SanitizeObjectName(ImportContext.ObjectName);
		FString NewPackageName = PackageTools::SanitizePackageName(FinalPackagePathName / MeshName);

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

UStaticMesh* UGLTFImporter::ImportSingleMesh(FGltfImportContext& ImportContext, EGltfMeshImportType ImportType, const FGltfPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh)
{
	UStaticMesh* NewMesh = nullptr;

	if (ImportType == EGltfMeshImportType::StaticMesh)
	{
		NewMesh = FGLTFStaticMeshImporter::ImportStaticMesh(ImportContext, PrimToImport, RawTriangles, singleMesh);
	}

	return NewMesh;
}

bool UGLTFImporter::ShowImportOptions(UObject& ImportOptions)
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

tinygltf::Model* UGLTFImporter::ReadGLTFFile(FGltfImportContext& ImportContext, const FString& Filename)
{
	FString FilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Filename);
	FString CleanFilename = FPaths::GetCleanFilename(Filename);

	tinygltf::Model* Model = new tinygltf::Model();

	std::string err;
	std::string filename = TCHAR_TO_ANSI(*FilePath);
	tinygltf::TinyGLTF gltf;

	const FString Extension = FPaths::GetExtension(CleanFilename);

	bool success = false;
	if (Extension == TEXT("gltf"))
	{
		success = gltf.LoadASCIIFromFile(Model, &err, filename);
	}
	else if (Extension == TEXT("glb"))
	{
		success = gltf.LoadBinaryFromFile(Model, &err, filename);
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

UTexture* UGLTFImporter::ImportTexture(FGltfImportContext& ImportContext, tinygltf::Image *img, bool bSetupAsNormalMap, const char *MaterialProperty)
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
	FString BasePackageName = FPackageName::GetLongPackagePath(ImportContext.Parent->GetOutermost()->GetName()) / TextureName;
	BasePackageName = PackageTools::SanitizePackageName(BasePackageName);

	UTexture* ExistingTexture = NULL;
	UPackage* TexturePackage = NULL;
	// First check if the asset already exists.
	{
		FString ObjectPath = BasePackageName + TEXT(".") + TextureName;
		ExistingTexture = LoadObject<UTexture>(NULL, *ObjectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}


	if (!ExistingTexture)
	{
		const FString Suffix(TEXT(""));

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString FinalPackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, TextureName);

		TexturePackage = CreatePackage(NULL, *FinalPackageName);
	}
	else
	{
		TexturePackage = ExistingTexture->GetOutermost();
	}

	FString FinalFilePath;
	if (IFileManager::Get().FileExists(*AbsoluteFilename))
	{
		// try opening from absolute path
		FinalFilePath = AbsoluteFilename;
	}
	else if (IFileManager::Get().FileExists(*(FileBasePath / GLTFToUnreal::ConvertString(img->uri))))
	{
		// try fbx file base path + relative path
		FinalFilePath = FileBasePath / GLTFToUnreal::ConvertString(img->uri);
	}
	else if (IFileManager::Get().FileExists(*(FileBasePath / AbsoluteFilename)))
	{
		// Some fbx files dont store the actual absolute filename as absolute and it is actually relative.  Try to get it relative to the FBX file we are importing
		FinalFilePath = FileBasePath / AbsoluteFilename;
	}
	else
	{
		//UE_LOG(LogFbxMaterialImport, Warning, TEXT("Unable to find Texture file %s"), *AbsoluteFilename);
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
		//UE_LOG(LogFbxMaterialImport, Verbose, TEXT("Loading texture file %s"), *FinalFilePath);
		const uint8* PtrTexture = DataBinary.GetData();
		const auto &TextureFact = NewObject<UTextureFactory>();
		TextureFact->AddToRoot();

		// save texture settings if texture exist
		TextureFact->SuppressImportOverwriteDialog();
		const TCHAR* TextureType = *Extension;

		// Unless the normal map setting is used during import, 
		//	the user has to manually hit "reimport" then "recompress now" button
		if (bSetupAsNormalMap)
		{
			if (!ExistingTexture)
			{
				TextureFact->LODGroup = TEXTUREGROUP_WorldNormalMap;
				TextureFact->CompressionSettings = TC_Normalmap;
				TextureFact->bFlipNormalMapGreenChannel = false; // ImportContext.ImportOptions->bInvertNormalMap;
			}
			else
			{
				//UE_LOG(LogFbxMaterialImport, Warning, TEXT("Manual texture reimport and recompression may be needed for %s"), *TextureName);
			}
		}

		UnrealTexture = (UTexture*)TextureFact->FactoryCreateBinary(
			UTexture2D::StaticClass(), TexturePackage, *TextureName,
			RF_Standalone | RF_Public, NULL, TextureType,
			PtrTexture, PtrTexture + DataBinary.Num(), GWarn);

		if (UnrealTexture != NULL)
		{
			//Make sure the AssetImportData point on the texture file and not on the fbx files since the factory point on the fbx file
			UnrealTexture->AssetImportData->Update(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FinalFilePath));

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(UnrealTexture);

			// Set the dirty flag so this package will get saved later
			TexturePackage->SetDirtyFlag(true);
		}
		TextureFact->RemoveFromRoot();
	}

	return UnrealTexture;
}


void UGLTFImporter::ImportTexturesFromNode(FGltfImportContext& ImportContext, tinygltf::Model* Model, tinygltf::Node* Node)
{
	const FString &Path = ImportContext.BasePath;

	int32 NbMat = Model->materials.size(); //Should be the number of materials used by the node (ie its mesh and their primitives).

	// visit all materials
	int32 MaterialIndex;
	for (MaterialIndex = 0; MaterialIndex < NbMat; MaterialIndex++)
	{
		tinygltf::Material &Material = Model->materials[MaterialIndex];

		//go through all the possible textures
		{
			const auto &baseColorTexture = Material.values.find("baseColorTexture");
			if (baseColorTexture != Material.values.end())
			{
				tinygltf::Parameter &param = baseColorTexture->second;
				if (param.json_double_value.size() > 0)
				{
					const auto &textureIndexEntry = param.json_double_value.find("index");
					if (textureIndexEntry != param.json_double_value.end())
					{
						int32 textureIndex = textureIndexEntry->second;
						if (textureIndex >= 0 && textureIndex < Model->textures.size())
						{
							tinygltf::Texture &texture = Model->textures[textureIndex];
							int32 source = texture.source;

							if (source >= 0 && source < Model->images.size())
							{
								tinygltf::Image &img = Model->images[source];

								bool bSetupAsNormalMap = false;
								ImportTexture(ImportContext, &img, bSetupAsNormalMap);
							}
						}
					}
				}
			}
		}//end if(Material)
	}// end for MaterialIndex
}

void UGLTFImporter::CreateUnrealMaterial(FGltfImportContext& ImportContext, tinygltf::Material *Mat, TArray<UMaterialInterface*>& OutMaterials)
{
	// Make sure we have a parent
	//if (!ensure(ImportContext.Parent.IsValid()))
	if (!ImportContext.Parent)
	{
		return;
	}

	FString MaterialFullName = ObjectTools::SanitizeObjectName(GLTFToUnreal::ConvertString(Mat->name));
	FString BasePackageName = PackageTools::SanitizePackageName(FPackageName::GetLongPackagePath(ImportContext.Parent->GetOutermost()->GetName()) / MaterialFullName);

	//This ensures that if the object name is the same as the material name, then the package for the material will be different.
	BasePackageName = BasePackageName + TEXT(".") + MaterialFullName;

	const FString Suffix(TEXT(""));
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString FinalPackageName;
	FString OutAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, OutAssetName);

	UPackage* Package = CreatePackage(NULL, *FinalPackageName);

	// create an unreal material asset
	auto MaterialFactory = NewObject<UMaterialFactoryNew>();

	UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, *MaterialFullName, RF_Standalone | RF_Public, NULL, GWarn);
	if (UnrealMaterial != NULL)
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(UnrealMaterial);

		UnrealMaterial->TwoSided = false;
		const auto &doubleSidedProp = Mat->additionalValues.find("doubleSided");
		if (doubleSidedProp != Mat->additionalValues.end())
		{
			tinygltf::Parameter &param = doubleSidedProp->second;
			UnrealMaterial->TwoSided = param.bool_value;
		}

		const auto &alphaModeProp = Mat->additionalValues.find("alphaMode");
		if (alphaModeProp != Mat->additionalValues.end())
		{
			tinygltf::Parameter &param = alphaModeProp->second;
			if (param.string_value == "BLEND") 
			{
				UnrealMaterial->BlendMode = BLEND_Translucent;
				UnrealMaterial->TranslucencyLightingMode = TLM_Surface;
			}
			else if (param.string_value == "MASK")
			{
				UnrealMaterial->BlendMode = BLEND_Masked;

				const auto &alphaCutoffProp = Mat->additionalValues.find("alphaCutoff");
				if (alphaCutoffProp != Mat->additionalValues.end())
				{
					tinygltf::Parameter &param = alphaCutoffProp->second;
					if (param.number_array.size() > 0)
					{
						UnrealMaterial->OpacityMaskClipValue = param.number_array[0];
					}
				}
			}
		}


		/*
		//KB: Leaving here for reference later on when I add support for skeletal meshes.
		if (bForSkeletalMesh)
		{
			bool bNeedsRecompile = false;
			UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
		}
		*/

		// Set the dirty flag so this package will get saved later
		Package->SetDirtyFlag(true);

		FScopedSlowTask MaterialProgress(8.0, LOCTEXT("ImportingGLTFMaterial", "Creating glTF Material Nodes"));

		FVector2D location(-260, -260);

		SharedTextureMap texMap;

		bool usingPBR = true;
		if (!CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "baseColorTexture", TextureType_PBR, UnrealMaterial->BaseColor, false, location))
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "diffuseTexture", TextureType_SPEC, UnrealMaterial->BaseColor, false, location);
			usingPBR = false;
		}

		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "emissiveTexture", TextureType_DEFAULT, UnrealMaterial->EmissiveColor, false, location);

		if (usingPBR)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Roughness, false, location, ColorChannel_Green);
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Metallic, false, location, ColorChannel_Blue);
		}
		else
		{
			//For now I am ignoring the specular data (rgb) since it makes materials too dark. The method does work, but due to the look its disabled for now.
			//CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "specularGlossinessTexture", TextureType_SPEC, UnrealMaterial->Specular, false, location);

			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "specularGlossinessTexture", TextureType_SPEC, UnrealMaterial->Roughness, false, location, ColorChannel_Alpha);
		}

		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "normalTexture", TextureType_DEFAULT, UnrealMaterial->Normal, true, location);
		CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, "occlusionTexture", TextureType_DEFAULT, UnrealMaterial->AmbientOcclusion, false, location, ColorChannel_Red);


		const char *opactityMaterial = "baseColorTexture";
		TextureType opactityType = TextureType_PBR;
		if (!usingPBR)
		{
			opactityMaterial = "diffuseTexture";
			opactityType = TextureType_SPEC;
		}

		if (UnrealMaterial->BlendMode == BLEND_Translucent)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, opactityMaterial, opactityType, UnrealMaterial->Opacity, false, location, ColorChannel_Alpha);
		}
		else if (UnrealMaterial->BlendMode == BLEND_Masked)
		{
			CreateAndLinkExpressionForMaterialProperty(MaterialProgress, ImportContext, Mat, UnrealMaterial, texMap, opactityMaterial, opactityType, UnrealMaterial->OpacityMask, false, location, ColorChannel_Alpha);
		}


		//KB: Leave this here as a reference, in case I need to add a diffuse channel. Perhaps it causes a bug if the glTF file does not contain a BaseColor for some reason?
		//FixupMaterial(FbxMaterial, UnrealMaterial); // add random diffuse if none exists

		if (UnrealMaterial)
		{
			// let the material update itself if necessary
			UnrealMaterial->PreEditChange(NULL);
			UnrealMaterial->PostEditChange();

			OutMaterials.Add(UnrealMaterial);
		}
	}
}

void CreateMultiplyExpression(UMaterial* UnrealMaterial, FExpressionInput& MaterialInput, UMaterialExpression *ExpressionFactor, UMaterialExpression *UnrealTextureExpression, ColorChannel colorChannel)
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
		if (Outputs.Num() == 5)
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
	if (Outputs.Num() == 5)
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

void UGLTFImporter::AttachOutputs(FExpressionInput& MaterialInput, ColorChannel colorChannel)
{
	if (MaterialInput.Expression)
	{
		TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
		FExpressionOutput* Output = nullptr;

		if (Outputs.Num() == 5)
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

bool UGLTFImporter::CreateAndLinkExpressionForMaterialProperty(
	FScopedSlowTask &MaterialProgress,
	FGltfImportContext& ImportContext,
	tinygltf::Material *mat,
	UMaterial* UnrealMaterial,
	SharedTextureMap &texMap,
	const char* MaterialProperty,
	TextureType texType,
	FExpressionInput& MaterialInput,
	bool bSetupAsNormalMap,
	FVector2D& Location,
	ColorChannel colorChannel)
{
	MaterialProgress.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("ImportingGLTFMaterial", "Creating Material Node {0}"), FText::FromString(GLTFToUnreal::ConvertString(MaterialProperty))));

	bool bCreated = false;

	// Based on what we are creating get the data from the different location in the tinygltf::Material datastructure.
	tinygltf::ParameterMap *map = nullptr;
	switch (texType)
	{
	case TextureType_PBR:
		map = &mat->values;
		break;
	case TextureType_DEFAULT:
		map = &mat->additionalValues;
		break;
	case TextureType_SPEC:
		map = &mat->extPBRValues;
		break;
	}

	// This method is currently used to do all the material attachments 
	// This enum is just used to identify what channel we are actually dealing with
	// TODO: For readability refactor each channel out into its own method
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
	};

	PBRTYPE pbrType = PBRTYPE_Undefined;

	UMaterialExpressionVectorParameter *baseColorFactor = nullptr;
	UMaterialExpressionScalarParameter *metallicFactor = nullptr;
	UMaterialExpressionScalarParameter *roughnessFactor = nullptr;
	UMaterialExpressionVectorParameter *emissiveFactor = nullptr;
	UMaterialExpressionVectorParameter *specularFactor = nullptr;
	UMaterialExpressionVectorParameter *diffuseFactor = nullptr;
	UMaterialExpressionScalarParameter *glossinessFactor = nullptr;

	UMaterialExpressionTextureSample* UnrealTextureExpression = nullptr;

	if (strcmp(MaterialProperty, "baseColorTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_Color;

		const auto &baseColorProp = map->find("baseColorFactor");
		if (baseColorProp != map->end())
		{
			tinygltf::Parameter &param = baseColorProp->second;
			if (param.number_array.size() == 4)
			{
				baseColorFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (baseColorFactor)
				{
					if (baseColorFactor->CanRenameNode())
					{
						baseColorFactor->SetEditableName(GLTFToUnreal::ConvertString("baseColorFactor"));
					}

					UnrealMaterial->Expressions.Add(baseColorFactor);
					baseColorFactor->DefaultValue.R = param.number_array[0];
					baseColorFactor->DefaultValue.G = param.number_array[1];
					baseColorFactor->DefaultValue.B = param.number_array[2];
					baseColorFactor->DefaultValue.A = param.number_array[3];

					//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
					const auto &baseColorTextureProp = map->find("baseColorTexture");
					if (baseColorTextureProp == map->end())
					{
						MaterialInput.Expression = baseColorFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "baseColorTexture") == 0 && colorChannel == ColorChannel_Alpha)
	{
		pbrType = PBRTYPE_Opacity;

		const auto &baseColorProp = map->find("baseColorFactor");
		if (baseColorProp != map->end())
		{
			tinygltf::Parameter &param = baseColorProp->second;
			if (param.number_array.size() == 4)
			{
				//If there is no baseColorTexture then we just use this alpha part of the baseColorFactor and hook it up directly to the material
				const auto &baseColorTextureProp = map->find("baseColorTexture");
				if (baseColorTextureProp == map->end())
				{
					UMaterialExpressionScalarParameter *opacityFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
					if (opacityFactor)
					{
						if (opacityFactor->CanRenameNode())
						{
							opacityFactor->SetEditableName(GLTFToUnreal::ConvertString("opacityFactor"));
						}

						UnrealMaterial->Expressions.Add(opacityFactor);
						opacityFactor->DefaultValue = param.number_array[3];
						MaterialInput.Expression = opacityFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "diffuseTexture") == 0 && colorChannel == ColorChannel_All)
	{
		pbrType = PBRTYPE_Diffuse;

		const auto &diffuseProp = map->find("diffuseFactor");
		if (diffuseProp != map->end())
		{
			tinygltf::Parameter &param = diffuseProp->second;
			if (param.number_array.size() == 4)
			{
				diffuseFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (diffuseFactor)
				{
					if (diffuseFactor->CanRenameNode())
					{
						diffuseFactor->SetEditableName(GLTFToUnreal::ConvertString("diffuseFactor"));
					}

					UnrealMaterial->Expressions.Add(diffuseFactor);
					diffuseFactor->DefaultValue.R = param.number_array[0];
					diffuseFactor->DefaultValue.G = param.number_array[1];
					diffuseFactor->DefaultValue.B = param.number_array[2];
					diffuseFactor->DefaultValue.A = param.number_array[3];

					//If there is no diffuseTexture then we just use this color by itself and hook it up directly to the material
					const auto &diffuseTextureProp = map->find("diffuseTexture");
					if (diffuseTextureProp == map->end())
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

		const auto &diffuseProp = map->find("diffuseFactor");
		if (diffuseProp != map->end())
		{
			tinygltf::Parameter &param = diffuseProp->second;
			if (param.number_array.size() == 4)
			{
				//If there is no diffuseTexture then we just use this color by itself and hook it up directly to the material
				const auto &diffuseTextureProp = map->find("diffuseTexture");
				if (diffuseTextureProp == map->end())
				{
					UMaterialExpressionScalarParameter *opacityFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
					if (opacityFactor)
					{
						if (opacityFactor->CanRenameNode())
						{
							opacityFactor->SetEditableName(GLTFToUnreal::ConvertString("opacityFactor"));
						}

						UnrealMaterial->Expressions.Add(opacityFactor);
						opacityFactor->DefaultValue = param.number_array[3];
						MaterialInput.Expression = opacityFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "specularGlossinessTexture") == 0 && colorChannel == ColorChannel_All) //RGB is used for specular channel
	{
		pbrType = PBRTYPE_Specular;

		const auto &specularProp = map->find("specularFactor");
		if (specularProp != map->end())
		{
			tinygltf::Parameter &param = specularProp->second;
			if (param.number_array.size() == 3)
			{
				specularFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (specularFactor)
				{
					if (specularFactor->CanRenameNode())
					{
						specularFactor->SetEditableName(GLTFToUnreal::ConvertString("specularFactor"));
					}

					UnrealMaterial->Expressions.Add(specularFactor);
					specularFactor->DefaultValue.R = param.number_array[0];
					specularFactor->DefaultValue.G = param.number_array[1];
					specularFactor->DefaultValue.B = param.number_array[2];
					specularFactor->DefaultValue.A = 1.0;

					//If there is no specularTexture then we just use this color by itself and hook it up directly to the material
					const auto &specularTextureProp = map->find("specularGlossinessTexture");
					if (specularTextureProp == map->end())
					{
						MaterialInput.Expression = specularFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "specularGlossinessTexture") == 0 && colorChannel == ColorChannel_Alpha) //Alpha is used for roughness channel with an additional negation node (1 - value).
	{
		pbrType = PBRTYPE_Glossiness;

		const auto &glossinessProp = map->find("glossinessFactor");
		if (glossinessProp != map->end())
		{
			tinygltf::Parameter &param = glossinessProp->second;
			if (param.number_array.size() == 1)
			{
				glossinessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (glossinessFactor)
				{
					if (glossinessFactor->CanRenameNode())
					{
						glossinessFactor->SetEditableName(GLTFToUnreal::ConvertString("glossinessFactor"));
					}

					UnrealMaterial->Expressions.Add(glossinessFactor);
					glossinessFactor->DefaultValue = param.number_array[0];

					//If there is no specularTexture then we just use this color by itself and hook it up directly to the material
					const auto &specularGlossinessTextureProp = map->find("specularGlossinessTexture");
					if (specularGlossinessTextureProp == map->end())
					{
						MaterialInput.Expression = glossinessFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "metallicRoughnessTexture") == 0 && colorChannel == ColorChannel_Blue) //Metallic comes from the blue channel of metallicRoughnessTexture
	{
		pbrType = PBRTYPE_Metallic;

		const auto &metallicProp = map->find("metallicFactor");
		if (metallicProp != map->end())
		{
			tinygltf::Parameter &param = metallicProp->second;
			if (param.number_array.size() == 1)
			{
				metallicFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (metallicFactor)
				{
					if (metallicFactor->CanRenameNode())
					{
						metallicFactor->SetEditableName(GLTFToUnreal::ConvertString("metallicFactor"));
					}

					UnrealMaterial->Expressions.Add(metallicFactor);
					metallicFactor->DefaultValue = param.number_array[0];

					//If there is no metallicRoughnessTexture then we just use this color by itself and hook it up directly to the material
					const auto &metallicRoghnessTextureProp = map->find("metallicRoughnessTexture");
					if (metallicRoghnessTextureProp == map->end())
					{
						MaterialInput.Expression = metallicFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}
	else if (strcmp(MaterialProperty, "metallicRoughnessTexture") == 0 && colorChannel == ColorChannel_Green) 	//roughness comes from the green channel of metallicRoughnessTexture
	{
		pbrType = PBRTYPE_Roughness;

		const auto &roughnessProp = map->find("roughnessFactor");
		if (roughnessProp != map->end())
		{
			tinygltf::Parameter &param = roughnessProp->second;
			if (param.number_array.size() == 1)
			{
				roughnessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
				if (roughnessFactor)
				{
					if (roughnessFactor->CanRenameNode())
					{
						roughnessFactor->SetEditableName(GLTFToUnreal::ConvertString("roughnessFactor"));
					}

					UnrealMaterial->Expressions.Add(roughnessFactor);
					roughnessFactor->DefaultValue = param.number_array[0];

					//If there is no metallicRoughnessTexture then we just use this color by itself and hook it up directly to the material
					const auto &metallicRoghnessTextureProp = map->find("metallicRoughnessTexture");
					if (metallicRoghnessTextureProp == map->end())
					{
						MaterialInput.Expression = roughnessFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}

				}
			}
		}
	}
	else  if (strcmp(MaterialProperty, "emissiveTexture") == 0)
	{
		pbrType = PBRTYPE_Emissive;

		const auto &roughnessProp = map->find("emissiveFactor");
		if (roughnessProp != map->end())
		{
			tinygltf::Parameter &param = roughnessProp->second;
			if (param.number_array.size() == 3)
			{
				emissiveFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
				if (emissiveFactor)
				{
					if (emissiveFactor->CanRenameNode())
					{
						emissiveFactor->SetEditableName(GLTFToUnreal::ConvertString("emissiveFactor"));
					}

					UnrealMaterial->Expressions.Add(emissiveFactor);
					emissiveFactor->DefaultValue.R = param.number_array[0];
					emissiveFactor->DefaultValue.G = param.number_array[1];
					emissiveFactor->DefaultValue.B = param.number_array[2];
					emissiveFactor->DefaultValue.A = 1.0;

					//If there is no emissiveTexture then we just use this color by itself and hook it up directly to the material
					const auto &emissiveTetxureProp = map->find("emissiveTexture");
					if (emissiveTetxureProp == map->end())
					{
						MaterialInput.Expression = emissiveFactor;
						AttachOutputs(MaterialInput, ColorChannel_All);
						return true;
					}
				}
			}
		}
	}

	// Find the image and add it to the material 
	const auto &property = map->find(MaterialProperty);
	if (property != map->end())
	{
		tinygltf::Parameter &param = property->second;
		if (param.json_double_value.size() > 0)
		{
			float scaleValue = 1.0;
			const auto &scaleEntry = param.json_double_value.find("scale");
			if (scaleEntry != param.json_double_value.end())
			{
				scaleValue = scaleEntry->second;
			}

			int32 texCoordValue = 0;
			const auto &texCoordEntry = param.json_double_value.find("textCoord");
			if (texCoordEntry != param.json_double_value.end())
			{
				texCoordValue = (int32)texCoordEntry->second;
			}

			const auto &textureIndexEntry = param.json_double_value.find("index");
			if (textureIndexEntry != param.json_double_value.end())
			{
				int32 textureIndex = textureIndexEntry->second;
				if (textureIndex >= 0 && textureIndex < ImportContext.Model->textures.size())
				{
					tinygltf::Texture &texture = ImportContext.Model->textures[textureIndex];
					int32 source = texture.source;

					if (source >= 0 && source < ImportContext.Model->images.size())
					{
						tinygltf::Image &img = ImportContext.Model->images[source];

						//See if we have already loaded in this image
						SharedTexture *sharedMap = texMap.Find(source);
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
							UTexture* UnrealTexture = ImportTexture(ImportContext, &img, bSetupAsNormalMap, MaterialProperty);
							if (UnrealTexture)
							{
								float ScaleU = 1.0;  
								float ScaleV = 1.0; 

								/*
								//TODO: Respect the sampler settings
								if (texture.sampler >= 0 && texture.sampler < ImportContext.Model->samplers.size())
								{
									const auto &sampler = ImportContext.Model->samplers[texture.sampler];
								}
								*/

								// and link it to the material 
								UnrealTextureExpression = NewObject<UMaterialExpressionTextureSample>(UnrealMaterial);
								if (UnrealTextureExpression)
								{
									UnrealMaterial->Expressions.Add(UnrealTextureExpression);
									MaterialInput.Expression = UnrealTextureExpression;
									UnrealTextureExpression->Texture = UnrealTexture;
									UnrealTextureExpression->SamplerType = bSetupAsNormalMap ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
									UnrealTextureExpression->MaterialExpressionEditorX = FMath::TruncToInt(Location.X);
									UnrealTextureExpression->MaterialExpressionEditorY = FMath::TruncToInt(Location.Y);

									Location.Y += 200;

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
					const auto &textureScaleEntry = param.json_double_value.find("scale");
					if (textureScaleEntry != param.json_double_value.end())
					{
						UMaterialExpressionScalarParameter *scaleFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
						if (scaleFactor)
						{
							if (scaleFactor->CanRenameNode())
							{
								scaleFactor->SetEditableName(GLTFToUnreal::ConvertString("normalTextureScale"));
							}

							UnrealMaterial->Expressions.Add(scaleFactor);
							scaleFactor->DefaultValue = textureScaleEntry->second;
							CreateMultiplyExpression(UnrealMaterial, MaterialInput, scaleFactor, UnrealTextureExpression, ColorChannel_All);
						}
					}
				}

				// Special case for occlusion (since there is no occlusionFactor like other channels)
				// occlusion has a strength. 
				else if (strcmp(MaterialProperty, "occlusionTexture") == 0)
				{
					const auto &textureStrengthEntry = param.json_double_value.find("strength");
					if (textureStrengthEntry != param.json_double_value.end())
					{
						UMaterialExpressionScalarParameter *strengthFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
						if (strengthFactor)
						{
							if (strengthFactor->CanRenameNode())
							{
								strengthFactor->SetEditableName(GLTFToUnreal::ConvertString("occlusionTextureStrength"));
							}

							UnrealMaterial->Expressions.Add(strengthFactor);
							strengthFactor->DefaultValue = textureStrengthEntry->second;

							CreateMultiplyExpression(UnrealMaterial, MaterialInput, strengthFactor, UnrealTextureExpression, ColorChannel_Red);

							colorChannel = ColorChannel_All; 
						}
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
				}
				break;
				default: break;
				}

				AttachOutputs(MaterialInput, colorChannel);
			}
		}
	}

	return bCreated;
}

int32 UGLTFImporter::CreateNodeMaterials(FGltfImportContext &ImportContext, TArray<UMaterialInterface*>& OutMaterials)
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

void FGltfImportContext::Init(UObject* InParent, const FString& InName, const FString &InBasePath, tinygltf::Model* InModel)
{
	Parent = InParent;
	ObjectName = InName;
	BasePath = InBasePath;
	ImportPathName = InParent->GetOutermost()->GetName();

	// Path should not include the filename
	ImportPathName.RemoveFromEnd(FString(TEXT("/")) + InName);

	ImportObjectFlags = RF_Public | RF_Standalone | RF_Transactional;

	TSubclassOf<UGLTFPrimResolver> ResolverClass = GetDefault<UGLTFImporterProjectSettings>()->CustomPrimResolver;
	if (!ResolverClass)
	{
		ResolverClass = UGLTFPrimResolver::StaticClass();
	}

	PrimResolver = NewObject<UGLTFPrimResolver>(GetTransientPackage(), ResolverClass);
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
	bApplyWorldTransformToGeometry = false;
	bFindUnrealAssetReferences = false;
}

void FGltfImportContext::AddErrorMessage(EMessageSeverity::Type MessageSeverity, FText ErrorMessage)
{
	TokenizedErrorMessages.Add(FTokenizedMessage::Create(MessageSeverity, ErrorMessage));
}

void FGltfImportContext::DisplayErrorMessages(bool bAutomated)
{
	if (!bAutomated)
	{
		//Always clear the old message after an import or re-import
		const TCHAR* LogTitle = TEXT("GLTFImport");
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		TSharedPtr<class IMessageLogListing> LogListing = MessageLogModule.GetLogListing(LogTitle);
		LogListing->SetLabel(FText::FromString("USD Import"));
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

void FGltfImportContext::ClearErrorMessages()
{
	TokenizedErrorMessages.Empty();
}

#undef LOCTEXT_NAMESPACE

