// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonColor3.h"
#include "Json/GLTFJsonColor4.h"
#include "Json/GLTFJsonTextureTransform.h"
#include "Json/GLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonTextureInfo
{
	FGLTFJsonTextureIndex Index;
	int32 TexCoord;

	FGLTFJsonTextureTransform Transform;

	FGLTFJsonTextureInfo()
		: TexCoord(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("index"), Index);

		if (TexCoord != 0)
		{
			JsonWriter.WriteValue(TEXT("texCoord"), TexCoord);
		}

		if (Transform != FGLTFJsonTextureTransform())
		{
			const EGLTFJsonExtension Extension = EGLTFJsonExtension::KHR_TextureTransform;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteObjectStart(TEXT("extensions"));
			JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
			Transform.WriteObject(JsonWriter, Extensions);
			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonNormalTextureInfo : FGLTFJsonTextureInfo
{
	float Scale;

	FGLTFJsonNormalTextureInfo()
		: Scale(1)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("index"), Index);

		if (TexCoord != 0)
		{
			JsonWriter.WriteValue(TEXT("texCoord"), TexCoord);
		}

		if (Scale != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("scale"), Scale);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonOcclusionTextureInfo : FGLTFJsonTextureInfo
{
	float Strength;

	FGLTFJsonOcclusionTextureInfo()
		: Strength(1)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("index"), Index);

		if (TexCoord != 0)
		{
			JsonWriter.WriteValue(TEXT("texCoord"), TexCoord);
		}

		if (Strength != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("strength"), Strength);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonPBRMetallicRoughness
{
	FGLTFJsonColor4 BaseColorFactor;
	FGLTFJsonTextureInfo BaseColorTexture;

	float MetallicFactor;
	float RoughnessFactor;
	FGLTFJsonTextureInfo MetallicRoughnessTexture;

	FGLTFJsonPBRMetallicRoughness()
		: BaseColorFactor(FGLTFJsonColor4::White)
		, MetallicFactor(1)
		, RoughnessFactor(1)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (BaseColorFactor != FGLTFJsonColor4::White)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("baseColorFactor"));
			BaseColorFactor.WriteArray(JsonWriter);
		}

		if (BaseColorTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("baseColorTexture"));
			BaseColorTexture.WriteObject(JsonWriter, Extensions);
		}

		if (MetallicFactor != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("metallicFactor"), MetallicFactor);
		}

		if (RoughnessFactor != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("roughnessFactor"), RoughnessFactor);
		}

		if (MetallicRoughnessTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("metallicRoughnessTexture"));
			MetallicRoughnessTexture.WriteObject(JsonWriter, Extensions);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonClearCoatExtension
{
	float ClearCoatFactor;
	FGLTFJsonTextureInfo ClearCoatTexture;

	float ClearCoatRoughnessFactor;
	FGLTFJsonTextureInfo ClearCoatRoughnessTexture;

	FGLTFJsonNormalTextureInfo ClearCoatNormalTexture;

	FGLTFJsonClearCoatExtension()
		: ClearCoatFactor(0)
		, ClearCoatRoughnessFactor(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (ClearCoatFactor != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("clearcoatFactor"), ClearCoatFactor);
		}

		if (ClearCoatTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("clearcoatTexture"));
			ClearCoatTexture.WriteObject(JsonWriter, Extensions);
		}

		if (ClearCoatRoughnessFactor != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("clearcoatRoughnessFactor"), ClearCoatRoughnessFactor);
		}

		if (ClearCoatRoughnessTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("clearcoatRoughnessTexture"));
			ClearCoatRoughnessTexture.WriteObject(JsonWriter, Extensions);
		}

		if (ClearCoatNormalTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("clearcoatNormalTexture"));
			ClearCoatNormalTexture.WriteObject(JsonWriter, Extensions);
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonMaterial
{
	FString Name;

	EGLTFJsonShadingModel ShadingModel;

	FGLTFJsonPBRMetallicRoughness PBRMetallicRoughness;

	FGLTFJsonNormalTextureInfo NormalTexture;
	FGLTFJsonOcclusionTextureInfo OcclusionTexture;

	FGLTFJsonTextureInfo EmissiveTexture;
	FGLTFJsonColor3 EmissiveFactor;

	EGLTFJsonAlphaMode AlphaMode;
	float AlphaCutoff;

	bool DoubleSided;

	EGLTFJsonBlendMode BlendMode;

	FGLTFJsonClearCoatExtension ClearCoat;

	FGLTFJsonMaterial()
		: ShadingModel(EGLTFJsonShadingModel::Default)
		, EmissiveFactor(FGLTFJsonColor3::Black)
		, AlphaMode(EGLTFJsonAlphaMode::Opaque)
		, AlphaCutoff(0.5f)
		, DoubleSided(false)
		, BlendMode(EGLTFJsonBlendMode::None)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		if (ShadingModel != EGLTFJsonShadingModel::None)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("pbrMetallicRoughness"));
			PBRMetallicRoughness.WriteObject(JsonWriter, Extensions);
		}

		if (NormalTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("normalTexture"));
			NormalTexture.WriteObject(JsonWriter, Extensions);
		}

		if (OcclusionTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("occlusionTexture"));
			OcclusionTexture.WriteObject(JsonWriter, Extensions);
		}

		if (EmissiveTexture.Index != INDEX_NONE)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("emissiveTexture"));
			EmissiveTexture.WriteObject(JsonWriter, Extensions);
		}

		if (EmissiveFactor != FGLTFJsonColor3::Black)
		{
			JsonWriter.WriteIdentifierPrefix(TEXT("emissiveFactor"));
			EmissiveFactor.WriteArray(JsonWriter);
		}

		if (AlphaMode != EGLTFJsonAlphaMode::Opaque)
		{
			JsonWriter.WriteValue(TEXT("alphaMode"), FGLTFJsonUtility::ToString(AlphaMode));
		}

		if (AlphaMode == EGLTFJsonAlphaMode::Mask && AlphaCutoff != 0.5f)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("alphaCutoff"), AlphaCutoff);
		}

		if (DoubleSided)
		{
			JsonWriter.WriteValue(TEXT("doubleSided"), DoubleSided);
		}

		if (BlendMode != EGLTFJsonBlendMode::None || ShadingModel == EGLTFJsonShadingModel::Unlit || ShadingModel == EGLTFJsonShadingModel::ClearCoat)
		{
			JsonWriter.WriteObjectStart(TEXT("extensions"));

			if (BlendMode != EGLTFJsonBlendMode::None)
			{
				const EGLTFJsonExtension Extension = EGLTFJsonExtension::EPIC_BlendModes;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteObjectStart();
				JsonWriter.WriteValue(TEXT("blendMode"), FGLTFJsonUtility::ToString(BlendMode));
				JsonWriter.WriteObjectEnd();
			}

			if (ShadingModel == EGLTFJsonShadingModel::Unlit)
			{
				const EGLTFJsonExtension Extension = EGLTFJsonExtension::KHR_MaterialsUnlit;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
				JsonWriter.WriteObjectStart(); // Write empty object
				JsonWriter.WriteObjectEnd();
			}
			else if (ShadingModel == EGLTFJsonShadingModel::ClearCoat)
			{
				const EGLTFJsonExtension Extension = EGLTFJsonExtension::KHR_MaterialsClearCoat;
				Extensions.Used.Add(Extension);

				JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
				ClearCoat.WriteObject(JsonWriter, Extensions);
			}

			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
