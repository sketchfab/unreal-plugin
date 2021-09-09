// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFStaticMeshExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFStaticMeshExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFStaticMeshExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
