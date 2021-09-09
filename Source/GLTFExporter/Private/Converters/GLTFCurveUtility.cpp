// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFCurveUtility.h"
#include "Curves/CurveLinearColor.h"

bool FGLTFCurveUtility::HasAnyAdjustment(const UCurveLinearColor& ColorCurve)
{
	const float ErrorTolerance = KINDA_SMALL_NUMBER;

	return !FMath::IsNearlyEqual(ColorCurve.AdjustBrightness, 1.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustHue, 0.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustSaturation, 1.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustVibrance, 0.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustBrightnessCurve, 1.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustMaxAlpha, 1.0f, ErrorTolerance) ||
		!FMath::IsNearlyEqual(ColorCurve.AdjustMinAlpha, 0.0f, ErrorTolerance);
}
