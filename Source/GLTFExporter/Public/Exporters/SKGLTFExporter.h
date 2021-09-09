// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "SKGLTFExporter.generated.h"

class FGLTFContainerBuilder;
class USKGLTFExportOptions;

UCLASS(Abstract)
class SKGLTFEXPORTER_API USKGLTFExporter : public UExporter
{
public:

	GENERATED_BODY()

	explicit USKGLTFExporter(const FObjectInitializer& ObjectInitializer);

	virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Archive, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags) override;

protected:

	virtual bool AddObject(FGLTFContainerBuilder& Builder, const UObject* Object);

public:

	USKGLTFExportOptions* GetExportOptions();
};
