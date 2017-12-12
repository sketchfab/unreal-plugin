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
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "ARFilter.h"
#include "Factories/MaterialImportHelpers.h"
#include "EditorFramework/AssetImportData.h"
#include "Materials/MaterialExpressionMultiply.h"


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


/*
const FUsdGeomData* FUsdPrimToImport::GetGeomData(int32 LODIndex, double Time) const
{
	if (NumLODs == 0)
	{
		return Prim->GetGeometryData(Time);
	}
	else
	{
		IUsdPrim* Child = Prim->GetLODChild(LODIndex);
		return Child->GetGeometryData(Time);
	}
}
*/

UGLTFImporter::UGLTFImporter(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

UObject* UGLTFImporter::ImportMeshes(FGltfImportContext& ImportContext, const TArray<FGltfPrimToImport>& PrimsToImport)
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ImportingGLTFMeshes", "Importing GLTF Meshes"));
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
			if(!ImportContext.PathToImportAssetMap.Contains(NewPackageName))
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

		if(bShouldImport)
		{
			UStaticMesh* NewMesh = ImportSingleMesh(ImportContext, MeshImportType, PrimToImport, singleStaticMesh);
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

UStaticMesh* UGLTFImporter::ImportSingleMesh(FGltfImportContext& ImportContext, EGltfMeshImportType ImportType, const FGltfPrimToImport& PrimToImport, UStaticMesh *singleMesh)
{
	UStaticMesh* NewMesh = nullptr;

 	if (ImportType == EGltfMeshImportType::StaticMesh)
 	{
 		NewMesh = FGLTFStaticMeshImporter::ImportStaticMesh(ImportContext, PrimToImport, singleMesh);
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
	//FilePath = FPaths::GetPath(FilePath) + TEXT("/");
	FString CleanFilename = FPaths::GetCleanFilename(Filename);

	//TODO: Clean up
	//I am  just create a Model here directly, it needs to be deleted when finished.
	tinygltf::Model* Model = new tinygltf::Model(); // UnrealUSDWrapper::ImportUSDFile(TCHAR_TO_ANSI(*FilePath), TCHAR_TO_ANSI(*CleanFilename));

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

UTexture* UGLTFImporter::ImportTexture(FGltfImportContext& ImportContext, tinygltf::Image *img, bool bSetupAsNormalMap)
{
	if (!img)
	{
		return nullptr;
	}

	const FString &FileBasePath = ImportContext.BasePath;

	// create an unreal texture asset
	UTexture* UnrealTexture = NULL;
	FString AbsoluteFilename = GLTFToUnreal::ConvertString(img->uri);
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

void UGLTFImporter::CreateUnrealMaterial(FGltfImportContext& ImportContext, tinygltf::Material *mat, TArray<UMaterialInterface*>& OutMaterials)
{
	// Make sure we have a parent
	//if (!ensure(ImportContext.Parent.IsValid()))
	if (!ImportContext.Parent)
	{
		return;
	}

	/*
	if (ImportOptions->OverrideMaterials.Contains(FbxMaterial.GetUniqueID()))
	{
		UMaterialInterface* FoundMaterial = *(ImportOptions->OverrideMaterials.Find(FbxMaterial.GetUniqueID()));
		if (ImportedMaterialData.IsUnique(FbxMaterial, FName(*FoundMaterial->GetPathName())) == false)
		{
			ImportedMaterialData.AddImportedMaterial(FbxMaterial, *FoundMaterial);
		}
		// The material is override add the existing one
		OutMaterials.Add(FoundMaterial);
		return;
	}
	*/

	FString MaterialFullName = GLTFToUnreal::ConvertString(mat->name); //GetMaterialFullName(FbxMaterial);
	FString BasePackageName = FPackageName::GetLongPackagePath(ImportContext.Parent->GetOutermost()->GetName());

	/*
	if (ImportOptions->MaterialBasePath != NAME_None)
	{
		BasePackageName = ImportOptions->MaterialBasePath.ToString();
	}
	else
	{
		BasePackageName += TEXT("/");
	}
	*/
	BasePackageName += TEXT("/");

	BasePackageName = PackageTools::SanitizePackageName(BasePackageName);

	// The material could already exist in the project
	FName ObjectPath = *(BasePackageName + TEXT(".") + MaterialFullName);

	/*
	if (ImportedMaterialData.IsUnique(FbxMaterial, ObjectPath))
	{
		UMaterialInterface* FoundMaterial = ImportedMaterialData.GetUnrealMaterial(FbxMaterial);
		if (FoundMaterial)
		{
			// The material was imported from this FBX.  Reuse it
			OutMaterials.Add(FoundMaterial);
			return;
		}
	}
	else
	{
		FBXImportOptions* FbxImportOptions = GetImportOptions();

		FText Error;
		UMaterialInterface* FoundMaterial = UMaterialImportHelpers::FindExistingMaterialFromSearchLocation(ObjectPath.ToString(), BasePackageName, FbxImportOptions->MaterialSearchLocation, Error);

		if (!Error.IsEmpty())
		{
			AddTokenizedErrorMessage(
				FTokenizedMessage::Create(EMessageSeverity::Warning,
					FText::Format(LOCTEXT("FbxMaterialImport_MultipleMaterialsFound", "While importing '{0}': {1}"),
						FText::FromString(Parent->GetOutermost()->GetName()),
						Error)),
				FFbxErrors::Generic_LoadingSceneFailed);
		}
		// do not override existing materials
		if (FoundMaterial)
		{
			ImportedMaterialData.AddImportedMaterial(FbxMaterial, *FoundMaterial);
			OutMaterials.Add(FoundMaterial);
			return;
		}
	}
	*/

	const FString Suffix(TEXT(""));
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString FinalPackageName;
	FString OutAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, OutAssetName);

	UPackage* Package = CreatePackage(NULL, *FinalPackageName);

	// Check if we can use the specified base material to instance from it
	/*
	FBXImportOptions* FbxImportOptions = GetImportOptions();
	bool bCanInstance = false;
	if (FbxImportOptions->BaseMaterial)
	{
		bCanInstance = false;
		// try to use the material as a base for the new material to instance from
		FbxProperty FbxDiffuseProperty = FbxMaterial.FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (FbxDiffuseProperty.IsValid())
		{
			bCanInstance = CanUseMaterialWithInstance(FbxMaterial, FbxSurfaceMaterial::sDiffuse, FbxImportOptions->BaseDiffuseTextureName, FbxImportOptions->BaseMaterial, UVSets);
		}
		else
		{
			bCanInstance = !FbxImportOptions->BaseColorName.IsEmpty();
		}
		FbxProperty FbxEmissiveProperty = FbxMaterial.FindProperty(FbxSurfaceMaterial::sEmissive);
		if (FbxDiffuseProperty.IsValid())
		{
			bCanInstance &= CanUseMaterialWithInstance(FbxMaterial, FbxSurfaceMaterial::sEmissive, FbxImportOptions->BaseEmmisiveTextureName, FbxImportOptions->BaseMaterial, UVSets);
		}
		else
		{
			bCanInstance &= !FbxImportOptions->BaseEmissiveColorName.IsEmpty();
		}
		bCanInstance &= CanUseMaterialWithInstance(FbxMaterial, FbxSurfaceMaterial::sSpecular, FbxImportOptions->BaseSpecularTextureName, FbxImportOptions->BaseMaterial, UVSets);
		bCanInstance &= CanUseMaterialWithInstance(FbxMaterial, FbxSurfaceMaterial::sNormalMap, FbxImportOptions->BaseNormalTextureName, FbxImportOptions->BaseMaterial, UVSets);
	}
	*/

	UMaterialInterface* UnrealMaterialFinal = nullptr;

	/*
	if (bCanInstance) {
		const auto &MaterialInstanceFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
		MaterialInstanceFactory->InitialParent = FbxImportOptions->BaseMaterial;
		UMaterialInstanceConstant* UnrealMaterialConstant = (UMaterialInstanceConstant*)MaterialInstanceFactory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), Package, *MaterialFullName, RF_Standalone | RF_Public, NULL, GWarn);
		if (UnrealMaterialConstant != NULL)
		{
			UnrealMaterialFinal = UnrealMaterialConstant;
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(UnrealMaterialConstant);

			// Set the dirty flag so this package will get saved later
			Package->SetDirtyFlag(true);

			//UnrealMaterialConstant->SetParentEditorOnly(FbxImportOptions->BaseMaterial);


			// textures and properties
			bool bDiffuseTextureCreated = LinkMaterialProperty(FbxMaterial, UnrealMaterialConstant, FbxSurfaceMaterial::sDiffuse, FName(*FbxImportOptions->BaseDiffuseTextureName), false);
			bool bEmissiveTextureCreated = LinkMaterialProperty(FbxMaterial, UnrealMaterialConstant, FbxSurfaceMaterial::sEmissive, FName(*FbxImportOptions->BaseEmmisiveTextureName), false);
			LinkMaterialProperty(FbxMaterial, UnrealMaterialConstant, FbxSurfaceMaterial::sSpecular, FName(*FbxImportOptions->BaseSpecularTextureName), false);
			if (!LinkMaterialProperty(FbxMaterial, UnrealMaterialConstant, FbxSurfaceMaterial::sNormalMap, FName(*FbxImportOptions->BaseNormalTextureName), true))
			{
				LinkMaterialProperty(FbxMaterial, UnrealMaterialConstant, FbxSurfaceMaterial::sBump, FName(*FbxImportOptions->BaseNormalTextureName), true); // no bump in unreal, use as normal map
			}

			// If we only have colors and its different from the base material
			if (!bDiffuseTextureCreated)
			{
				FbxDouble3 DiffuseColor;
				bool OverrideColor = false;

				if (FbxMaterial.GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					DiffuseColor = ((FbxSurfacePhong&)(FbxMaterial)).Diffuse.Get();
					OverrideColor = true;
				}
				else if (FbxMaterial.GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					DiffuseColor = ((FbxSurfaceLambert&)(FbxMaterial)).Diffuse.Get();
					OverrideColor = true;
				}
				if (OverrideColor)
				{
					FLinearColor LinearColor((float)DiffuseColor[0], (float)DiffuseColor[1], (float)DiffuseColor[2]);
					FLinearColor CurrentLinearColor;
					if (UnrealMaterialConstant->GetVectorParameterValue(FName(*FbxImportOptions->BaseColorName), CurrentLinearColor))
					{
						//Alpha is not consider for diffuse color
						LinearColor.A = CurrentLinearColor.A;
						if (!CurrentLinearColor.Equals(LinearColor))
						{
							UnrealMaterialConstant->SetVectorParameterValueEditorOnly(FName(*FbxImportOptions->BaseColorName), LinearColor);
						}
					}
				}
			}
			if (!bEmissiveTextureCreated)
			{
				FbxDouble3 EmissiveColor;
				bool OverrideColor = false;

				if (FbxMaterial.GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					EmissiveColor = ((FbxSurfacePhong&)(FbxMaterial)).Emissive.Get();
					OverrideColor = true;
				}
				else if (FbxMaterial.GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					EmissiveColor = ((FbxSurfaceLambert&)(FbxMaterial)).Emissive.Get();
					OverrideColor = true;
				}
				if (OverrideColor)
				{
					FLinearColor LinearColor((float)EmissiveColor[0], (float)EmissiveColor[1], (float)EmissiveColor[2]);
					FLinearColor CurrentLinearColor;
					if (UnrealMaterialConstant->GetVectorParameterValue(FName(*FbxImportOptions->BaseEmissiveColorName), CurrentLinearColor))
					{
						//Alpha is not consider for emissive color
						LinearColor.A = CurrentLinearColor.A;
						if (!CurrentLinearColor.Equals(LinearColor))
						{
							UnrealMaterialConstant->SetVectorParameterValueEditorOnly(FName(*FbxImportOptions->BaseEmissiveColorName), LinearColor);
						}
					}
				}
			}
		}
	}
	else
	*/
	{
		// create an unreal material asset
		auto MaterialFactory = NewObject<UMaterialFactoryNew>();

		UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(
			UMaterial::StaticClass(), Package, *MaterialFullName, RF_Standalone | RF_Public, NULL, GWarn);

		if (UnrealMaterial != NULL)
		{
			UnrealMaterialFinal = UnrealMaterial;
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(UnrealMaterial);

			UnrealMaterial->TwoSided = false;
			const auto &doubleSidedProp = mat->additionalValues.find("doubleSided");
			if (doubleSidedProp != mat->additionalValues.end())
			{
				tinygltf::Parameter &param = doubleSidedProp->second;
				UnrealMaterial->TwoSided = param.bool_value;
			}

			/*
			if (bForSkeletalMesh)
			{
				bool bNeedsRecompile = false;
				UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
			}
			*/

			// Set the dirty flag so this package will get saved later
			Package->SetDirtyFlag(true);

			// textures and properties
#if DEBUG_LOG_FBX_MATERIAL_PROPERTIES
			const FbxProperty &FirstProperty = FbxMaterial.GetFirstProperty();
			if (FirstProperty.IsValid())
			{
				UE_LOG(LogFbxMaterialImport, Display, TEXT("Creating Material [%s]"), UTF8_TO_TCHAR(FbxMaterial.GetName()));
				LogPropertyAndChild(FbxMaterial, FirstProperty);
				UE_LOG(LogFbxMaterialImport, Display, TEXT("-------------------------------"));
			}
#endif
			if (!CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "baseColorTexture", TextureType_PBR, UnrealMaterial->BaseColor, false, FVector2D(-240, -320)))
			{
				CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "diffuseTexture", TextureType_SPEC, UnrealMaterial->BaseColor, false, FVector2D(-240, -320));
			}

			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "emissiveTexture", TextureType_DEFAULT, UnrealMaterial->EmissiveColor, false, FVector2D(-240, -64));
			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "specularTexture", TextureType_SPEC, UnrealMaterial->Specular, false, FVector2D(-240, -128));
			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Roughness, false, FVector2D(-240, -180), 1);
			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "metallicRoughnessTexture", TextureType_PBR, UnrealMaterial->Metallic, false, FVector2D(-240, -210), 2);
			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "normalTexture", TextureType_DEFAULT, UnrealMaterial->Normal, true, FVector2D(-240, 256));

			/*
			if (CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "opacity", false, UnrealMaterial->Opacity, false, FVector2D(200, 256)))
			{
				UnrealMaterial->BlendMode = BLEND_Translucent;
				CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "opacitymask", false, UnrealMaterial->OpacityMask, false, FVector2D(150, 256));
			}
			*/

			CreateAndLinkExpressionForMaterialProperty(ImportContext, mat, UnrealMaterial, "occlusionTexture", TextureType_DEFAULT, UnrealMaterial->AmbientOcclusion, false, FVector2D(-240, -310), 0);

			//FixupMaterial(FbxMaterial, UnrealMaterial); // add random diffuse if none exists
		}

		// compile shaders for PC (from UPrecompileShadersCommandlet::ProcessMaterial
		// and FMaterialEditor::UpdateOriginalMaterial)
	}
	if (UnrealMaterialFinal)
	{
		// let the material update itself if necessary
		UnrealMaterialFinal->PreEditChange(NULL);
		UnrealMaterialFinal->PostEditChange();

		//ImportedMaterialData.AddImportedMaterial(FbxMaterial, *UnrealMaterialFinal);

		OutMaterials.Add(UnrealMaterialFinal);
	}
}

bool UGLTFImporter::CreateAndLinkExpressionForMaterialProperty(
	FGltfImportContext& ImportContext,
	tinygltf::Material *mat,
	UMaterial* UnrealMaterial,
	const char* MaterialProperty,
	TextureType texType,
	FExpressionInput& MaterialInput,
	bool bSetupAsNormalMap,
	const FVector2D& Location,
	int32 colorChannel)
{
	bool bCreated = false;

	tinygltf::ParameterMap *map = nullptr;
	if (texType == TextureType_PBR)
	{
		map = &mat->values;
	}
	else if (texType == TextureType_DEFAULT)
	{
		map = &mat->additionalValues;
	}
	else if (texType == TextureType_SPEC)
	{
		map = &mat->extPBRValues;
	}

	enum PBRTYPE
	{
		PBRTYPE_Color,
		PBRTYPE_Metallic,
		PBRTYPE_Roughness,
		PBRTYPE_Emissive,
		PBRTYPE_Specular,
		PBRTYPE_Diffuse
	} pbrType;

	UMaterialExpressionVectorParameter *baseColorFactor = nullptr;
	UMaterialExpressionScalarParameter *metallicFactor = nullptr;
	UMaterialExpressionScalarParameter *roughnessFactor = nullptr;
	UMaterialExpressionVectorParameter *emissiveFactor = nullptr;
	UMaterialExpressionTextureSample* UnrealTextureExpression = nullptr;

	UMaterialExpressionVectorParameter *specularFactor = nullptr;
	UMaterialExpressionVectorParameter *diffuseFactor = nullptr;
	{
		const auto &metallicRoghnessTextureProp = map->find("metallicRoughnessTexture");
		const auto &baseColorTextureProp = map->find("baseColorTexture");
		const auto &emissiveTetxureProp = map->find("emissiveTexture");
		const auto &diffuseTextureProp = map->find("diffuseTexture");
		const auto &specularTextureProp = map->find("specularTexture");
		if (FString(MaterialProperty) == "baseColorTexture")
		{
			pbrType = PBRTYPE_Color;

			const auto &baseColorProp = map->find("baseColorFactor");
			if (baseColorProp != map->end())
			{
				tinygltf::Parameter &param = baseColorProp->second;
				if (param.number_array.size() == 4)
				{
					// Create a texture coord node for the texture sample
					baseColorFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
					if (baseColorFactor)
					{
						UnrealMaterial->Expressions.Add(baseColorFactor);
						baseColorFactor->DefaultValue.R = param.number_array[0];
						baseColorFactor->DefaultValue.G = param.number_array[1];
						baseColorFactor->DefaultValue.B = param.number_array[2];
						baseColorFactor->DefaultValue.A = param.number_array[3];

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (baseColorTextureProp == map->end())
						{
							MaterialInput.Expression = baseColorFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}
					}
				}
			}
		}

		if (FString(MaterialProperty) == "specularTexture")
		{
			pbrType = PBRTYPE_Specular;

			const auto &specularProp = map->find("specularFactor");
			if (specularProp != map->end())
			{
				tinygltf::Parameter &param = specularProp->second;
				if (param.number_array.size() == 3)
				{
					// Create a texture coord node for the texture sample
					specularFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
					if (specularFactor)
					{
						UnrealMaterial->Expressions.Add(specularFactor);
						specularFactor->DefaultValue.R = param.number_array[0];
						specularFactor->DefaultValue.G = param.number_array[1];
						specularFactor->DefaultValue.B = param.number_array[2];
						specularFactor->DefaultValue.A = 1.0;

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (specularTextureProp == map->end())
						{
							MaterialInput.Expression = specularFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}
					}
				}
			}
		}

		if (FString(MaterialProperty) == "diffuseTexture")
		{
			pbrType = PBRTYPE_Diffuse;

			const auto &diffuseProp = map->find("diffuseFactor");
			if (diffuseProp != map->end())
			{
				tinygltf::Parameter &param = diffuseProp->second;
				if (param.number_array.size() == 4)
				{
					// Create a texture coord node for the texture sample
					diffuseFactor = NewObject<UMaterialExpressionVectorParameter>(UnrealMaterial);
					if (diffuseFactor)
					{
						UnrealMaterial->Expressions.Add(diffuseFactor);
						diffuseFactor->DefaultValue.R = param.number_array[0];
						diffuseFactor->DefaultValue.G = param.number_array[1];
						diffuseFactor->DefaultValue.B = param.number_array[2];
						diffuseFactor->DefaultValue.A = param.number_array[3];

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (diffuseTextureProp == map->end())
						{
							MaterialInput.Expression = diffuseFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}
					}
				}
			}
		}

		if (FString(MaterialProperty) == "metallicRoughnessTexture" && colorChannel == 2 /*This is hack for now, need to refactor*/)
		{
			pbrType = PBRTYPE_Metallic;

			const auto &metallicProp = map->find("metallicFactor");
			if (metallicProp != map->end())
			{
				tinygltf::Parameter &param = metallicProp->second;
				if (param.number_array.size() == 1)
				{
					// Create a texture coord node for the texture sample
					metallicFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
					if (metallicFactor)
					{
						UnrealMaterial->Expressions.Add(metallicFactor);
						metallicFactor->DefaultValue = param.number_array[0];

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (metallicRoghnessTextureProp == map->end())
						{
							MaterialInput.Expression = metallicFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}
					}
				}
			}
		}

		if (FString(MaterialProperty) == "metallicRoughnessTexture" && colorChannel == 1 /*This is hack for now, need to refactor*/)
		{
			pbrType = PBRTYPE_Roughness;

			const auto &roughnessProp = map->find("roughnessFactor");
			if (roughnessProp != map->end())
			{
				tinygltf::Parameter &param = roughnessProp->second;
				if (param.number_array.size() == 1)
				{
					// Create a texture coord node for the texture sample
					roughnessFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
					if (roughnessFactor)
					{
						UnrealMaterial->Expressions.Add(roughnessFactor);
						roughnessFactor->DefaultValue = param.number_array[0];

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (metallicRoghnessTextureProp == map->end())
						{
							MaterialInput.Expression = roughnessFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}

					}
				}
			}
		}

		if (FString(MaterialProperty) == "emissiveTexture")
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
						UnrealMaterial->Expressions.Add(emissiveFactor);
						emissiveFactor->DefaultValue.R = param.number_array[0];
						emissiveFactor->DefaultValue.G = param.number_array[1];
						emissiveFactor->DefaultValue.B = param.number_array[2];
						emissiveFactor->DefaultValue.A = 1.0;

						//If there is no baseColorTexture then we just use this color by itself and hook it up directly to the material
						if (emissiveTetxureProp == map->end())
						{
							MaterialInput.Expression = emissiveFactor;
							TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MaterialInput.Mask = Output->Mask;
							MaterialInput.MaskR = Output->MaskR;
							MaterialInput.MaskG = Output->MaskG;
							MaterialInput.MaskB = Output->MaskB;
							MaterialInput.MaskA = Output->MaskA;
							return true;
						}
					}
				}
			}
		}
	}

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

			int texCoordValue = 0;
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

						UTexture* UnrealTexture = ImportTexture(ImportContext, &img, bSetupAsNormalMap);

						if (UnrealTexture)
						{
							float ScaleU = 1.0;  //FbxTexture->GetScaleU();
							float ScaleV = 1.0;  //FbxTexture->GetScaleV();

							// and link it to the material 
							UnrealTextureExpression = NewObject<UMaterialExpressionTextureSample>(UnrealMaterial);
							UnrealMaterial->Expressions.Add(UnrealTextureExpression);
							MaterialInput.Expression = UnrealTextureExpression;
							UnrealTextureExpression->Texture = UnrealTexture;
							UnrealTextureExpression->SamplerType = bSetupAsNormalMap ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
							UnrealTextureExpression->MaterialExpressionEditorX = FMath::TruncToInt(Location.X);
							UnrealTextureExpression->MaterialExpressionEditorY = FMath::TruncToInt(Location.Y);


							// add/find UVSet and set it to the texture
							//FbxString UVSetName = FbxTexture->UVSet.Get();
							FString LocalUVSetName; // = UTF8_TO_TCHAR(UVSetName.Buffer());
							if (LocalUVSetName.IsEmpty())
							{
								LocalUVSetName = TEXT("UVmap_0");
							}
							//int32 SetIndex = UVSet.Find(LocalUVSetName);
							int32 SetIndex = 0;
							if ((SetIndex != 0 && SetIndex != INDEX_NONE) || ScaleU != 1.0f || ScaleV != 1.0f)
							{
								// Create a texture coord node for the texture sample
								UMaterialExpressionTextureCoordinate* MyCoordExpression = NewObject<UMaterialExpressionTextureCoordinate>(UnrealMaterial);
								UnrealMaterial->Expressions.Add(MyCoordExpression);
								MyCoordExpression->CoordinateIndex = (SetIndex >= 0) ? SetIndex : 0;
								MyCoordExpression->UTiling = ScaleU;
								MyCoordExpression->VTiling = ScaleV;
								UnrealTextureExpression->Coordinates.Expression = MyCoordExpression;
								MyCoordExpression->MaterialExpressionEditorX = FMath::TruncToInt(Location.X - 175);
								MyCoordExpression->MaterialExpressionEditorY = FMath::TruncToInt(Location.Y);
							}

							bCreated = true;
						}
					}
				}

				//normals have a scale
				if (FString(MaterialProperty) == "normalTexture")
				{
					const auto &textureScaleEntry = param.json_double_value.find("scale");
					if (textureScaleEntry != param.json_double_value.end())
					{
						UMaterialExpressionScalarParameter *scaleFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
						if (scaleFactor)
						{
							UnrealMaterial->Expressions.Add(scaleFactor);
							scaleFactor->DefaultValue = textureScaleEntry->second;

							UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
							UnrealMaterial->Expressions.Add(MultiplyExpression);
							MultiplyExpression->A.Expression = scaleFactor;
							MultiplyExpression->B.Expression = UnrealTextureExpression;
							MaterialInput.Expression = MultiplyExpression;
						}
					}
				}

				//occlusion has a strength. Why don't both of these just have normalFactor and occlusionFactor?
				if (FString(MaterialProperty) == "occlusionTexture")
				{
					const auto &textureStrengthEntry = param.json_double_value.find("strength");
					if (textureStrengthEntry != param.json_double_value.end())
					{
						UMaterialExpressionScalarParameter *strengthFactor = NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
						if (strengthFactor)
						{
							UnrealMaterial->Expressions.Add(strengthFactor);
							strengthFactor->DefaultValue = textureStrengthEntry->second;

							UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
							UnrealMaterial->Expressions.Add(MultiplyExpression);
							MultiplyExpression->A.Expression = strengthFactor;
							MultiplyExpression->B.Expression = UnrealTextureExpression;

							//Hook up the red channel of the texture map for ambient occlusion
							TArray<FExpressionOutput> Outputs = MultiplyExpression->B.Expression->GetOutputs();
							FExpressionOutput* Output = Outputs.GetData();
							MultiplyExpression->B.Mask = Output->Mask;
							MultiplyExpression->B.MaskR = Output->MaskR;

							MaterialInput.Expression = MultiplyExpression;

							colorChannel = -1; //Just use the default assignment.
						}
					}
				}


				if (pbrType == PBRTYPE_Color && baseColorFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = baseColorFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (pbrType == PBRTYPE_Roughness && roughnessFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = roughnessFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (pbrType == PBRTYPE_Metallic && metallicFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = metallicFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (pbrType == PBRTYPE_Emissive && emissiveFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = emissiveFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (pbrType == PBRTYPE_Diffuse && diffuseFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = diffuseFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (pbrType == PBRTYPE_Specular && specularFactor && UnrealTextureExpression)
				{
					UMaterialExpressionMultiply* MultiplyExpression = NewObject<UMaterialExpressionMultiply>(UnrealMaterial);
					UnrealMaterial->Expressions.Add(MultiplyExpression);
					MultiplyExpression->A.Expression = specularFactor;
					MultiplyExpression->B.Expression = UnrealTextureExpression;
					MaterialInput.Expression = MultiplyExpression;
				}

				if (MaterialInput.Expression)
				{
					TArray<FExpressionOutput> Outputs = MaterialInput.Expression->GetOutputs();
					FExpressionOutput* Output = Outputs.GetData();
					MaterialInput.Mask = Output->Mask;

					switch (colorChannel)
					{
					case 0:
						MaterialInput.MaskR = Output->MaskR;
						break;
					case 1:
						MaterialInput.MaskG = Output->MaskG;
						break;
					case 2:
						MaterialInput.MaskB = Output->MaskB;
						break;
					default:
						MaterialInput.MaskR = Output->MaskR;
						MaterialInput.MaskG = Output->MaskG;
						MaterialInput.MaskB = Output->MaskB;
						MaterialInput.MaskA = Output->MaskA;
						break;
					}
				}
			}
		}
	}

	return bCreated;
}


int32 UGLTFImporter::CreateNodeMaterials(FGltfImportContext &ImportContext, TArray<UMaterialInterface*>& OutMaterials)
{
	int32 MaterialCount = ImportContext.Model->materials.size();
	/*
	TArray<FbxSurfaceMaterial*> UsedSurfaceMaterials;
	FbxMesh *MeshNode = FbxNode->GetMesh();
	TSet<int32> UsedMaterialIndexes;
	if (MeshNode)
	{
		for (int32 ElementMaterialIndex = 0; ElementMaterialIndex < MeshNode->GetElementMaterialCount(); ++ElementMaterialIndex)
		{
			FbxGeometryElementMaterial *ElementMaterial = MeshNode->GetElementMaterial(ElementMaterialIndex);
			switch (ElementMaterial->GetMappingMode())
			{
			case FbxLayerElement::eAllSame:
			{
				if (ElementMaterial->GetIndexArray().GetCount() > 0)
				{
					UsedMaterialIndexes.Add(ElementMaterial->GetIndexArray()[0]);
				}
			}
			break;
			case FbxLayerElement::eByPolygon:
			{
				for (int32 MaterialIndex = 0; MaterialIndex < ElementMaterial->GetIndexArray().GetCount(); ++MaterialIndex)
				{
					UsedMaterialIndexes.Add(ElementMaterial->GetIndexArray()[MaterialIndex]);
				}
			}
			break;
			}
		}
	}
	*/
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		CreateUnrealMaterial(ImportContext, &ImportContext.Model->materials[MaterialIndex], OutMaterials);
		//Create only the material used by the mesh element material
		/*
		if (MeshNode == nullptr || UsedMaterialIndexes.Contains(MaterialIndex))
		{
			FbxSurfaceMaterial *FbxMaterial = FbxNode->GetMaterial(MaterialIndex);

			if (FbxMaterial)
			{
				CreateUnrealMaterial(*FbxMaterial, OutMaterials, UVSets, bForSkeletalMesh);
			}
		}
		else
		{
			OutMaterials.Add(nullptr);
		}
		*/
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
	ImportPathName.RemoveFromEnd(FString(TEXT("/"))+InName);

	ImportObjectFlags = RF_Public | RF_Standalone | RF_Transactional;

	TSubclassOf<UGLTFPrimResolver> ResolverClass = GetDefault<UGLTFImporterProjectSettings>()->CustomPrimResolver;
	if (!ResolverClass)
	{
		ResolverClass = UGLTFPrimResolver::StaticClass();
	}

	PrimResolver = NewObject<UGLTFPrimResolver>(GetTransientPackage(), ResolverClass);
	PrimResolver->Init();

	/*
	if(InStage->GetUpAxis() == EUsdUpAxis::ZAxis)
	{
		// A matrix that converts Z up right handed coordinate system to Z up left handed (unreal)
		ConversionTransform =
			FTransform(FMatrix
			(
				FPlane(1, 0, 0, 0),
				FPlane(0, -1, 0, 0),
				FPlane(0, 0, 1, 0),
				FPlane(0, 0, 0, 1)
			));
	}
	else
	*/
	{
		// A matrix that converts Y up right handed coordinate system to Z up left handed (unreal)
		ConversionTransform =
			FTransform(FMatrix
			(
				FPlane(1, 0, 0, 0),
				FPlane(0, 0, 1, 0),
				FPlane(0, -1, 0, 0),
				FPlane(0, 0, 0, 1)
			));
	}

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
	if(!bAutomated)
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

