// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFJsonBuilder.h"

class FGLTFBufferBuilder : public FGLTFJsonBuilder
{
protected:

	FGLTFBufferBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions);
	~FGLTFBufferBuilder();

	const TArray<uint8>& GetBufferData() const;

public:

	FGLTFJsonBufferViewIndex AddBufferView(const void* RawData, uint64 ByteLength, ESKGLTFJsonBufferTarget BufferTarget = ESKGLTFJsonBufferTarget::None, uint8 DataAlignment = 4);

	template <class ElementType>
	FGLTFJsonBufferViewIndex AddBufferView(const TArray<ElementType>& Array, ESKGLTFJsonBufferTarget BufferTarget = ESKGLTFJsonBufferTarget::None, uint8 DataAlignment = 4)
	{
		return AddBufferView(Array.GetData(), Array.Num() * sizeof(ElementType), BufferTarget, DataAlignment);
	}

private:

	FGLTFJsonBufferIndex BufferIndex;
	TUniquePtr<FArchive> BufferArchive;
};
