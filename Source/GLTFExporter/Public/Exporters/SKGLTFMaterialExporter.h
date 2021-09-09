// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFMaterialExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFMaterialExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFMaterialExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
