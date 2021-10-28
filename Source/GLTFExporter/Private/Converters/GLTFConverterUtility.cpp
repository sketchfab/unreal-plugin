// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFTextureUtility.h"
#include "Actors/SKGLTFCameraActor.h"

ESKGLTFJsonCameraType FGLTFConverterUtility::ConvertCameraType(ECameraProjectionMode::Type ProjectionMode)
{
	switch (ProjectionMode)
	{
		case ECameraProjectionMode::Perspective:  return ESKGLTFJsonCameraType::Perspective;
		case ECameraProjectionMode::Orthographic: return ESKGLTFJsonCameraType::Orthographic;
		default:                                  return ESKGLTFJsonCameraType::None;
	}
}

ESKGLTFJsonLightType FGLTFConverterUtility::ConvertLightType(ELightComponentType ComponentType)
{
	switch (ComponentType)
	{
		case LightType_Directional: return ESKGLTFJsonLightType::Directional;
		case LightType_Point:       return ESKGLTFJsonLightType::Point;
		case LightType_Spot:        return ESKGLTFJsonLightType::Spot;
		default:                    return ESKGLTFJsonLightType::None;
	}
}

ESKGLTFJsonInterpolation FGLTFConverterUtility::ConvertInterpolation(const EAnimInterpolationType Type)
{
	switch (Type)
	{
		case EAnimInterpolationType::Linear: return ESKGLTFJsonInterpolation::Linear;
		case EAnimInterpolationType::Step:   return ESKGLTFJsonInterpolation::Step;
		default:                             return ESKGLTFJsonInterpolation::None;
	}
}

ESKGLTFJsonShadingModel FGLTFConverterUtility::ConvertShadingModel(EMaterialShadingModel ShadingModel)
{
	switch (ShadingModel)
	{
		case MSM_Unlit:      return ESKGLTFJsonShadingModel::Unlit;
		case MSM_DefaultLit: return ESKGLTFJsonShadingModel::Default;
		case MSM_ClearCoat:  return ESKGLTFJsonShadingModel::ClearCoat;
		default:             return ESKGLTFJsonShadingModel::None;
	}
}

ESKGLTFJsonAlphaMode FGLTFConverterUtility::ConvertAlphaMode(EBlendMode Mode)
{
	switch (Mode)
	{
		case BLEND_Opaque:         return ESKGLTFJsonAlphaMode::Opaque;
		case BLEND_Masked:         return ESKGLTFJsonAlphaMode::Mask;
		case BLEND_Translucent:    return ESKGLTFJsonAlphaMode::Blend;
		case BLEND_Additive:       return ESKGLTFJsonAlphaMode::Blend;
		case BLEND_Modulate:       return ESKGLTFJsonAlphaMode::Blend;
		case BLEND_AlphaComposite: return ESKGLTFJsonAlphaMode::Blend;
		default:                   return ESKGLTFJsonAlphaMode::None;
	}
}

ESKGLTFJsonBlendMode FGLTFConverterUtility::ConvertBlendMode(EBlendMode Mode)
{
	switch (Mode)
	{
		case BLEND_Additive:       return ESKGLTFJsonBlendMode::Additive;
		case BLEND_Modulate:       return ESKGLTFJsonBlendMode::Modulate;
		case BLEND_AlphaComposite: return ESKGLTFJsonBlendMode::AlphaComposite;
		default:                   return ESKGLTFJsonBlendMode::None;
	}
}

ESKGLTFJsonTextureWrap FGLTFConverterUtility::ConvertWrap(TextureAddress Address)
{
	switch (Address)
	{
		case TA_Wrap:   return ESKGLTFJsonTextureWrap::Repeat;
		case TA_Mirror: return ESKGLTFJsonTextureWrap::MirroredRepeat;
		case TA_Clamp:  return ESKGLTFJsonTextureWrap::ClampToEdge;
		default:        return ESKGLTFJsonTextureWrap::None; // TODO: add error handling in callers
	}
}

ESKGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMinFilter(TextureFilter Filter)
{
	switch (Filter)
	{
		case TF_Nearest:   return ESKGLTFJsonTextureFilter::NearestMipmapNearest;
		case TF_Bilinear:  return ESKGLTFJsonTextureFilter::LinearMipmapNearest;
		case TF_Trilinear: return ESKGLTFJsonTextureFilter::LinearMipmapLinear;
		default:           return ESKGLTFJsonTextureFilter::None;
	}
}

ESKGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMagFilter(TextureFilter Filter)
{
	switch (Filter)
	{
		case TF_Nearest:   return ESKGLTFJsonTextureFilter::Nearest;
		case TF_Bilinear:  return ESKGLTFJsonTextureFilter::Linear;
		case TF_Trilinear: return ESKGLTFJsonTextureFilter::Linear;
		default:           return ESKGLTFJsonTextureFilter::None;
	}
}

ESKGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMinFilter(TextureFilter Filter, TextureGroup LODGroup)
{
	return ConvertMinFilter(Filter == TF_Default ? FGLTFTextureUtility::GetDefaultFilter(LODGroup) : Filter);
}

ESKGLTFJsonTextureFilter FGLTFConverterUtility::ConvertMagFilter(TextureFilter Filter, TextureGroup LODGroup)
{
	return ConvertMagFilter(Filter == TF_Default ? FGLTFTextureUtility::GetDefaultFilter(LODGroup) : Filter);
}

ESKGLTFJsonCubeFace FGLTFConverterUtility::ConvertCubeFace(ECubeFace CubeFace)
{
	switch (CubeFace)
	{
		case CubeFace_PosX:	return ESKGLTFJsonCubeFace::NegX;
		case CubeFace_NegX:	return ESKGLTFJsonCubeFace::PosX;
		case CubeFace_PosY:	return ESKGLTFJsonCubeFace::PosZ;
		case CubeFace_NegY:	return ESKGLTFJsonCubeFace::NegZ;
		case CubeFace_PosZ:	return ESKGLTFJsonCubeFace::PosY;
		case CubeFace_NegZ:	return ESKGLTFJsonCubeFace::NegY;
		default:            return ESKGLTFJsonCubeFace::None; // TODO: add error handling in callers
	}
}

ESKGLTFJsonCameraControlMode FGLTFConverterUtility::ConvertCameraControlMode(ESKGLTFCameraControlMode CameraMode)
{
	switch (CameraMode)
	{
		case ESKGLTFCameraControlMode::FreeLook: return ESKGLTFJsonCameraControlMode::FreeLook;
		case ESKGLTFCameraControlMode::Orbital:  return ESKGLTFJsonCameraControlMode::Orbital;
		default:                               return ESKGLTFJsonCameraControlMode::None; // TODO: add error handling in callers
	}
}
