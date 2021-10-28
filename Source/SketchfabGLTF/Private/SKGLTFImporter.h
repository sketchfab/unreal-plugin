// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "SKGLTFImportOptions_SKETCHFAB.h"
#include "Logging/TokenizedMessage.h"
#include "SKGLTFPrimResolver.h"
#include "Materials/MaterialExpressionTextureSample.h"

THIRD_PARTY_INCLUDES_START
#include "tiny_gltf.h"
THIRD_PARTY_INCLUDES_END

#include "SKGLTFImporter.generated.h"


class USKGLTFPrimResolver;
struct FRawMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogGLTFImport, Log, All);

USTRUCT()
struct FGLTFImportContext
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
	USKGLTFImportOptions_SKETCHFAB* ImportOptions;

	UPROPERTY()
	USKGLTFPrimResolver* PrimResolver;

	FString BasePath;
	TArray<UMaterialInterface*> Materials;

	tinygltf::Model* Model;

	TMap<int, int> MaterialMap;

	/** Converts from a source coordinate system to Unreal */
	FTransform ConversionTransform;

	/** Object flags to apply to newly imported objects */
	EObjectFlags ImportObjectFlags;

	/** Whether or not to apply world transformations to the actual geometry */
	bool bApplyWorldTransform;

	/** Whether to automatically create Unreal materials for materials found in the glTF scene */
	bool bImportMaterials;
	bool bImportInNewFolder;

	bool disableTangents;
	bool disableNormals;

	virtual ~FGLTFImportContext() { }

	virtual void Init(UObject* InParent, const FString& InName, const FString &InBasePath, tinygltf::Model* InModel);

	void AddErrorMessage(EMessageSeverity::Type MessageSeverity, FText ErrorMessage);
	void DisplayErrorMessages(bool bAutomated);
	void ClearErrorMessages();

	FString GetImportPath(FString DirectoryName="");
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

enum ColorChannel
{
	ColorChannel_Red,
	ColorChannel_Green,
	ColorChannel_Blue,
	ColorChannel_Alpha,
	ColorChannel_All,
};

struct SharedTexture
{
	UMaterialExpressionTextureSample* expression;
	int32 texCoords;

};

typedef TMap<int32, SharedTexture> SharedTextureMap;

UCLASS(transient)
class USKGLTFImporter : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	bool ShowImportOptions(UObject& ImportOptions);

	tinygltf::Model* ReadGLTFFile(FGLTFImportContext& ImportContext, const FString& Filename);

	UObject* ImportMeshes(FGLTFImportContext& ImportContext, const TArray<FGLTFPrimToImport>& PrimsToImport);

	UTexture* ImportTexture(FGLTFImportContext& ImportContext, tinygltf::Image *img, EMaterialSamplerType samplerType, const char *MaterialProperty = nullptr);

	void CreateUnrealMaterial(FGLTFImportContext& ImportContext, tinygltf::Material *mat, TArray<UMaterialInterface*>& OutMaterials);
	static void UseVertexColors(UMaterial* UnrealMaterial);
	static void CreateMultiplyExpression(UMaterial* UnrealMaterial, FExpressionInput& MaterialInput, UMaterialExpression* ExpressionFactor, UMaterialExpression* UnrealTextureExpression, ColorChannel colorChannel);
	bool CreateAndLinkExpressionForMaterialProperty(FScopedSlowTask &materialProgress, FGLTFImportContext& ImportContext, tinygltf::Material *mat, UMaterial* UnrealMaterial, SharedTextureMap &texMap, const char* MaterialProperty, TextureType texType,
		FExpressionInput& MaterialInput, EMaterialSamplerType samplerType, FVector2D& Location, ColorChannel colorChannel = ColorChannel_All);

	int32 CreateNodeMaterials(FGLTFImportContext &ImportContext, TArray<UMaterialInterface*>& OutMaterials);
	void AttachOutputs(FExpressionInput& MaterialInput, ColorChannel colorChannel);

};