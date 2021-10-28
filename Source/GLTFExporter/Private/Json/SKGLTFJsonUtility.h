// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonExtensions.h"

struct FGLTFJsonUtility
{
	template <typename EnumType>
	static int32 ToInteger(EnumType Value)
	{
		return static_cast<int32>(Value);
	}

	static const TCHAR* ToString(ESKGLTFJsonExtension Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonExtension::KHR_LightsPunctual:      return TEXT("KHR_lights_punctual");
			case ESKGLTFJsonExtension::KHR_MaterialsClearCoat:  return TEXT("KHR_materials_clearcoat");
			case ESKGLTFJsonExtension::KHR_MaterialsUnlit:      return TEXT("KHR_materials_unlit");
			case ESKGLTFJsonExtension::KHR_MeshQuantization:    return TEXT("KHR_mesh_quantization");
			case ESKGLTFJsonExtension::KHR_TextureTransform:    return TEXT("KHR_texture_transform");
			case ESKGLTFJsonExtension::EPIC_AnimationHotspots:  return TEXT("EPIC_animation_hotspots");
			case ESKGLTFJsonExtension::EPIC_AnimationPlayback:  return TEXT("EPIC_animation_playback");
			case ESKGLTFJsonExtension::EPIC_BlendModes:         return TEXT("EPIC_blend_modes");
			case ESKGLTFJsonExtension::EPIC_CameraControls:     return TEXT("EPIC_camera_controls");
			case ESKGLTFJsonExtension::EPIC_HDRIBackdrops:      return TEXT("EPIC_hdri_backdrops");
			case ESKGLTFJsonExtension::EPIC_LevelVariantSets:   return TEXT("EPIC_level_variant_sets");
			case ESKGLTFJsonExtension::EPIC_LightmapTextures:   return TEXT("EPIC_lightmap_textures");
			case ESKGLTFJsonExtension::EPIC_SkySpheres:         return TEXT("EPIC_sky_spheres");
			case ESKGLTFJsonExtension::EPIC_TextureHDREncoding: return TEXT("EPIC_texture_hdr_encoding");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonAlphaMode Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonAlphaMode::Opaque: return TEXT("OPAQUE");
			case ESKGLTFJsonAlphaMode::Blend:  return TEXT("BLEND");
			case ESKGLTFJsonAlphaMode::Mask:   return TEXT("MASK");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonBlendMode Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonBlendMode::Additive:       return TEXT("ADDITIVE");
			case ESKGLTFJsonBlendMode::Modulate:       return TEXT("MODULATE");
			case ESKGLTFJsonBlendMode::AlphaComposite: return TEXT("ALPHACOMPOSITE");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonMimeType Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonMimeType::PNG:  return TEXT("image/png");
			case ESKGLTFJsonMimeType::JPEG: return TEXT("image/jpeg");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonAccessorType Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonAccessorType::Scalar: return TEXT("SCALAR");
			case ESKGLTFJsonAccessorType::Vec2:   return TEXT("VEC2");
			case ESKGLTFJsonAccessorType::Vec3:   return TEXT("VEC3");
			case ESKGLTFJsonAccessorType::Vec4:   return TEXT("VEC4");
			case ESKGLTFJsonAccessorType::Mat2:   return TEXT("MAT2");
			case ESKGLTFJsonAccessorType::Mat3:   return TEXT("MAT3");
			case ESKGLTFJsonAccessorType::Mat4:   return TEXT("MAT4");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonHDREncoding Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonHDREncoding::RGBM: return TEXT("RGBM");
			case ESKGLTFJsonHDREncoding::RGBE: return TEXT("RGBE");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonCubeFace Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonCubeFace::PosX: return TEXT("PosX");
			case ESKGLTFJsonCubeFace::NegX: return TEXT("NegX");
			case ESKGLTFJsonCubeFace::PosY: return TEXT("PosY");
			case ESKGLTFJsonCubeFace::NegY: return TEXT("NegY");
			case ESKGLTFJsonCubeFace::PosZ: return TEXT("PosZ");
			case ESKGLTFJsonCubeFace::NegZ: return TEXT("NegZ");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonCameraType Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonCameraType::Perspective:  return TEXT("perspective");
			case ESKGLTFJsonCameraType::Orthographic: return TEXT("orthographic");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonLightType Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonLightType::Directional: return TEXT("directional");
			case ESKGLTFJsonLightType::Point:       return TEXT("point");
			case ESKGLTFJsonLightType::Spot:        return TEXT("spot");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonInterpolation Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonInterpolation::Linear:      return TEXT("LINEAR");
			case ESKGLTFJsonInterpolation::Step:        return TEXT("STEP");
			case ESKGLTFJsonInterpolation::CubicSpline: return TEXT("CUBICSPLINE");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonTargetPath Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonTargetPath::Translation: return TEXT("translation");
			case ESKGLTFJsonTargetPath::Rotation:    return TEXT("rotation");
			case ESKGLTFJsonTargetPath::Scale:       return TEXT("scale");
			case ESKGLTFJsonTargetPath::Weights:     return TEXT("weights");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	static const TCHAR* ToString(ESKGLTFJsonCameraControlMode Value)
	{
		switch (Value)
		{
			case ESKGLTFJsonCameraControlMode::FreeLook: return TEXT("freeLook");
			case ESKGLTFJsonCameraControlMode::Orbital:  return TEXT("orbital");
			default:
				checkNoEntry();
				return TEXT("");
		}
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	static void WriteExactValue(TJsonWriter<CharType, PrintPolicy>& JsonWriter, float Value)
	{
		FString ExactStringRepresentation = FString::Printf(TEXT("%.9g"), Value);
		JsonWriter.WriteRawJSONValue(ExactStringRepresentation);
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	static void WriteExactValue(TJsonWriter<CharType, PrintPolicy>& JsonWriter, const FString& Identifier, float Value)
	{
		FString ExactStringRepresentation = FString::Printf(TEXT("%.9g"), Value);
		JsonWriter.WriteRawJSONValue(Identifier, ExactStringRepresentation);
	}

	template <class WriterType, class ContainerType>
	static void WriteObjectArray(WriterType& JsonWriter, const FString& Identifier, const ContainerType& Container, FGLTFJsonExtensions& Extensions, bool bWriteIfEmpty = false)
	{
		if (Container.Num() > 0 || bWriteIfEmpty)
		{
			JsonWriter.WriteArrayStart(Identifier);
			for (const auto& Element : Container)
			{
				Element.WriteObject(JsonWriter, Extensions);
			}
			JsonWriter.WriteArrayEnd();
		}
	}

	template <class WriterType, class ContainerType>
	static void WriteObjectPtrArray(WriterType& JsonWriter, const FString& Identifier, const ContainerType& Container, FGLTFJsonExtensions& Extensions, bool bWriteIfEmpty = false)
	{
		if (Container.Num() > 0 || bWriteIfEmpty)
		{
			JsonWriter.WriteArrayStart(Identifier);
			for (const auto& Element : Container)
			{
				Element->WriteObject(JsonWriter, Extensions);
			}
			JsonWriter.WriteArrayEnd();
		}
	}

	template <class WriterType, class ContainerType>
	static void WriteStringArray(WriterType& JsonWriter, const FString& Identifier, const ContainerType& Container, bool bWriteIfEmpty = false)
	{
		if (Container.Num() > 0 || bWriteIfEmpty)
		{
			JsonWriter.WriteArrayStart(Identifier);
			for (const auto& Element : Container)
			{
				JsonWriter.WriteValue(ToString(Element));
			}
			JsonWriter.WriteArrayEnd();
		}
	}

	template <class WriterType, class ElementType, SIZE_T Size>
	static void WriteFixedArray(WriterType& JsonWriter, const FString& Identifier, const ElementType (&Array)[Size])
	{
		JsonWriter.WriteArrayStart(Identifier);
		for (const ElementType& Element : Array)
		{
			JsonWriter.WriteValue(Element);
		}
		JsonWriter.WriteArrayEnd();
	}
};
