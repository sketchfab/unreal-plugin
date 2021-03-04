// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Converters/GLTFPackedTypes.h"
#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonVector2.h"
#include "Json/GLTFJsonVector3.h"
#include "Json/GLTFJsonVector4.h"
#include "Json/GLTFJsonColor4.h"
#include "Json/GLTFJsonMatrix4.h"
#include "Json/GLTFJsonQuaternion.h"
#include "Engine/EngineTypes.h"

enum class EGLTFCameraControlMode : unsigned char;

struct FGLTFConverterUtility
{
	static float ConvertLength(const float Length, const float ConversionScale)
	{
		return Length * ConversionScale;
	}

	static FGLTFJsonVector3 ConvertVector(const FVector& Vector)
	{
		// UE4 uses a left-handed coordinate system, with Z up.
		// glTF uses a right-handed coordinate system, with Y up.
		return { Vector.X, Vector.Z, Vector.Y };
	}

	static FGLTFJsonVector3 ConvertPosition(const FVector& Position, const float ConversionScale)
	{
		return ConvertVector(Position * ConversionScale);
	}

	static FGLTFJsonVector3 ConvertScale(const FVector& Scale)
	{
		return ConvertVector(Scale);
	}

	static FGLTFJsonVector3 ConvertNormal(const FVector& Normal)
	{
		return ConvertVector(Normal);
	}

	static FGLTFPackedVector16 ConvertNormal(const FPackedRGBA16N& Normal)
	{
		return { Normal.X, Normal.Z, Normal.Y };
	}

	static FGLTFPackedVector8 ConvertNormal(const FPackedNormal& Normal)
	{
		return { Normal.Vector.X, Normal.Vector.Z, Normal.Vector.Y };
	}

	static FGLTFJsonVector4 ConvertTangent(const FVector& Tangent)
	{
		// glTF stores tangent as Vec4, with W component indicating handedness of tangent basis.
		return { ConvertVector(Tangent), 1.0f };
	}

	static FGLTFPackedVector16 ConvertTangent(const FPackedRGBA16N& Tangent)
	{
		return { Tangent.X, Tangent.Z, Tangent.Y, MAX_int16 /* = 1.0 */ };
	}

	static FGLTFPackedVector8 ConvertTangent(const FPackedNormal& Tangent)
	{
		return { Tangent.Vector.X, Tangent.Vector.Z, Tangent.Vector.Y, MAX_int8 /* = 1.0 */ };
	}

	static FGLTFJsonVector2 ConvertUV(const FVector2D& UV)
	{
		// No conversion actually needed, this is primarily for type-safety.
		return { UV.X, UV.Y };
	}

	static FGLTFJsonColor4 ConvertColor(const FLinearColor& Color)
	{
		// Just make sure its non-negative (which can happen when using MakeFromColorTemperature).
		return {
			FMath::Max(Color.R, 0.0f),
			FMath::Max(Color.G, 0.0f),
			FMath::Max(Color.B, 0.0f),
			FMath::Max(Color.A, 0.0f)
		};
	}

	static FGLTFPackedColor ConvertColor(const FColor& Color)
	{
		// UE4 uses ABGR or ARGB depending on endianness.
		// glTF always uses RGBA independent of endianness.
		return { Color.R, Color.G, Color.B, Color.A };
	}

	static FGLTFJsonQuaternion ConvertRotation(const FQuat& Rotation)
	{
		// UE4 uses a left-handed coordinate system, with Z up.
		// glTF uses a right-handed coordinate system, with Y up.
		// Rotation = (qX, qY, qZ, qW) = (sin(angle/2) * aX, sin(angle/2) * aY, sin(angle/2) * aZ, cons(angle/2))
		// where (aX, aY, aZ) - rotation axis, angle - rotation angle
		// Y swapped with Z between these coordinate systems
		// also, as handedness is changed rotation is inversed - hence negation
		// therefore glTFRotation = (-qX, -qZ, -qY, qw)

		// Not checking if quaternion is normalized
		// e.g. some sources use non-unit Quats for rotation tangents

		// Return the identity quaternion when possible (depending on tolerance)
		if (Rotation.Equals(FQuat::Identity))
		{
			return FGLTFJsonQuaternion::Identity;
		}

		return { -Rotation.X, -Rotation.Z, -Rotation.Y, Rotation.W };
	}

	static FGLTFJsonMatrix4 ConvertMatrix(const FMatrix& Matrix)
	{
		// Unreal stores matrix elements in row major order.
		// glTF stores matrix elements in column major order.

		FGLTFJsonMatrix4 Result;
		for (int32 Row = 0; Row < 4; ++Row)
		{
			for (int32 Col = 0; Col < 4; ++Col)
			{
				Result(Row, Col) = Matrix.M[Row][Col];
			}
		}
		return Result;
	}

	static FGLTFJsonMatrix4 ConvertTransform(const FTransform& Transform, const float ConversionScale)
	{
		const FQuat Rotation = Transform.GetRotation();
		const FVector Translation = Transform.GetTranslation();
		const FVector Scale = Transform.GetScale3D();

		const FQuat ConvertedRotation(-Rotation.X, -Rotation.Z, -Rotation.Y, Rotation.W);
		const FVector ConvertedTranslation = FVector(Translation.X, Translation.Z, Translation.Y) * ConversionScale;
		const FVector ConvertedScale(Scale.X, Scale.Z, Scale.Y);

		const FTransform ConvertedTransform(ConvertedRotation, ConvertedTranslation, ConvertedScale);
		const FMatrix Matrix = ConvertedTransform.ToMatrixWithScale(); // TODO: should it be ToMatrixNoScale()?
		return ConvertMatrix(Matrix);
	}

	static float ConvertFieldOfView(float FOVInDegress, float AspectRatio)
	{
		const float HorizontalFOV = FMath::DegreesToRadians(FOVInDegress);
		const float VerticalFOV = 2 * FMath::Atan(FMath::Tan(HorizontalFOV / 2) / AspectRatio);
		return VerticalFOV;
	}

	static FGLTFJsonQuaternion ConvertCameraDirection()
	{
		// Unreal uses +X axis as camera direction in Unreal coordinates.
		// glTF uses +Y as camera direction in Unreal coordinates.

		return ConvertRotation(FRotator(0, 90, 0).Quaternion());
	}

	static FGLTFJsonQuaternion ConvertLightDirection()
	{
		// Unreal uses +X axis as light direction in Unreal coordinates.
		// glTF uses +Y as light direction in Unreal coordinates.

		return ConvertRotation(FRotator(0, 90, 0).Quaternion());
	}

	static float ConvertLightAngle(const float Angle)
	{
		// Unreal uses degrees.
		// glTF uses radians.
		return FMath::DegreesToRadians(Angle);
	}

	static EGLTFJsonCameraType ConvertCameraType(ECameraProjectionMode::Type ProjectionMode);
	static EGLTFJsonLightType ConvertLightType(ELightComponentType ComponentType);

	static EGLTFJsonInterpolation ConvertInterpolation(const EAnimInterpolationType Type);

	static EGLTFJsonShadingModel ConvertShadingModel(EMaterialShadingModel ShadingModel);

	static EGLTFJsonAlphaMode ConvertAlphaMode(EBlendMode Mode);
	static EGLTFJsonBlendMode ConvertBlendMode(EBlendMode Mode);

	static EGLTFJsonTextureWrap ConvertWrap(TextureAddress Address);

	static EGLTFJsonTextureFilter ConvertMinFilter(TextureFilter Filter);
	static EGLTFJsonTextureFilter ConvertMagFilter(TextureFilter Filter);

	static EGLTFJsonTextureFilter ConvertMinFilter(TextureFilter Filter, TextureGroup LODGroup);
	static EGLTFJsonTextureFilter ConvertMagFilter(TextureFilter Filter, TextureGroup LODGroup);

	static EGLTFJsonCubeFace ConvertCubeFace(ECubeFace CubeFace);

	static EGLTFJsonCameraControlMode ConvertCameraControlMode(EGLTFCameraControlMode CameraMode);

	template <typename ComponentType>
	static EGLTFJsonComponentType GetComponentType()
	{
		if (TIsSame<ComponentType, int8  >::Value) return EGLTFJsonComponentType::S8;
		if (TIsSame<ComponentType, uint8 >::Value) return EGLTFJsonComponentType::U8;
		if (TIsSame<ComponentType, int16 >::Value) return EGLTFJsonComponentType::S16;
		if (TIsSame<ComponentType, uint16>::Value) return EGLTFJsonComponentType::U16;
		if (TIsSame<ComponentType, int32 >::Value) return EGLTFJsonComponentType::S32;
		if (TIsSame<ComponentType, uint32>::Value) return EGLTFJsonComponentType::U32;
		if (TIsSame<ComponentType, float >::Value) return EGLTFJsonComponentType::F32;
		return EGLTFJsonComponentType::None;
	}
};
