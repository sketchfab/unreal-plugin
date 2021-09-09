// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/SKGLTFCameraActor.h"
#include "Components/InputComponent.h"

namespace
{
	// Scales to convert from the export-friendly sensitivity-values stored in our properties
	// to values that we can use when processing axis-values (to get similar results as in the viewer).
	const float RotationSensitivityScale = 16.667f;
	const float DollySensitivityScale = 0.1f;
}

ASKGLTFCameraActor::ASKGLTFCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Mode(ESKGLTFCameraControlMode::FreeLook)
	, Target(nullptr)
	, PitchAngleMin(-90.0f)
	, PitchAngleMax(90.0f)
	, YawAngleMin(0.0f)
	, YawAngleMax(360.0f)
	, DistanceMin(100.0f)
	, DistanceMax(1000.0f)
	, DollyDuration(0.2f)
	, DollySensitivity(0.5f)
	, RotationInertia(0.1f)
	, RotationSensitivity(0.3f)
	, Distance(0.0f)
	, Pitch(0.0f)
	, Yaw(0.0f)
	, TargetDistance(0.0f)
	, TargetPitch(0.0f)
	, TargetYaw(0.0f)
	, DollyTime(0.0f)
	, DollyStartDistance(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;
}

#if WITH_EDITOR
void ASKGLTFCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	if (PropertyThatChanged)
	{
		const FName PropertyName = PropertyThatChanged->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Target))
		{
			if (Target == this)
			{
				Target = nullptr;
				// TODO: don't use LogTemp
				UE_LOG(LogTemp, Warning, TEXT("The camera target must not be the camera's own actor"));
			}
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PitchAngleMin))
		{
			PitchAngleMin = FMath::Clamp(PitchAngleMin, -90.0f, 90.0f);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PitchAngleMax))
		{
			PitchAngleMax = FMath::Clamp(PitchAngleMax, -90.0f, 90.0f);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, YawAngleMin))
		{
			YawAngleMin = FMath::Clamp(
				YawAngleMin,
				FMath::Max(-360.0f, YawAngleMax - 360.0f),
				FMath::Min(360.0f, YawAngleMax));
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, YawAngleMax))
		{
			YawAngleMax = FMath::Clamp(
				YawAngleMax,
				FMath::Max(-360.0f, YawAngleMin),
				FMath::Min(360.0f, YawAngleMin + 360.0f));
		}
	}
}
#endif // WITH_EDITOR

void ASKGLTFCameraActor::BeginPlay()
{
	Super::BeginPlay();

	if (Mode == ESKGLTFCameraControlMode::FreeLook)
	{
		const FRotator Rotation = GetActorRotation();

		Pitch = ClampPitch(Rotation.Pitch);
		Yaw = ClampYaw(Rotation.Yaw);
		TargetPitch = Pitch;
		TargetYaw = Yaw;
	}
	else if (Mode == ESKGLTFCameraControlMode::Orbital)
	{
		const FVector FocusPosition = GetFocusPosition();

		// Ensure that the camera is initially aimed at the focus-position
		SetActorRotation(GetLookAtRotation(FocusPosition));

		const FVector Position = GetActorLocation();
		const FRotator Rotation = GetActorRotation();

		// Calculate values based on the current location and orientation
		Distance = ClampDistance((FocusPosition - Position).Size());
		Pitch = ClampPitch(Rotation.Pitch);
		Yaw = ClampYaw(Rotation.Yaw);
		TargetDistance = Distance;
		TargetPitch = Pitch;
		TargetYaw = Yaw;
	}
	else
	{
		checkNoEntry();
	}

	if (InputComponent)
	{
		InputComponent->BindAxisKey(EKeys::MouseX, this, &ASKGLTFCameraActor::OnMouseX);
		InputComponent->BindAxisKey(EKeys::MouseY, this, &ASKGLTFCameraActor::OnMouseY);
		InputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &ASKGLTFCameraActor::OnMouseWheelAxis);
	}
}

void ASKGLTFCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Mode == ESKGLTFCameraControlMode::FreeLook)
	{
		const float Alpha = (RotationInertia == 0.0f) ? 1.0f : FMath::Min(DeltaSeconds / RotationInertia, 1.0f);
		Yaw = FMath::Lerp(Yaw, TargetYaw, Alpha);
		Pitch = FMath::Lerp(Pitch, TargetPitch, Alpha);

		SetActorRotation(FQuat::MakeFromEuler(FVector(0.0f, Pitch, Yaw)));
	}
	else if (Mode == ESKGLTFCameraControlMode::Orbital)
	{
		if (DollyTime != 0.0f)
		{
			DollyTime = FMath::Max(DollyTime - DeltaSeconds, 0.0f);
			Distance = FMath::InterpEaseInOut(DollyStartDistance, TargetDistance, (DollyDuration - DollyTime) / DollyDuration, 1.2f);
		}

		const float Alpha = (RotationInertia == 0.0f) ? 1.0f : FMath::Min(DeltaSeconds / RotationInertia, 1.0f);
		Yaw = FMath::Lerp(Yaw, TargetYaw, Alpha);
		Pitch = FMath::Lerp(Pitch, TargetPitch, Alpha);

		const FTransform FocusTransform = FTransform(GetFocusPosition());
		const FTransform DollyTransform = FTransform(-FVector::ForwardVector * Distance);
		const FTransform RotationTransform = FTransform(FQuat::MakeFromEuler(FVector(0.0f, Pitch, Yaw)));
		const FTransform ResultTransform = DollyTransform * RotationTransform * FocusTransform;

		SetActorTransform(ResultTransform);
	}
}

void ASKGLTFCameraActor::PreInitializeComponents()
{
	AutoReceiveInput = static_cast<EAutoReceiveInput::Type>(GetAutoActivatePlayerIndex() + 1);

	Super::PreInitializeComponents();
}

void ASKGLTFCameraActor::PostActorCreated()
{
	SetAutoActivateForPlayer(EAutoReceiveInput::Player0);
}

void ASKGLTFCameraActor::OnMouseX(float AxisValue)
{
	TargetYaw = ClampYaw(TargetYaw + AxisValue * RotationSensitivity * RotationSensitivityScale);

	if (!UsesYawLimits())
	{
		// Prevent TargetYaw from reaching excessive values by keeping it in the range (-360, 360)
		const float ClampedTargetYaw = FMath::Fmod(TargetYaw, 360.f);

		Yaw += ClampedTargetYaw - TargetYaw;
		TargetYaw = ClampedTargetYaw;
	}
}

void ASKGLTFCameraActor::OnMouseY(float AxisValue)
{
	TargetPitch = ClampPitch(TargetPitch + AxisValue * RotationSensitivity * RotationSensitivityScale);
}

void ASKGLTFCameraActor::OnMouseWheelAxis(float AxisValue)
{
	if (Mode == ESKGLTFCameraControlMode::Orbital)
	{
		if (!FMath::IsNearlyZero(AxisValue))
		{
			const float DeltaDistance = -AxisValue * (TargetDistance * DollySensitivity * DollySensitivityScale);

			DollyTime = DollyDuration;
			TargetDistance = ClampDistance(TargetDistance + DeltaDistance);
			DollyStartDistance = Distance;
		}
	}
}

float ASKGLTFCameraActor::ClampDistance(float Value) const
{
	return FMath::Clamp(Value, DistanceMin, DistanceMax);
}

float ASKGLTFCameraActor::ClampPitch(float Value) const
{
	return FMath::Clamp(Value, PitchAngleMin, PitchAngleMax);
}

float ASKGLTFCameraActor::ClampYaw(float Value) const
{
	return UsesYawLimits() ? FMath::Clamp(Value, YawAngleMin, YawAngleMax) : Value;
}

void ASKGLTFCameraActor::RemoveInertia()
{
	Yaw = TargetYaw;
	Pitch = TargetPitch;
	Distance = TargetDistance;
}

FRotator ASKGLTFCameraActor::GetLookAtRotation(const FVector TargetPosition) const
{
	const FVector EyePosition = GetActorLocation();

	return FRotationMatrix::MakeFromXZ(TargetPosition - EyePosition, FVector::UpVector).Rotator();
}

FVector ASKGLTFCameraActor::GetFocusPosition() const
{
	return this->Target != nullptr ? this->Target->GetActorLocation() : FVector::ZeroVector;
}

bool ASKGLTFCameraActor::SetAutoActivateForPlayer(const EAutoReceiveInput::Type Player)
{
	// TODO: remove hack by adding proper API access to ACameraActor (or by other means)
	FProperty* Property = GetClass()->FindPropertyByName(TEXT("AutoActivateForPlayer"));
	if (Property == nullptr)
	{
		return false;
	}

	TEnumAsByte<EAutoReceiveInput::Type>* ValuePtr = Property->ContainerPtrToValuePtr<TEnumAsByte<EAutoReceiveInput::Type>>(this);
	if (ValuePtr == nullptr)
	{
		return false;
	}

	*ValuePtr = Player;
	return true;
}

bool ASKGLTFCameraActor::UsesYawLimits() const
{
	return !FMath::IsNearlyEqual(YawAngleMax - YawAngleMin, 360.0f);
}
