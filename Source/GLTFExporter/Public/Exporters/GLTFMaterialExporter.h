// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Exporters/GLTFExporter.h"
#include "GLTFMaterialExporter.generated.h"

UCLASS()
class GLTFEXPORTER_API UGLTFMaterialExporter final : public UGLTFExporter
{
public:

	GENERATED_BODY()

	explicit UGLTFMaterialExporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer());

	bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object) override;
};
