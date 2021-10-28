// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFSkySphereConverters.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFMaterialUtility.h"
#include "Converters/SKGLTFActorUtility.h"
#include "Converters/SKGLTFCurveUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"

FGLTFJsonSkySphereIndex FGLTFSkySphereConverter::Convert(const AActor* SkySphereActor)
{
	const UBlueprint* Blueprint = FGLTFActorUtility::GetBlueprintFromActor(SkySphereActor);
	if (!FGLTFActorUtility::IsSkySphereBlueprint(Blueprint))
	{
		return FGLTFJsonSkySphereIndex(INDEX_NONE);
	}

	FGLTFJsonSkySphere JsonSkySphere;
	SkySphereActor->GetName(JsonSkySphere.Name);

	const UStaticMeshComponent* StaticMeshComponent = nullptr;
	FGLTFActorUtility::TryGetPropertyValue(SkySphereActor, TEXT("SkySphereMesh"), StaticMeshComponent);

	if (StaticMeshComponent != nullptr)
	{
		const USceneComponent* ParentComponent = StaticMeshComponent->GetAttachParent();
		const FName SocketName = StaticMeshComponent->GetAttachSocketName();

		const FTransform Transform = StaticMeshComponent->GetComponentTransform();
		const FTransform ParentTransform = ParentComponent->GetSocketTransform(SocketName);
		const FTransform RelativeTransform = Transform.GetRelativeTransform(ParentTransform);

		JsonSkySphere.Scale = FGLTFConverterUtility::ConvertScale(RelativeTransform.GetScale3D());
		JsonSkySphere.SkySphereMesh = Builder.GetOrAddMesh(StaticMeshComponent, { FGLTFMaterialUtility::GetDefault() });
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export property SkySphereMesh for Sky Sphere %s"), *SkySphereActor->GetName()));
	}

	const ADirectionalLight* DirectionalLight = nullptr;
	if (FGLTFActorUtility::TryGetPropertyValue(SkySphereActor, TEXT("Directional light actor"), DirectionalLight))
	{
		JsonSkySphere.DirectionalLight = Builder.GetOrAddNode(DirectionalLight);
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export property Directional light actor for Sky Sphere %s"), *SkySphereActor->GetName()));
	}

	const UMaterialInstance* SkyMaterial = nullptr;
	FGLTFActorUtility::TryGetPropertyValue(SkySphereActor, TEXT("Sky material"), SkyMaterial);

	if (SkyMaterial != nullptr)
	{
		ConvertScalarParameter(SkySphereActor, SkyMaterial, TEXT("Sun Radius"), JsonSkySphere.SunRadius);
		ConvertScalarParameter(SkySphereActor, SkyMaterial, TEXT("NoisePower1"), JsonSkySphere.NoisePower1);
		ConvertScalarParameter(SkySphereActor, SkyMaterial, TEXT("NoisePower2"), JsonSkySphere.NoisePower2);

		ConvertTextureParameter(SkySphereActor, SkyMaterial, ESkySphereTextureParameter::SkyTexture, JsonSkySphere.SkyTexture);
		ConvertTextureParameter(SkySphereActor, SkyMaterial, ESkySphereTextureParameter::CloudsTexture, JsonSkySphere.CloudsTexture);
		ConvertTextureParameter(SkySphereActor, SkyMaterial, ESkySphereTextureParameter::StarsTexture, JsonSkySphere.StarsTexture);
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export property Sky material for Sky Sphere %s"),
			*SkySphereActor->GetName()));
	}

	ConvertProperty(SkySphereActor, TEXT("Sun height"), JsonSkySphere.SunHeight);
	ConvertProperty(SkySphereActor, TEXT("Sun brightness"), JsonSkySphere.SunBrightness);
	ConvertProperty(SkySphereActor, TEXT("Stars brightness"), JsonSkySphere.StarsBrightness);
	ConvertProperty(SkySphereActor, TEXT("Cloud speed"), JsonSkySphere.CloudSpeed);
	ConvertProperty(SkySphereActor, TEXT("Cloud opacity"), JsonSkySphere.CloudOpacity);
	ConvertProperty(SkySphereActor, TEXT("Horizon Falloff"), JsonSkySphere.HorizonFalloff);
	ConvertProperty(SkySphereActor, TEXT("Colors determined by sun position"), JsonSkySphere.bColorsDeterminedBySunPosition);

	ConvertColorProperty(SkySphereActor, TEXT("Zenith Color"), JsonSkySphere.ZenithColor);
	ConvertColorProperty(SkySphereActor, TEXT("Horizon color"), JsonSkySphere.HorizonColor);
	ConvertColorProperty(SkySphereActor, TEXT("Cloud color"), JsonSkySphere.CloudColor);
	ConvertColorProperty(SkySphereActor, TEXT("Overall Color"), JsonSkySphere.OverallColor);

	ConvertColorCurveProperty(SkySphereActor, TEXT("Zenith color curve"), JsonSkySphere.ZenithColorCurve);
	ConvertColorCurveProperty(SkySphereActor, TEXT("Horizon color curve"), JsonSkySphere.HorizonColorCurve);
	ConvertColorCurveProperty(SkySphereActor, TEXT("Cloud color curve"), JsonSkySphere.CloudColorCurve);

	return Builder.AddSkySphere(JsonSkySphere);
}

template <class ValueType>
void FGLTFSkySphereConverter::ConvertProperty(const AActor* Actor, const TCHAR* PropertyName, ValueType& OutValue) const
{
	check(Actor != nullptr);

	if (!FGLTFActorUtility::TryGetPropertyValue(Actor, PropertyName, OutValue))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export property %s for Sky Sphere %s"),
			PropertyName,
			*Actor->GetName()));
	}
}

void FGLTFSkySphereConverter::ConvertColorProperty(const AActor* Actor, const TCHAR* PropertyName, FGLTFJsonColor4& OutValue) const
{
	check(Actor != nullptr);

	FLinearColor LinearColor;
	if (FGLTFActorUtility::TryGetPropertyValue(Actor, PropertyName, LinearColor))
	{
		OutValue = FGLTFConverterUtility::ConvertColor(LinearColor);
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export property %s for Sky Sphere %s"),
			PropertyName,
			*Actor->GetName()));
	}
}

void FGLTFSkySphereConverter::ConvertColorCurveProperty(const AActor* Actor, const TCHAR* PropertyName, FGLTFJsonSkySphereColorCurve& OutValue) const
{
	check(Actor != nullptr);

	const UCurveLinearColor* ColorCurve = nullptr;
	FGLTFActorUtility::TryGetPropertyValue(Actor, PropertyName, ColorCurve);

	if (ColorCurve != nullptr)
	{
		if (FGLTFCurveUtility::HasAnyAdjustment(*ColorCurve))
		{
			Builder.AddWarningMessage(FString::Printf(
				TEXT("Adjustments for property %s in Sky Sphere %s are not supported"),
				PropertyName,
				*Actor->GetName()));
		}

		OutValue.ComponentCurves.AddDefaulted(3);

		for (uint32 ComponentIndex = 0; ComponentIndex < 3; ++ComponentIndex)
		{
			const FRichCurve& FloatCurve = ColorCurve->FloatCurves[ComponentIndex];
			const uint32 KeyCount = FloatCurve.Keys.Num();

			TArray<FGLTFJsonSkySphereColorCurve::FKey>& ComponentKeys = OutValue.ComponentCurves[ComponentIndex].Keys;
			ComponentKeys.AddUninitialized(KeyCount);

			uint32 KeyIndex = 0;

			for (const FRichCurveKey& Key: FloatCurve.Keys)
			{
				ComponentKeys[KeyIndex++] = { Key.Time, Key.Value };
			}
		}
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export property %s for Sky Sphere %s"),
			PropertyName,
			*Actor->GetName()));
	}
}

void FGLTFSkySphereConverter::ConvertScalarParameter(const AActor* Actor, const UMaterialInstance* Material, const TCHAR* ParameterName, float& OutValue) const
{
	check(Actor != nullptr && Material != nullptr);

	if (!Material->GetScalarParameterValue(ParameterName, OutValue))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export parameter %s (in material %s) for Sky Sphere %s"),
			ParameterName,
			*Material->GetName(),
			*Actor->GetName()));
	}
}

void FGLTFSkySphereConverter::ConvertTextureParameter(const AActor* Actor, const UMaterialInstance* Material, const ESkySphereTextureParameter Parameter, FGLTFJsonTextureIndex& OutValue) const
{
	check(Actor != nullptr && Material != nullptr);

	const TCHAR* TexturePath = nullptr;

	// TODO: the sky sphere material doesn't expose the relevant textures as parameters, so we use hard-coded asset paths.
	// We should try to find a way of extracting and returning the correct texture from the material anyway.
	switch (Parameter)
	{
		case ESkySphereTextureParameter::SkyTexture:    TexturePath = TEXT("/Engine/EngineSky/T_Sky_Blue.T_Sky_Blue"); break;
		case ESkySphereTextureParameter::CloudsTexture: TexturePath = TEXT("/Engine/EngineSky/T_Sky_Clouds_M.T_Sky_Clouds_M"); break;
		case ESkySphereTextureParameter::StarsTexture:  TexturePath = TEXT("/Engine/EngineSky/T_Sky_Stars.T_Sky_Stars"); break;
		default:                                        checkNoEntry();
	}

	const UTexture2D* Texture2D = TexturePath != nullptr ? LoadObject<UTexture2D>(nullptr, TexturePath) : nullptr;
	if (Texture2D != nullptr)
	{
		OutValue = Builder.GetOrAddTexture(Texture2D);
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to export texture %s (used in material %s) for Sky Sphere %s"),
			TexturePath,
			*Material->GetName(),
			*Actor->GetName()));
	}
}
