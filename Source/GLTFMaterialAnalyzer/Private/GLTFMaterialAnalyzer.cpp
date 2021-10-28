// Copyright Epic Games, Inc. All Rights Reserved.

#include "SKGLTFMaterialAnalyzer.h"
#include "SKGLTFProxyMaterialCompiler.h"
#include "Materials/HLSLMaterialTranslator.h"

USKGLTFMaterialAnalyzer::USKGLTFMaterialAnalyzer()
{
	ResetToDefaults();
}

void USKGLTFMaterialAnalyzer::ResetToDefaults()
{
	Property = MP_MAX;
	CustomOutput = {};
	Material = nullptr;
	Analysis = nullptr;
}

void USKGLTFMaterialAnalyzer::AnalyzeMaterialPropertyEx(const UMaterialInterface* InMaterial, const EMaterialProperty& InProperty, const FString& InCustomOutput, FGLTFMaterialAnalysis& OutAnalysis)
{
	// NOTE: besides avoiding creating a new object each time a property is analyzed,
	// the use of GetDefaultObject() also server another purpose:
	//
	// When a property in an object that may be referenced by materials is modified,
	// the editor will evaluate which materials need updating.
	// USKGLTFMaterialAnalyzer has no implementations for the pure virtual functions
	// defined in UMaterialInterface, and the editor would crash if it was processed.
	// Fortunately objects with the RF_ClassDefaultObject flag
	// are excluded from being processed.

	USKGLTFMaterialAnalyzer* Analyzer = Cast<USKGLTFMaterialAnalyzer>(StaticClass()->GetDefaultObject());

	Analyzer->Property = InProperty;
	Analyzer->CustomOutput = InCustomOutput;
	Analyzer->Material = const_cast<UMaterialInterface*>(InMaterial);
	Analyzer->Analysis = &OutAnalysis;

	// NOTE: When analyzing custom outputs, the property *must* be set to MP_MAX or the compiler will refuse to compile the output
	const EMaterialProperty SafeProperty = InProperty == MP_CustomOutput ? MP_MAX : InProperty;

	int32 DummyNumTextureCoordinates; // Dummy value from built-in analysis not used since it's insufficient
	bool DummyRequiresVertexData; // Dummy value from built-in analysis not used since it's insufficient
	Analyzer->AnalyzeMaterialProperty(SafeProperty, DummyNumTextureCoordinates, DummyRequiresVertexData);

	Analyzer->ResetToDefaults();
}

UMaterialExpressionCustomOutput* USKGLTFMaterialAnalyzer::GetCustomOutputExpression() const
{
	for (UMaterialExpression* Expression : Material->GetMaterial()->Expressions)
	{
		UMaterialExpressionCustomOutput* CustomOutputExpression = Cast<UMaterialExpressionCustomOutput>(Expression);
		if (CustomOutputExpression != nullptr && CustomOutputExpression->GetDisplayName() == CustomOutput)
		{
			return CustomOutputExpression;
		}
	}

	return nullptr;
}

FMaterialResource* USKGLTFMaterialAnalyzer::GetMaterialResource(ERHIFeatureLevel::Type InFeatureLevel, EMaterialQualityLevel::Type QualityLevel)
{
	return Material != nullptr ? Material->GetMaterialResource(InFeatureLevel, QualityLevel) : nullptr;
}

int32 USKGLTFMaterialAnalyzer::CompilePropertyEx(FMaterialCompiler* Compiler, const FGuid& AttributeID)
{
	if (Material == nullptr)
	{
		return INDEX_NONE;
	}

	FGLTFProxyMaterialCompiler ProxyCompiler(Compiler);
	int32 Result;

	if (Property == MP_CustomOutput)
	{
		UMaterialExpressionCustomOutput* CustomOutputExpression = GetCustomOutputExpression();
		if (CustomOutputExpression == nullptr)
		{
			return INDEX_NONE;
		}

		Result = CustomOutputExpression->Compile(&ProxyCompiler, 0);
	}
	else
	{
		Result = Material->CompilePropertyEx(&ProxyCompiler, AttributeID);
	}

	class FHLSLMaterialTranslatorHack : public FHLSLMaterialTranslator
	{
		using FHLSLMaterialTranslator::FHLSLMaterialTranslator;

		friend USKGLTFMaterialAnalyzer;
	};

	FHLSLMaterialTranslatorHack* TranslatorHack = static_cast<FHLSLMaterialTranslatorHack*>(Compiler);

	Analysis->TextureCoordinates = TranslatorHack->AllocatedUserTexCoords;
	Analysis->ShadingModels = TranslatorHack->ShadingModelsFromCompilation;

	if (Result != INDEX_NONE)
	{
		Analysis->ParameterCode = TranslatorHack->GetParameterCode(Result);
	}

	// TODO: investigate if we need to check more conditions to determine that vertex data is required
	Analysis->bRequiresVertexData =
		TranslatorHack->bUsesVertexColor ||
		TranslatorHack->bUsesTransformVector ||
		TranslatorHack->bNeedsWorldPositionExcludingShaderOffsets ||
		TranslatorHack->bUsesAOMaterialMask ||
		TranslatorHack->bUsesVertexPosition;

	return Result;
}

bool USKGLTFMaterialAnalyzer::IsPropertyActive(EMaterialProperty InProperty) const
{
	return Material != nullptr && Material->IsPropertyActive(InProperty);
}
