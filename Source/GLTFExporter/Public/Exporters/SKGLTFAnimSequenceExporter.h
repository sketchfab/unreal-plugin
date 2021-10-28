// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFAnimSequenceExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFAnimSequenceExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFAnimSequenceExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
