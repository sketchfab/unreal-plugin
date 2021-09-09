// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFLevelExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFLevelExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFLevelExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
