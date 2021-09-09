// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rendering/SkinWeightVertexBuffer.h"

class FGLTFSkinWeightDataVertexBufferHack : public FVertexBuffer
{
public:

	FORCEINLINE uint32 GetBoneIndexByteSize() const
	{ return (Use16BitBoneIndex() ? sizeof(FBoneIndex16) : sizeof(FBoneIndex8)); }

	FORCEINLINE bool Use16BitBoneIndex() const
	{ return bUse16BitBoneIndex; }

	uint32 GetBoneIndex(uint32 VertexWeightOffset, uint32 VertexInfluenceCount, uint32 InfluenceIndex) const
	{
		if (InfluenceIndex < VertexInfluenceCount)
		{
			uint8* BoneData = Data + VertexWeightOffset;
			if (Use16BitBoneIndex())
			{
				FBoneIndex16* BoneIndex16Ptr = (FBoneIndex16*)BoneData;
				return BoneIndex16Ptr[InfluenceIndex];
			}
			else
			{
				return BoneData[InfluenceIndex];
			}
		}
		else
		{
			return 0;
		}
	}

	uint8 GetBoneWeight(uint32 VertexWeightOffset, uint32 VertexInfluenceCount, uint32 InfluenceIndex) const
	{
		if (InfluenceIndex < VertexInfluenceCount)
		{
			uint8* BoneData = Data + VertexWeightOffset;
			uint32 BoneWeightOffset = GetBoneIndexByteSize() * VertexInfluenceCount;
			return BoneData[BoneWeightOffset + InfluenceIndex];
		}
		else
		{
			return 0;
		}
	}

	// guaranteed only to be valid if the vertex buffer is valid
	FShaderResourceViewRHIRef SRVValue;

	/** true if this vertex buffer will be used with CPU skinning. Resource arrays are set to cpu accessible if this is true */
	bool bNeedsCPUAccess;

	bool bVariableBonesPerVertex;

	/** Has extra bone influences per Vertex, which means using a different TGPUSkinVertexBase */
	uint32 MaxBoneInfluences;

	/** Use 16 bit bone index instead of 8 bit */
	bool bUse16BitBoneIndex;

	/** The vertex data storage type */
	FStaticMeshVertexDataInterface* WeightData;

	/** The cached vertex data pointer. */
	uint8* Data;

	/** The cached number of vertices. */
	uint32 NumVertices;

	/** The cached number of bones. */
	uint32 NumBones;
};


class FGLTFSkinWeightLookupVertexBufferHack : public FVertexBuffer
{
public:

	void GetWeightOffsetAndInfluenceCount(uint32 VertexIndex, uint32& OutWeightOffset, uint32& OutInfluenceCount) const
	{
		uint32 Offset = VertexIndex * 4;
		uint32 DataUInt32 = *((uint32*)(&Data[Offset]));
		OutWeightOffset = DataUInt32 >> 8;
		OutInfluenceCount = DataUInt32 & 0xff;
	}

	// guaranteed only to be valid if the vertex buffer is valid
	FShaderResourceViewRHIRef SRVValue;

	/** true if this vertex buffer will be used with CPU skinning. Resource arrays are set to cpu accessible if this is true */
	bool bNeedsCPUAccess;

	/** The vertex data storage type */
	FStaticMeshVertexDataInterface* LookupData;

	/** The cached vertex data pointer. */
	uint8* Data;

	/** The cached number of vertices. */
	uint32 NumVertices;
};

class FGLTFSkinWeightVertexBufferHack
{
public:

	FGLTFSkinWeightVertexBufferHack(const FSkinWeightVertexBuffer* VertexBuffer)
		: VertexBuffer(*VertexBuffer)
		, DataVertexBuffer(*(FGLTFSkinWeightDataVertexBufferHack*)VertexBuffer->GetDataVertexBuffer())
		, LookupVertexBuffer(*(FGLTFSkinWeightLookupVertexBufferHack*)VertexBuffer->GetLookupVertexBuffer())
	{
	}

	bool GetVariableBonesPerVertex() const { return VertexBuffer.GetVariableBonesPerVertex(); }
	uint32 GetConstantInfluencesVertexStride() const { return VertexBuffer.GetConstantInfluencesVertexStride(); }
	uint32 GetMaxBoneInfluences() const { return VertexBuffer.GetMaxBoneInfluences(); }

	void GetVertexInfluenceOffsetCount(uint32 VertexIndex, uint32& VertexWeightOffset, uint32& VertexInfluenceCount) const
	{
		if (GetVariableBonesPerVertex())
		{
			LookupVertexBuffer.GetWeightOffsetAndInfluenceCount(VertexIndex, VertexWeightOffset, VertexInfluenceCount);
		}
		else
		{
			VertexWeightOffset = GetConstantInfluencesVertexStride() * VertexIndex;
			VertexInfluenceCount = GetMaxBoneInfluences();
		}
	}

	uint32 GetBoneIndex(uint32 VertexIndex, uint32 InfluenceIndex) const
	{
		uint32 VertexWeightOffset = 0;
		uint32 VertexInfluenceCount = 0;
		GetVertexInfluenceOffsetCount(VertexIndex, VertexWeightOffset, VertexInfluenceCount);
		return DataVertexBuffer.GetBoneIndex(VertexWeightOffset, VertexInfluenceCount, InfluenceIndex);
	}

	uint8 GetBoneWeight(uint32 VertexIndex, uint32 InfluenceIndex) const
	{
		uint32 VertexWeightOffset = 0;
		uint32 VertexInfluenceCount = 0;
		GetVertexInfluenceOffsetCount(VertexIndex, VertexWeightOffset, VertexInfluenceCount);
		return DataVertexBuffer.GetBoneWeight(VertexWeightOffset, VertexInfluenceCount, InfluenceIndex);
	}

private:

	const FSkinWeightVertexBuffer& VertexBuffer;
	const FGLTFSkinWeightDataVertexBufferHack& DataVertexBuffer;
	const FGLTFSkinWeightLookupVertexBufferHack& LookupVertexBuffer;
};
