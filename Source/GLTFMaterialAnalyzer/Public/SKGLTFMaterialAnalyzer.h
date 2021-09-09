// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Materials/MaterialInterface.h"
#include "SKGLTFMaterialAnalysis.h"
#include "SKGLTFMaterialAnalyzer.generated.h"

UCLASS(NotBlueprintType, Transient)
class SKGLTFMATERIALANALYZER_API USKGLTFMaterialAnalyzer : public UMaterialInterface
{
	GENERATED_BODY()

public:

	static void AnalyzeMaterialPropertyEx(const UMaterialInterface* InMaterial, const EMaterialProperty& InProperty, const FString& InCustomOutput, FGLTFMaterialAnalysis& OutAnalysis);

private:

	USKGLTFMaterialAnalyzer();

	void ResetToDefaults();

	UMaterialExpressionCustomOutput* GetCustomOutputExpression() const;

	virtual FMaterialResource* GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel) override;

	virtual int32 CompilePropertyEx(FMaterialCompiler* Compiler, const FGuid& AttributeID) override;

	virtual bool IsPropertyActive(EMaterialProperty InProperty) const override;

	EMaterialProperty Property;
	FString CustomOutput;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	UMaterialInterface* Material;

	FGLTFMaterialAnalysis* Analysis;
};
