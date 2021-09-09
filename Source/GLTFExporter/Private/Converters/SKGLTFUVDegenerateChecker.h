// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFIndexArray.h"
#include "Engine.h"

class FGLTFUVDegenerateChecker final : public TGLTFConverter<float, const FMeshDescription*, FGLTFIndexArray, int32>
{
	virtual void Sanitize(const FMeshDescription*& Description, FGLTFIndexArray& SectionIndices, int32& TexCoord) override;

	virtual float Convert(const FMeshDescription* Description, FGLTFIndexArray SectionIndices, int32 TexCoord) override;

	static bool IsDegenerateTriangle(const TStaticArray<FVector2D, 3>& Points);
	static bool IsDegenerateTriangle(const TStaticArray<FVector, 3>& Points);
};
