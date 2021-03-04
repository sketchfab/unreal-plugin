// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/GLTFExporter.h"
#include "GLTFSkeletalMeshExporter.generated.h"

UCLASS()
class GLTFEXPORTER_API UGLTFSkeletalMeshExporter final : public UGLTFExporter
{
public:

	GENERATED_BODY()

	explicit UGLTFSkeletalMeshExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
