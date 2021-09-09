// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFBackdropConverters.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFMaterialUtility.h"
#include "Converters/SKGLTFActorUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"

FGLTFJsonBackdropIndex FGLTFBackdropConverter::Convert(const AActor* BackdropActor)
{
	const UBlueprint* Blueprint = FGLTFActorUtility::GetBlueprintFromActor(BackdropActor);
	if (!FGLTFActorUtility::IsHDRIBackdropBlueprint(Blueprint))
	{
		return FGLTFJsonBackdropIndex(INDEX_NONE);
	}

	FGLTFJsonBackdrop JsonBackdrop;
	BackdropActor->GetName(JsonBackdrop.Name);

	const USceneComponent* SceneComponent = BackdropActor->GetRootComponent();
	const FRotator Rotation = SceneComponent->GetComponentRotation();

	// NOTE: when calculating map rotation in the HDRI_Attributes material function using RotateAboutAxis,
	// an angle in radians is used as input. But RotateAboutAxis expects a value of 1 per full revolution, not 2*PI.
	// The end result is that the map rotation is always 2*PI more than the actual yaw of the scene component.

	// TODO: remove the 2*PI multiplier if HDRI_Attributes is updated to match map rotation with yaw
	JsonBackdrop.Angle = FRotator::ClampAxis(Rotation.Yaw * 2.0f * PI);

	const UStaticMesh* Mesh;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("Mesh"), Mesh))
	{
		JsonBackdrop.Mesh = Builder.GetOrAddMesh(Mesh, { FGLTFMaterialUtility::GetDefault() });
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export Mesh for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	const UTextureCube* Cubemap;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("Cubemap"), Cubemap))
	{
		// TODO: add a proper custom gltf extension with its own converters for cubemaps

		for (int32 CubeFaceIndex = 0; CubeFaceIndex < CubeFace_MAX; ++CubeFaceIndex)
		{
			const ECubeFace CubeFace = static_cast<ECubeFace>(CubeFaceIndex);
			const ESKGLTFJsonCubeFace JsonCubeFace = FGLTFConverterUtility::ConvertCubeFace(CubeFace);
			JsonBackdrop.Cubemap[static_cast<int32>(JsonCubeFace)] = Builder.GetOrAddTexture(Cubemap, CubeFace);
		}
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export Cubemap for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	float Intensity;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("Intensity"), Intensity))
	{
		JsonBackdrop.Intensity = Intensity;
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export Intensity for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	float Size;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("Size"), Size))
	{
		JsonBackdrop.Size = Size;
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export Size for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	FVector ProjectionCenter;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("ProjectionCenter"), ProjectionCenter))
	{
		JsonBackdrop.ProjectionCenter = FGLTFConverterUtility::ConvertPosition(ProjectionCenter, Builder.ExportOptions->ExportUniformScale);
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export ProjectionCenter for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	float LightingDistanceFactor;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("LightingDistanceFactor"), LightingDistanceFactor))
	{
		JsonBackdrop.LightingDistanceFactor = LightingDistanceFactor;
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export LightingDistanceFactor for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	bool UseCameraProjection;
	if (FGLTFActorUtility::TryGetPropertyValue(BackdropActor, TEXT("UseCameraProjection"), UseCameraProjection))
	{
		JsonBackdrop.UseCameraProjection = UseCameraProjection;
	}
	else
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export UseCameraProjection for HDRIBackdrop %s"), *BackdropActor->GetName()));
	}

	return Builder.AddBackdrop(JsonBackdrop);
}
