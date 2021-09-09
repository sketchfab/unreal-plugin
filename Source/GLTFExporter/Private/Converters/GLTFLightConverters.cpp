// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFLightConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFNameUtility.h"

FGLTFJsonLightIndex FGLTFLightConverter::Convert(const ULightComponent* LightComponent)
{
	FGLTFJsonLight Light;

	Light.Name = FGLTFNameUtility::GetName(LightComponent);
	Light.Type = FGLTFConverterUtility::ConvertLightType(LightComponent->GetLightType());

	if (Light.Type == ESKGLTFJsonLightType::None)
	{
		// TODO: report error (unsupported light component type)
		return FGLTFJsonLightIndex(INDEX_NONE);
	}

	Light.Intensity = LightComponent->Intensity;

	const FLinearColor TemperatureColor = LightComponent->bUseTemperature ? FLinearColor::MakeFromColorTemperature(LightComponent->Temperature) : FLinearColor::White;
	Light.Color = FGLTFConverterUtility::ConvertColor(TemperatureColor * LightComponent->GetLightColor());

	if (const UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(LightComponent))
	{
		Light.Range = FGLTFConverterUtility::ConvertLength(PointLightComponent->AttenuationRadius, Builder.ExportOptions->ExportUniformScale);
	}

	if (const USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(LightComponent))
	{
		Light.Spot.InnerConeAngle = FGLTFConverterUtility::ConvertLightAngle(SpotLightComponent->InnerConeAngle);
		Light.Spot.OuterConeAngle = FGLTFConverterUtility::ConvertLightAngle(SpotLightComponent->OuterConeAngle);
	}

	return Builder.AddLight(Light);
}
