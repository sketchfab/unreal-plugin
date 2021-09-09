// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFLevelSequenceExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFLevelSequenceExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFLevelSequenceExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
