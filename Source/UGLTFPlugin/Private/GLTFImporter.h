// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "GLTFImportOptions.h"
#include "TokenizedMessage.h"
#include "GLTFPrimResolver.h"

THIRD_PARTY_INCLUDES_START
#include "tiny_gltf.h"
THIRD_PARTY_INCLUDES_END

#include "GLTFImporter.generated.h"


class UGLTFPrimResolver;
class IGltfPrim;
struct FRawMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogGLTFImport, Log, All);

USTRUCT()
struct FGltfImportContext
{
	GENERATED_BODY()
	
	/** Mapping of path to imported assets  */
	TMap<FString, UObject*> PathToImportAssetMap;

	/** Parent package to import a single mesh to */
	UPROPERTY()
	UObject* Parent;

	/** Name to use when importing a single mesh */
	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FString ImportPathName;

	UPROPERTY()
	UGLTFImportOptions* ImportOptions;

	UPROPERTY()
	UGLTFPrimResolver* PrimResolver;

	FString BasePath;
	TArray<UMaterialInterface*> Materials;

	tinygltf::Model* Model;

	TMap<int, int> MaterialMap;

	/** Converts from a source coordinate system to Unreal */
	FTransform ConversionTransform;

	/** Object flags to apply to newly imported objects */
	EObjectFlags ImportObjectFlags;

	/** Whether or not to apply world transformations to the actual geometry */
	bool bApplyWorldTransformToGeometry;

	/** If true stop at any USD prim that has an unreal asset reference.  Geometry that is a child such prims will be ignored */
	bool bFindUnrealAssetReferences;

	/** Whether to automatically create Unreal materials for materials found in the glTF scene */
	bool bImportMaterials;

	virtual ~FGltfImportContext() { }

	virtual void Init(UObject* InParent, const FString& InName, const FString &InBasePath, tinygltf::Model* InModel);

	void AddErrorMessage(EMessageSeverity::Type MessageSeverity, FText ErrorMessage);
	void DisplayErrorMessages(bool bAutomated);
	void ClearErrorMessages();
private:
	/** Error messages **/
	TArray<TSharedRef<FTokenizedMessage>> TokenizedErrorMessages;
};

enum TextureType
{
	TextureType_PBR,
	TextureType_SPEC,
	TextureType_DEFAULT,
};

UCLASS(transient)
class UGLTFImporter : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	bool ShowImportOptions(UObject& ImportOptions);

	tinygltf::Model* ReadGLTFFile(FGltfImportContext& ImportContext, const FString& Filename);

	UObject* ImportMeshes(FGltfImportContext& ImportContext, const TArray<FGltfPrimToImport>& PrimsToImport);
	UStaticMesh* ImportSingleMesh(FGltfImportContext& ImportContext, EGltfMeshImportType ImportType, const FGltfPrimToImport& PrimToImport, FRawMesh &RawTriangles, UStaticMesh *singleMesh = nullptr);

	UTexture* ImportTexture(FGltfImportContext& ImportContext, tinygltf::Image *img, bool bSetupAsNormalMap);
	void ImportTexturesFromNode(FGltfImportContext& ImportContext, tinygltf::Model* Model, tinygltf::Node* Node);

	void CreateUnrealMaterial(FGltfImportContext& ImportContext, tinygltf::Material *mat, TArray<UMaterialInterface*>& OutMaterials);

	bool CreateAndLinkExpressionForMaterialProperty(FGltfImportContext& ImportContext, tinygltf::Material *mat, UMaterial* UnrealMaterial, const char* MaterialProperty, TextureType texType,
		FExpressionInput& MaterialInput, bool bSetupAsNormalMap, const FVector2D& Location, int32 colorChannel=-1);

	int32 CreateNodeMaterials(FGltfImportContext &ImportContext, TArray<UMaterialInterface*>& OutMaterials);

};