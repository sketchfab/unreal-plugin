// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/GLTFExporter.h"
#include "GLTFLevelVariantSetsExporter.generated.h"

UCLASS()
class GLTFEXPORTER_API UGLTFLevelVariantSetsExporter final : public UGLTFExporter
{
public:

	GENERATED_BODY()

	explicit UGLTFLevelVariantSetsExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
