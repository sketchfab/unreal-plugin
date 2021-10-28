// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonVariation.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"
#include "Variant.h"
#include "PropertyValue.h"
#include "LevelVariantSets.h"

class FGLTFVariationConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonVariationIndex, const ULevelVariantSets*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonVariationIndex Convert(const ULevelVariantSets* LevelVariantSets) override;

	bool TryParseVariant(FGLTFJsonVariant& OutVariant, const UVariant* Variant) const;
	bool TryParseVariantBinding(FGLTFJsonVariant& OutVariant, const UVariantObjectBinding* Binding) const;
	bool TryParseVisibilityPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const;
	bool TryParseMaterialPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const;
	bool TryParseStaticMeshPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const;
	bool TryParseSkeletalMeshPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const;

	template<typename T>
	bool TryGetPropertyValue(UPropertyValue* Property, T& OutValue) const;

	FString GetLogContext(const UPropertyValue* Property) const;
	FString GetLogContext(const UVariantObjectBinding* Binding) const;
	FString GetLogContext(const UVariant* Variant) const;
	FString GetLogContext(const UVariantSet* VariantSet) const;
	FString GetLogContext(const ULevelVariantSets* LevelVariantSets) const;
};
