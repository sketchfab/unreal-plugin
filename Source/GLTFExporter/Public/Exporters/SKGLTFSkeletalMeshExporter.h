// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/SKGLTFExporter.h"
#include "SKGLTFSkeletalMeshExporter.generated.h"

UCLASS()
class SKGLTFEXPORTER_API USKGLTFSkeletalMeshExporter final : public USKGLTFExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFSkeletalMeshExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
