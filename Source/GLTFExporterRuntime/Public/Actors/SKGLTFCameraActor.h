// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "SKGLTFCameraActor.generated.h"

UENUM(BlueprintType)
enum class ESKGLTFCameraControlMode : uint8
{
	FreeLook,
	Orbital
};

/**
 * GLTF-compatible camera that will carry over settings and simulate the behavior in the resulting viewer.
 * Supports two modes:
 * - FreeLook: Allows the user to rotate the camera without modifying its position.
 * - Orbital: Focuses one actor in the scene and orbits it through mouse control.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "GLTF Camera")
class SKGLTFEXPORTERRUNTIME_API ASKGLTFCameraActor : public ACameraActor
{
	GENERATED_BODY()
	//~ Begin UObject Interface
public:
	ASKGLTFCameraActor(const FObjectInitializer& ObjectInitializer);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End UObject Interface

	//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void PreInitializeComponents() override;
	virtual void PostActorCreated() override;
	//~ End AActor Interface

private:
	UFUNCTION()
	void OnMouseX(float AxisValue);

	UFUNCTION()
	void OnMouseY(float AxisValue);

	UFUNCTION()
	void OnMouseWheelAxis(float AxisValue);

	float ClampDistance(float Value) const;

	float ClampPitch(float Value) const;

	float ClampYaw(float Value) const;

	void RemoveInertia();

	FRotator GetLookAtRotation(const FVector TargetPosition) const;

	FVector GetFocusPosition() const;

	bool SetAutoActivateForPlayer(const EAutoReceiveInput::Type Player);

public:

	/* Camera mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	ESKGLTFCameraControlMode Mode;

	/* Actor which the camera will focus on and subsequently orbit when using Orbital mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor", meta=(EditCondition="Mode == ESKGLTFCameraControlMode::Orbital"))
	AActor* Target;

	/* Minimum pitch angle (in degrees) for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float PitchAngleMin;

	/* Maximum pitch angle (in degrees) for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float PitchAngleMax;

	/* Minimum yaw angle (in degrees) for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float YawAngleMin;

	/* Maximum yaw angle (in degrees) for the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float YawAngleMax;

	/* Closest distance the camera can approach the focused actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float DistanceMin;

	/* Farthest distance the camera can recede from the focused actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GLTF Camera Actor")
	float DistanceMax;

	/* Duration (in seconds) that it takes the camera to complete a change in distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GLTF Camera Actor")
	float DollyDuration;

	/* Size of the dolly movement relative to user input. The higher the value, the faster it moves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GLTF Camera Actor")
	float DollySensitivity;

	/* Deceleration that occurs after rotational movement. The higher the value, the longer it takes to settle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GLTF Camera Actor")
	float RotationInertia;

	/* Size of the rotational movement relative to user input. The higher the value, the faster it moves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GLTF Camera Actor")
	float RotationSensitivity;

private:
	float Distance;
	float Pitch;
	float Yaw;
	float TargetDistance;
	float TargetPitch;
	float TargetYaw;
	float DollyTime;
	float DollyStartDistance;

public:

	bool UsesYawLimits() const;
};
