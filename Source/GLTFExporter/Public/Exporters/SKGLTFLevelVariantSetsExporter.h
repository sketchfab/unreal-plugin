// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFLevelVariantSetsExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFLevelVariantSetsExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFLevelVariantSetsExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
