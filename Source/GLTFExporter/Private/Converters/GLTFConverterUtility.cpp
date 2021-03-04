// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/GLTFConverterUtility.h"
#include "Converters/GLTFTextureUtility.h"
#include "Actors/GLTFCameraActor.h"

EGLTFJsonCameraType FGLTFConverterUtility::ConvertCameraType(ECameraProjectionMode::Type ProjectionMode)
{
	switch (ProjectionMode)
	{
		case ECameraProjectionMode::Perspective:  return EGLTFJsonCameraType::Perspective;
		case ECameraProjectionMode::Orthographic: return EGLTFJsonCameraType::Orthographic;
		default:                                  return EGLTFJsonCameraType::None;
	}
}

EGLTFJsonLightType FGLTFConverterUtility::ConvertLightType(ELightComponentType ComponentType)
{
	switch (ComponentType)
	{
		case LightType_Directional: return EGLTFJsonLightType::Directional;
		case LightType_Point:       return EGLTFJsonLightType::Point;
		case LightType_Spot:        return EGLTFJsonLightType::Spot;
		default:                    return EGLTFJsonLightType::None;
	}
}

EGLTFJsonInterpolation FGLTFConverterUtility::ConvertInterpolation(const EAnimInterpolationType Type)
{
	switch (Type)
	{
		case EAnimInterpolationType::Linear: return EGLTFJsonInterpolation::Linear;
		case EAnimInterpolationType::Step:   return EGLTFJsonInterpolation::Step;
		default:                             return EGLTFJsonInterpolation::None;
	}
}

EGLTFJsonShadingModel FGLTFConverterUtility::ConvertShadingModel(EMaterialShadingModel ShadingModel)
{
	switch (ShadingModel)
	{
		case MSM_Unlit:      return EGLTFJsonShadingModel::Unlit;
		case MSM_DefaultLit: return EGLTFJsonShadingModel::Default;
		case MSM_ClearCoat:  return EGLTFJsonShadingModel::ClearCoat;
		default:             return EGLTFJsonShadingModel::None;
	}
}

EGLTFJsonAlphaMode FGLTFConverterUtility::ConvertAlphaMode(EBlendMode Mode)
{
	switch (Mode)
	{
		case BLEND_Opaque:         return EGLTFJsonAlphaMode::Opaque;
		case BLEND_Masked:         return EGLTFJsonAlphaMode::Mask;
		case BLEND_Translucent:    return EGLTFJsonAlphaMode::Blend;
		case BLEND_Additive:       return EGLTFJsonAlphaMode::Blend;
		case BLEND_Modulate:       return EGLTFJsonAlphaMode::Blend;
		case BLEND_AlphaComposite: return EGLTFJsonAlphaMode::Blend;
		default:                   return EGLTFJsonAlphaMode::None;
	}
}

EGLTFJsonBlendMode FGLTFConverterUtility::ConvertBlendMode(EBlendMode Mode)
{
	switch (Mode)
	{
		case BLEND_Additive:       return EGLTFJsonBlendMode::Additive;
		case BLEND_Modulate:       return EGLTFJsonBlendMode::Modulate;
		case BLEND_AlphaComposite: return EGLTFJsonBlendMode::AlphaComposite;
		default:                   return EGLTFJsonBlendMode::None;
	}
}

EGLTFJsonTextureWrap FGLTFConverterUtility::ConvertWrap(TextureAddress Address)
{
	switch (Address)
	{
		case TA_Wrap:   return EGLTFJsonTextureWrap::Repeat;
		case TA_Mirror: return EGLTFJsonTextureWrap::MirroredRepeat;
		case TA_Clamp:  return EGLTFJsonTextureWrap::ClampToEdge;
		default:        return EGLTFJsonTextureWrap::None; // TODO: add error handling in callers
	}
}

EGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMinFilter(TextureFilter Filter)
{
	switch (Filter)
	{
		case TF_Nearest:   return EGLTFJsonTextureFilter::NearestMipmapNearest;
		case TF_Bilinear:  return EGLTFJsonTextureFilter::LinearMipmapNearest;
		case TF_Trilinear: return EGLTFJsonTextureFilter::LinearMipmapLinear;
		default:           return EGLTFJsonTextureFilter::None;
	}
}

EGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMagFilter(TextureFilter Filter)
{
	switch (Filter)
	{
		case TF_Nearest:   return EGLTFJsonTextureFilter::Nearest;
		case TF_Bilinear:  return EGLTFJsonTextureFilter::Linear;
		case TF_Trilinear: return EGLTFJsonTextureFilter::Linear;
		default:           return EGLTFJsonTextureFilter::None;
	}
}

EGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMinFilter(TextureFilter Filter, TextureGroup LODGroup)
{
	return ConvertMinFilter(Filter == TF_Default ? FGLTFTextureUtility::GetDefaultFilter(LODGroup) : Filter);
}

EGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMagFilter(TextureFilter Filter, TextureGroup LODGroup)
{
	return ConvertMagFilter(Filter == TF_Default ? FGLTFTextureUtility::GetDefaultFilter(LODGroup) : Filter);
}

EGLTFJsonCubeFace FGLTFConverterUtility::ConvertCubeFace(ECubeFace CubeFace)
{
	switch (CubeFace)
	{
		case CubeFace_PosX:	return EGLTFJsonCubeFace::NegX;
		case CubeFace_NegX:	return EGLTFJsonCubeFace::PosX;
		case CubeFace_PosY:	return EGLTFJsonCubeFace::PosZ;
		case CubeFace_NegY:	return EGLTFJsonCubeFace::NegZ;
		case CubeFace_PosZ:	return EGLTFJsonCubeFace::PosY;
		case CubeFace_NegZ:	return EGLTFJsonCubeFace::NegY;
		default:            return EGLTFJsonCubeFace::None; // TODO: add error handling in callers
	}
}

EGLTFJsonCameraControlMode FGLTFConverterUtility::ConvertCameraControlMode(EGLTFCameraControlMode CameraMode)
{
	switch (CameraMode)
	{
		case EGLTFCameraControlMode::FreeLook: return EGLTFJsonCameraControlMode::FreeLook;
		case EGLTFCameraControlMode::Orbital:  return EGLTFJsonCameraControlMode::Orbital;
		default:                               return EGLTFJsonCameraControlMode::None; // TODO: add error handling in callers
	}
}
