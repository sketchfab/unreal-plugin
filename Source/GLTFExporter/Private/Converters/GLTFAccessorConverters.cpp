// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFAccessorConverters.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFSkinWeightVertexBufferHack.h"
#include "Builders/SKGLTFConvertBuilder.h"

namespace
{
	template <typename DestinationType, typename SourceType>
	struct TGLTFVertexNormalUtility
	{
		static DestinationType Convert(const FVector& Normal)
		{
			return FGLTFConverterUtility::ConvertNormal(SourceType(Normal));
		}
	};

	template <typename SourceType>
	struct TGLTFVertexNormalUtility<FGLTFJsonVector3, SourceType>
	{
		static FGLTFJsonVector3 Convert(const FVector& Normal)
		{
			return FGLTFConverterUtility::ConvertNormal(Normal);
		}
	};

	template <typename DestinationType, typename SourceType>
	struct TGLTFVertexTangentUtility
	{
		static DestinationType Convert(const FVector& Tangent)
		{
			return FGLTFConverterUtility::ConvertTangent(SourceType(Tangent));
		}
	};

	template <typename SourceType>
	struct TGLTFVertexTangentUtility<FGLTFJsonVector4, SourceType>
	{
		static FGLTFJsonVector4 Convert(const FVector& Tangent)
		{
			return FGLTFConverterUtility::ConvertTangent(Tangent);
		}
	};
}

FGLTFJsonAccessorIndex FGLTFPositionBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FPositionVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	TArray<FGLTFJsonVector3> Positions;
	Positions.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		Positions[VertexIndex] = FGLTFConverterUtility::ConvertPosition(VertexBuffer->VertexPosition(MappedVertexIndex), Builder.ExportOptions->ExportUniformScale);
	}

	// More accurate bounding box if based on raw vertex values
	FGLTFJsonVector3 MinPosition = VertexCount > 0 ? Positions[0] : FGLTFJsonVector3::Zero;
	FGLTFJsonVector3 MaxPosition = VertexCount > 0 ? Positions[0] : FGLTFJsonVector3::Zero;

	for (uint32 VertexIndex = 1; VertexIndex < VertexCount; ++VertexIndex)
	{
		const FGLTFJsonVector3& Position = Positions[VertexIndex];
		MinPosition.X = FMath::Min(MinPosition.X, Position.X);
		MinPosition.Y = FMath::Min(MinPosition.Y, Position.Y);
		MinPosition.Z = FMath::Min(MinPosition.Z, Position.Z);
		MaxPosition.X = FMath::Max(MaxPosition.X, Position.X);
		MaxPosition.Y = FMath::Max(MaxPosition.Y, Position.Y);
		MaxPosition.Z = FMath::Max(MaxPosition.Z, Position.Z);
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(Positions, ESKGLTFJsonBufferTarget::ArrayBuffer);
	JsonAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
	JsonAccessor.Count = VertexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec3;

	JsonAccessor.MinMaxLength = 3;
	JsonAccessor.Min[0] = MinPosition.X;
	JsonAccessor.Min[1] = MinPosition.Y;
	JsonAccessor.Min[2] = MinPosition.Z;
	JsonAccessor.Max[0] = MaxPosition.X;
	JsonAccessor.Max[1] = MaxPosition.Y;
	JsonAccessor.Max[2] = MaxPosition.Z;

	return Builder.AddAccessor(JsonAccessor);
}

FGLTFJsonAccessorIndex FGLTFColorBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FColorVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	TArray<FGLTFPackedColor> Colors;
	Colors.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		Colors[VertexIndex] = FGLTFConverterUtility::ConvertColor(VertexBuffer->VertexColor(MappedVertexIndex));
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(Colors, ESKGLTFJsonBufferTarget::ArrayBuffer);
	JsonAccessor.ComponentType = ESKGLTFJsonComponentType::U8;
	JsonAccessor.Count = VertexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec4;
	JsonAccessor.bNormalized = true;

	return Builder.AddAccessor(JsonAccessor);
}

FGLTFJsonAccessorIndex FGLTFNormalBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	FGLTFJsonBufferViewIndex BufferViewIndex;
	ESKGLTFJsonComponentType ComponentType;

	const bool bMeshQuantization = Builder.ExportOptions->bExportMeshQuantization;
	const bool bHighPrecision = VertexBuffer->GetUseHighPrecisionTangentBasis();

	if (bMeshQuantization)
	{
		Builder.AddExtension(ESKGLTFJsonExtension::KHR_MeshQuantization, true);

		if (bHighPrecision)
		{
			ComponentType = ESKGLTFJsonComponentType::S16;
			BufferViewIndex = ConvertBufferView<FGLTFPackedVector16, FPackedRGBA16N>(MeshSection, VertexBuffer);
			Builder.GetBufferView(BufferViewIndex).ByteStride = sizeof(FGLTFPackedVector16);
		}
		else
		{
			ComponentType = ESKGLTFJsonComponentType::S8;
			BufferViewIndex = ConvertBufferView<FGLTFPackedVector8, FPackedNormal>(MeshSection, VertexBuffer);
			Builder.GetBufferView(BufferViewIndex).ByteStride = sizeof(FGLTFPackedVector8);
		}
	}
	else
	{
		ComponentType = ESKGLTFJsonComponentType::F32;
		BufferViewIndex = bHighPrecision
			? ConvertBufferView<FGLTFJsonVector3, FPackedRGBA16N>(MeshSection, VertexBuffer)
			: ConvertBufferView<FGLTFJsonVector3, FPackedNormal>(MeshSection, VertexBuffer);
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = BufferViewIndex;
	JsonAccessor.ComponentType = ComponentType;
	JsonAccessor.Count = MeshSection->IndexMap.Num();
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec3;
	JsonAccessor.bNormalized = bMeshQuantization;

	return Builder.AddAccessor(JsonAccessor);
}

template <typename DestinationType, typename SourceType>
FGLTFJsonBufferViewIndex FGLTFNormalBufferConverter::ConvertBufferView(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	const void* TangentData = const_cast<FStaticMeshVertexBuffer*>(VertexBuffer)->GetTangentData();
	if (TangentData == nullptr)
	{
		// TODO: report error
		return FGLTFJsonBufferViewIndex(INDEX_NONE);
	}

	typedef TStaticMeshVertexTangentDatum<SourceType> VertexTangentType;
	const VertexTangentType* VertexTangents = static_cast<const VertexTangentType*>(TangentData);

	TArray<DestinationType> Normals;
	Normals.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		const FVector SafeNormal = VertexTangents[MappedVertexIndex].TangentZ.ToFVector().GetSafeNormal();
		Normals[VertexIndex] = TGLTFVertexNormalUtility<DestinationType, SourceType>::Convert(SafeNormal);
	}

	return Builder.AddBufferView(Normals, ESKGLTFJsonBufferTarget::ArrayBuffer);
}

FGLTFJsonAccessorIndex FGLTFTangentBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	FGLTFJsonBufferViewIndex BufferViewIndex;
	ESKGLTFJsonComponentType ComponentType;

	const bool bMeshQuantization = Builder.ExportOptions->bExportMeshQuantization;
	const bool bHighPrecision = VertexBuffer->GetUseHighPrecisionTangentBasis();

	if (bMeshQuantization)
	{
		Builder.AddExtension(ESKGLTFJsonExtension::KHR_MeshQuantization, true);

		if (bHighPrecision)
		{
			ComponentType = ESKGLTFJsonComponentType::S16;
			BufferViewIndex = ConvertBufferView<FGLTFPackedVector16, FPackedRGBA16N>(MeshSection, VertexBuffer);
		}
		else
		{
			ComponentType = ESKGLTFJsonComponentType::S8;
			BufferViewIndex = ConvertBufferView<FGLTFPackedVector8, FPackedNormal>(MeshSection, VertexBuffer);
		}
	}
	else
	{
		ComponentType = ESKGLTFJsonComponentType::F32;
		BufferViewIndex = bHighPrecision
			? ConvertBufferView<FGLTFJsonVector4, FPackedRGBA16N>(MeshSection, VertexBuffer)
			: ConvertBufferView<FGLTFJsonVector4, FPackedNormal>(MeshSection, VertexBuffer);
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = BufferViewIndex;
	JsonAccessor.ComponentType = ComponentType;
	JsonAccessor.Count = MeshSection->IndexMap.Num();
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec4;
	JsonAccessor.bNormalized = bMeshQuantization;

	return Builder.AddAccessor(JsonAccessor);
}

template <typename DestinationType, typename SourceType>
FGLTFJsonBufferViewIndex FGLTFTangentBufferConverter::ConvertBufferView(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer)
{
	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	const void* TangentData = const_cast<FStaticMeshVertexBuffer*>(VertexBuffer)->GetTangentData();
	if (TangentData == nullptr)
	{
		// TODO: report error
		return FGLTFJsonBufferViewIndex(INDEX_NONE);
	}

	typedef TStaticMeshVertexTangentDatum<SourceType> VertexTangentType;
	const VertexTangentType* VertexTangents = static_cast<const VertexTangentType*>(TangentData);

	TArray<DestinationType> Tangents;
	Tangents.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		const FVector SafeTangent = VertexTangents[MappedVertexIndex].TangentX.ToFVector().GetSafeNormal();
		Tangents[VertexIndex] = TGLTFVertexTangentUtility<DestinationType, SourceType>::Convert(SafeTangent);
	}

	return Builder.AddBufferView(Tangents, ESKGLTFJsonBufferTarget::ArrayBuffer);
}

FGLTFJsonAccessorIndex FGLTFUVBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer, uint32 UVIndex)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	const uint32 UVCount = VertexBuffer->GetNumTexCoords();
	if (UVIndex >= UVCount)
	{
		// TODO: report warning
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	// TODO: report warning or add support for half float precision UVs, i.e. !VertexBuffer->GetUseFullPrecisionUVs()?

	TArray<FGLTFJsonVector2> UVs;
	UVs.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		UVs[VertexIndex] = FGLTFConverterUtility::ConvertUV(VertexBuffer->GetVertexUV(MappedVertexIndex, UVIndex));
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(UVs, ESKGLTFJsonBufferTarget::ArrayBuffer);
	JsonAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
	JsonAccessor.Count = VertexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec2;

	return Builder.AddAccessor(JsonAccessor);
}

FGLTFJsonAccessorIndex FGLTFBoneIndexBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset)
{
	return MeshSection->MaxBoneIndex <= UINT8_MAX
		? Convert<uint8>(MeshSection, VertexBuffer, InfluenceOffset)
		: Convert<uint16>(MeshSection, VertexBuffer, InfluenceOffset);
}

template <typename IndexType>
FGLTFJsonAccessorIndex FGLTFBoneIndexBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset) const
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const uint32 InfluenceCount = VertexBuffer->GetMaxBoneInfluences();
	if (InfluenceOffset >= InfluenceCount)
	{
		// TODO: report warning
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	struct VertexBoneIndices
	{
		IndexType Index[4];
	};

	TArray<VertexBoneIndices> BoneIndices;
	BoneIndices.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const TArray<FBoneIndexType>& BoneMap = MeshSection->BoneMaps[MeshSection->BoneMapLookup[VertexIndex]];
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		VertexBoneIndices& VertexBones = BoneIndices[VertexIndex];

		for (int32 InfluenceIndex = 0; InfluenceIndex < 4; ++InfluenceIndex)
		{
			// TODO: remove hack
			const uint32 UnmappedBoneIndex = FGLTFSkinWeightVertexBufferHack(VertexBuffer).GetBoneIndex(MappedVertexIndex, InfluenceOffset + InfluenceIndex);
			VertexBones.Index[InfluenceIndex] = static_cast<IndexType>(BoneMap[UnmappedBoneIndex]);
		}
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(BoneIndices, ESKGLTFJsonBufferTarget::ArrayBuffer);
	JsonAccessor.ComponentType = FGLTFConverterUtility::GetComponentType<IndexType>();
	JsonAccessor.Count = VertexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec4;

	return Builder.AddAccessor(JsonAccessor);
}

FGLTFJsonAccessorIndex FGLTFBoneWeightBufferConverter::Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset)
{
	if (VertexBuffer == nullptr || VertexBuffer->GetNumVertices() == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const uint32 InfluenceCount = VertexBuffer->GetMaxBoneInfluences();
	if (InfluenceOffset >= InfluenceCount)
	{
		// TODO: report warning
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	const TArray<uint32>& IndexMap = MeshSection->IndexMap;
	const uint32 VertexCount = IndexMap.Num();

	struct VertexBoneWeights
	{
		uint8 Weights[4];
	};

	TArray<VertexBoneWeights> BoneWeights;
	BoneWeights.AddUninitialized(VertexCount);

	for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const uint32 MappedVertexIndex = IndexMap[VertexIndex];
		VertexBoneWeights& VertexWeights = BoneWeights[VertexIndex];

		for (uint32 InfluenceIndex = 0; InfluenceIndex < 4; ++InfluenceIndex)
		{
			// TODO: remove hack
			const uint8 BoneWeight = FGLTFSkinWeightVertexBufferHack(VertexBuffer).GetBoneWeight(MappedVertexIndex, InfluenceOffset + InfluenceIndex);
			VertexWeights.Weights[InfluenceIndex] = BoneWeight;
		}
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(BoneWeights, ESKGLTFJsonBufferTarget::ArrayBuffer);
	JsonAccessor.ComponentType = ESKGLTFJsonComponentType::U8;
	JsonAccessor.Count = VertexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Vec4;
	JsonAccessor.bNormalized = true;

	return Builder.AddAccessor(JsonAccessor);
}

FGLTFJsonAccessorIndex FGLTFIndexBufferConverter::Convert(const FGLTFMeshSection* MeshSection)
{
	const uint32 MaxVertexIndex = MeshSection->IndexMap.Num() - 1;
	if (MaxVertexIndex <= UINT8_MAX) return Convert<uint8>(MeshSection);
	if (MaxVertexIndex <= UINT16_MAX) return Convert<uint16>(MeshSection);
	return Convert<uint32>(MeshSection);
}

template <typename IndexType>
FGLTFJsonAccessorIndex FGLTFIndexBufferConverter::Convert(const FGLTFMeshSection* MeshSection) const
{
	const TArray<uint32>& IndexBuffer = MeshSection->IndexBuffer;
	const uint32 IndexCount = IndexBuffer.Num();
	if (IndexCount == 0)
	{
		return FGLTFJsonAccessorIndex(INDEX_NONE);
	}

	TArray<IndexType> Indices;
	Indices.AddUninitialized(IndexCount);

	for (uint32 Index = 0; Index < IndexCount; ++Index)
	{
		Indices[Index] = static_cast<IndexType>(IndexBuffer[Index]);
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(Indices, ESKGLTFJsonBufferTarget::ElementArrayBuffer, sizeof(IndexType));
	JsonAccessor.ComponentType = FGLTFConverterUtility::GetComponentType<IndexType>();
	JsonAccessor.Count = IndexCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Scalar;

	return Builder.AddAccessor(JsonAccessor);
}
