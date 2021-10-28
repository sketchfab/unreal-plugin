// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

class UCurveLinearColor;

struct FGLTFCurveUtility
{
	static bool HasAnyAdjustment(const UCurveLinearColor& ColorCurve);
};
