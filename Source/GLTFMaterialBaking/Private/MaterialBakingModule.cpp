// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialBakingModule.h"
#include "MaterialRenderItem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ExportMaterialProxy.h"
#include "Interfaces/IMainFrameModule.h"
#include "UObject/UObjectGlobals.h"
#include "MaterialBakingStructures.h"
#include "Framework/Application/SlateApplication.h"
#include "MaterialBakingHelpers.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Materials/MaterialInstance.h"
#include "RenderingThread.h"
#include "RHISurfaceDataConversion.h"
#include "Misc/ScopedSlowTask.h"
#include "MeshDescription.h"
#if WITH_EDITOR
#include "Misc/FileHelper.h"
#endif

#if ENGINE_MAJOR_VERSION == 5
#define INDEX_BUFFER FBufferRHIRef
#define VERTEX_BUFFER FBufferRHIRef
#else
#define INDEX_BUFFER FIndexBufferRHIRef
#define VERTEX_BUFFER FVertexBufferRHIRef
#endif

IMPLEMENT_MODULE(FSKMaterialBakingModule, SKGLTFMaterialBaking);

#define LOCTEXT_NAMESPACE "SKMaterialBakingModule"

/** Cvars for advanced features */
static TAutoConsoleVariable<int32> CVarUseMaterialProxyCaching(
	TEXT("MaterialBaking.UseMaterialProxyCaching"),
	1,
	TEXT("Determines whether or not Material Proxies should be cached to speed up material baking.\n")
	TEXT("0: Turned Off\n")
	TEXT("1: Turned On"),	
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarSaveIntermediateTextures(
	TEXT("MaterialBaking.SaveIntermediateTextures"),
	0,
	TEXT("Determines whether or not to save out intermediate BMP images for each flattened material property.\n")
	TEXT("0: Turned Off\n")
	TEXT("1: Turned On"),
	ECVF_Default);

namespace FSKMaterialBakingModuleImpl
{
	// Custom dynamic mesh allocator specifically tailored for Material Baking.
	// This will always reuse the same couple buffers, so searching linearly is not a problem.
	class FMaterialBakingDynamicMeshBufferAllocator : public FDynamicMeshBufferAllocator
	{
		// This must be smaller than the large allocation blocks on Windows 10 which is currently ~508K.
		// Large allocations uses VirtualAlloc directly without any kind of buffering before
		// releasing pages to the kernel, so it causes lots of soft page fault when
		// memory is first initialized.
		const uint32 SmallestPooledBufferSize = 256*1024;

		TArray<INDEX_BUFFER>  IndexBuffers;
		TArray<VERTEX_BUFFER> VertexBuffers;

		template <typename RefType>
		RefType GetSmallestFit(uint32 SizeInBytes, TArray<RefType>& Array)
		{
			uint32 SmallestFitIndex = UINT32_MAX;
			uint32 SmallestFitSize  = UINT32_MAX;
			for (int32 Index = 0; Index < Array.Num(); ++Index)
			{
				uint32 Size = Array[Index]->GetSize();
				if (Size >= SizeInBytes && (SmallestFitIndex == UINT32_MAX || Size < SmallestFitSize))
				{
					SmallestFitIndex = Index;
					SmallestFitSize  = Size;
				}
			}

			RefType Ref;
			// Do not reuse the smallest fit if it's a lot bigger than what we requested
			if (SmallestFitIndex != UINT32_MAX && SmallestFitSize < SizeInBytes*2)
			{
				Ref = Array[SmallestFitIndex];
				Array.RemoveAtSwap(SmallestFitIndex);
			}

			return Ref;
		}

		virtual INDEX_BUFFER AllocIndexBuffer(uint32 NumElements) override
		{
			uint32 BufferSize = GetIndexBufferSize(NumElements);
			if (BufferSize > SmallestPooledBufferSize)
			{
				INDEX_BUFFER Ref = GetSmallestFit(GetIndexBufferSize(NumElements), IndexBuffers);
				if (Ref.IsValid())
				{
					return Ref;
				}
			}

			return FDynamicMeshBufferAllocator::AllocIndexBuffer(NumElements);
		}

		virtual void ReleaseIndexBuffer(INDEX_BUFFER& IndexBufferRHI) override
		{
			if (IndexBufferRHI->GetSize() > SmallestPooledBufferSize)
			{
				IndexBuffers.Add(MoveTemp(IndexBufferRHI));
			}

			IndexBufferRHI = nullptr;
		}

		virtual VERTEX_BUFFER AllocVertexBuffer(uint32 Stride, uint32 NumElements) override
		{
			uint32 BufferSize = GetVertexBufferSize(Stride, NumElements);
			if (BufferSize > SmallestPooledBufferSize)
			{
				VERTEX_BUFFER Ref = GetSmallestFit(BufferSize, VertexBuffers);
				if (Ref.IsValid())
				{
					return Ref;
				}
			}

			return FDynamicMeshBufferAllocator::AllocVertexBuffer(Stride, NumElements);
		}

		virtual void ReleaseVertexBuffer(VERTEX_BUFFER& VertexBufferRHI) override
		{
			if (VertexBufferRHI->GetSize() > SmallestPooledBufferSize)
			{
				VertexBuffers.Add(MoveTemp(VertexBufferRHI));
			}

			VertexBufferRHI = nullptr;
		}
	};

	class FStagingBufferPool
	{
	public:
		FTexture2DRHIRef CreateStagingBuffer_RenderThread(FRHICommandListImmediate& RHICmdList, int32 Width, int32 Height, EPixelFormat Format)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(CreateStagingBuffer_RenderThread)

			auto StagingBufferPredicate = 
				[Width, Height, Format](const FTexture2DRHIRef& Texture2DRHIRef)
				{
					return Texture2DRHIRef->GetSizeX() == Width && Texture2DRHIRef->GetSizeY() == Height && Texture2DRHIRef->GetFormat() == Format;
				};

			// Process any staging buffers available for unmapping
			{
				TArray<FTexture2DRHIRef> ToUnmapLocal;
				{
					FScopeLock Lock(&ToUnmapLock);
					ToUnmapLocal = MoveTemp(ToUnmap);
				}

				for (int32 Index = 0, Num = ToUnmapLocal.Num(); Index < Num; ++Index)
				{
					RHICmdList.UnmapStagingSurface(ToUnmapLocal[Index]);
					Pool.Add(MoveTemp(ToUnmapLocal[Index]));
				}
			}

			// Find any pooled staging buffer with suitable properties.
			int32 Index = Pool.IndexOfByPredicate(StagingBufferPredicate);

			if (Index != -1)
			{
				FTexture2DRHIRef StagingBuffer = MoveTemp(Pool[Index]);
				Pool.RemoveAtSwap(Index);
				return StagingBuffer;
			}

			TRACE_CPUPROFILER_EVENT_SCOPE(RHICreateTexture2D)
#if ENGINE_MAJOR_VERSION == 5
			const TCHAR* DebugName = TEXT("");
			FRHIResourceCreateInfo CreateInfo(DebugName);
#else
			FRHIResourceCreateInfo CreateInfo;
#endif
			return RHICreateTexture2D(Width, Height, Format, 1, 1, TexCreate_CPUReadback, CreateInfo);
		}

		void ReleaseStagingBufferForUnmap_AnyThread(FTexture2DRHIRef& Texture2DRHIRef)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(ReleaseStagingBufferForUnmap_AnyThread)
			FScopeLock Lock(&ToUnmapLock);
			ToUnmap.Emplace(MoveTemp(Texture2DRHIRef));
		}

		void Clear_RenderThread(FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Clear_RenderThread)
			for (FTexture2DRHIRef& StagingSurface : ToUnmap)
			{
				RHICmdList.UnmapStagingSurface(StagingSurface);
			}

			ToUnmap.Empty();
			Pool.Empty();
		}

		~FStagingBufferPool()
		{
			check(Pool.Num() == 0);
		}

	private:
		TArray<FTexture2DRHIRef> Pool;

		// Not contented enough to warrant the use of lockless structures.
		FCriticalSection         ToUnmapLock;
		TArray<FTexture2DRHIRef> ToUnmap;
	};

	struct FRenderItemKey
	{
		const FMeshData* RenderData;
		const FIntPoint  RenderSize;

		FRenderItemKey(const FMeshData* InRenderData, const FIntPoint& InRenderSize)
			: RenderData(InRenderData)
			, RenderSize(InRenderSize)
		{
		}

		bool operator == (const FRenderItemKey& Other) const
		{
			return RenderData == Other.RenderData &&
				RenderSize == Other.RenderSize;
		}
	};

	uint32 GetTypeHash(const FRenderItemKey& Key)
	{
		return HashCombine(GetTypeHash(Key.RenderData), GetTypeHash(Key.RenderSize));
	}
}

void FSKMaterialBakingModule::StartupModule()
{
	// Set which properties should enforce gamma correction
	PerPropertyGamma.Add(MP_Normal);
	PerPropertyGamma.Add(MP_Opacity);
	PerPropertyGamma.Add(MP_OpacityMask);
	PerPropertyGamma.Add(MP_ShadingModel);
	PerPropertyGamma.Add(TEXT("ClearCoatBottomNormal"));

	// Set which pixel format should be used for the possible baked out material properties
	PerPropertyFormat.Add(MP_EmissiveColor, PF_FloatRGBA);
	PerPropertyFormat.Add(MP_Opacity, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_OpacityMask, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_BaseColor, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Metallic, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Specular, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Roughness, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Anisotropy, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Normal, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_Tangent, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_AmbientOcclusion, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_SubsurfaceColor, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_CustomData0, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_CustomData1, PF_B8G8R8A8);
	PerPropertyFormat.Add(MP_ShadingModel, PF_B8G8R8A8);
	PerPropertyFormat.Add(TEXT("ClearCoatBottomNormal"), PF_B8G8R8A8);

	// Register callback for modified objects
	FCoreUObjectDelegates::OnObjectModified.AddRaw(this, &FSKMaterialBakingModule::OnObjectModified);
}

void FSKMaterialBakingModule::ShutdownModule()
{
	// Unregister customization and callback
	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

	if (PropertyEditorModule)
	{
		PropertyEditorModule->UnregisterCustomPropertyTypeLayout(TEXT("PropertyEntry"));
	}
	FCoreUObjectDelegates::OnObjectModified.RemoveAll(this);
}

void FSKMaterialBakingModule::BakeMaterials(const TArray<FMaterialData*>& MaterialSettings, const TArray<FMeshData*>& MeshSettings, TArray<FBakeOutput>& Output)
{
	TArray<FMaterialDataEx> MaterialDataExs;
	TArray<FMaterialDataEx*> MaterialSettingsEx;

	// Translate old material data to extended types
	for (const FMaterialData* MaterialData : MaterialSettings)
	{
		FMaterialDataEx& MaterialDataEx = MaterialDataExs.AddDefaulted_GetRef();
		MaterialDataEx.Material = MaterialData->Material;
		MaterialDataEx.bPerformBorderSmear = MaterialData->bPerformBorderSmear;
		MaterialDataEx.BlendMode = MaterialData->BlendMode;

		for (const TPair<EMaterialProperty, FIntPoint>& PropertySizePair : MaterialData->PropertySizes)
		{
			MaterialDataEx.PropertySizes.Add(PropertySizePair.Key, PropertySizePair.Value);
		}

		MaterialSettingsEx.Add(&MaterialDataEx);
	}

	TArray<FBakeOutputEx> BakeOutputExs;
	BakeMaterials(MaterialSettingsEx, MeshSettings, BakeOutputExs);

	// Translate extended bake output to old types
	for (FBakeOutputEx& BakeOutputEx : BakeOutputExs)
	{
		FBakeOutput& BakeOutput = Output.AddDefaulted_GetRef();
		BakeOutput.EmissiveScale = BakeOutputEx.EmissiveScale;

		for (TPair<FMaterialPropertyEx, FIntPoint>& PropertySizePair : BakeOutputEx.PropertySizes)
		{
			BakeOutput.PropertySizes.Add(PropertySizePair.Key.Type, PropertySizePair.Value);
		}

		for (TPair<FMaterialPropertyEx, TArray<FColor>>& PropertyDataPair : BakeOutputEx.PropertyData)
		{
			BakeOutput.PropertyData.Add(PropertyDataPair.Key.Type, MoveTemp(PropertyDataPair.Value));
		}
	}
}

void FSKMaterialBakingModule::BakeMaterials(const TArray<FMaterialDataEx*>& MaterialSettings, const TArray<FMeshData*>& MeshSettings, TArray<FBakeOutputEx>& Output)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSKMaterialBakingModule::BakeMaterials)

	checkf(MaterialSettings.Num() == MeshSettings.Num(), TEXT("Number of material settings does not match that of MeshSettings"));
	const int32 NumMaterials = MaterialSettings.Num();
	const bool bSaveIntermediateTextures = CVarSaveIntermediateTextures.GetValueOnAnyThread() == 1;

	using namespace FSKMaterialBakingModuleImpl;
	FMaterialBakingDynamicMeshBufferAllocator MaterialBakingDynamicMeshBufferAllocator;

	FScopedSlowTask Progress(NumMaterials, LOCTEXT("BakeMaterials", "Baking Materials..."), true );
	Progress.MakeDialog(true);

	TArray<uint32> ProcessingOrder;
	ProcessingOrder.Reserve(MeshSettings.Num());
	for (int32 Index = 0; Index < MeshSettings.Num(); ++Index)
	{
		ProcessingOrder.Add(Index);
	}

	// Start with the biggest mesh first so we can always reuse the same vertex/index buffers.
	// This will decrease the number of allocations backed by newly allocated memory from the OS,
	// which will reduce soft page faults while copying into that memory.
	// Soft page faults are now incredibly expensive on Windows 10.
	Algo::SortBy(
		ProcessingOrder,
		[&MeshSettings](const uint32 Index){ return MeshSettings[Index]->RawMeshDescription ? MeshSettings[Index]->RawMeshDescription->Vertices().Num() : 0; },
		TGreater<>()
	);

	Output.SetNum(NumMaterials);

	struct FPipelineContext
	{
		typedef TFunction<void (FRHICommandListImmediate& RHICmdList)> FReadCommand;
		FReadCommand ReadCommand;
	};

	// Distance between the command sent to rendering and the GPU read-back of the result
	// to minimize sync time waiting on GPU.
	const int32 PipelineDepth = 16;
	int32 PipelineIndex = 0;
	FPipelineContext PipelineContext[PipelineDepth];

	// This will create and prepare FMeshMaterialRenderItem for each property sizes we're going to need
	auto PrepareRenderItems_AnyThread =
		[&](int32 MaterialIndex)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PrepareRenderItems);
		
		TMap<FSKMaterialBakingModuleImpl::FRenderItemKey, FMeshMaterialRenderItem*>* RenderItems = new TMap<FRenderItemKey, FMeshMaterialRenderItem *>();
		const FMaterialDataEx* CurrentMaterialSettings = MaterialSettings[MaterialIndex];
		const FMeshData* CurrentMeshSettings = MeshSettings[MaterialIndex];

		for (TMap<FMaterialPropertyEx, FIntPoint>::TConstIterator PropertySizeIterator = CurrentMaterialSettings->PropertySizes.CreateConstIterator(); PropertySizeIterator; ++PropertySizeIterator)
		{
			FRenderItemKey RenderItemKey(CurrentMeshSettings, PropertySizeIterator.Value());
			if (RenderItems->Find(RenderItemKey) == nullptr)
			{
#if ENGINE_MAJOR_VERSION == 5
				FMeshMaterialRenderItem* T = new FMeshMaterialRenderItem(PropertySizeIterator.Value(), CurrentMeshSettings, &MaterialBakingDynamicMeshBufferAllocator);
				RenderItems->Add(RenderItemKey, T);
#else
				RenderItems->Add(RenderItemKey, new FMeshMaterialRenderItem(PropertySizeIterator.Value(), CurrentMeshSettings, &MaterialBakingDynamicMeshBufferAllocator));
#endif
			}
		}

		return RenderItems;
	};

	// We reuse the pipeline depth to prepare render items in advance to avoid stalling the game thread
	int NextRenderItem = 0;
	TFuture<TMap<FRenderItemKey, FMeshMaterialRenderItem*>*> PreparedRenderItems[PipelineDepth];
	for (; NextRenderItem < NumMaterials && NextRenderItem < PipelineDepth; ++NextRenderItem)
	{
		PreparedRenderItems[NextRenderItem] = 
			Async(
				EAsyncExecution::ThreadPool,
				[&PrepareRenderItems_AnyThread, &ProcessingOrder, NextRenderItem]()
				{
					return PrepareRenderItems_AnyThread(ProcessingOrder[NextRenderItem]);
				}
			);
	}

	// Create all material proxies right away to start compiling shaders asynchronously and avoid stalling the baking process as much as possible
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(CreateMaterialProxies)

		for (int32 Index = 0; Index < NumMaterials; ++Index)
		{
			int32 MaterialIndex = ProcessingOrder[Index];
			const FMaterialDataEx* CurrentMaterialSettings = MaterialSettings[MaterialIndex];

			TArray<UTexture*> MaterialTextures;
			CurrentMaterialSettings->Material->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);

			// Force load materials used by the current material
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(LoadTexturesForMaterial)

				for (UTexture* Texture : MaterialTextures)
				{
					if (Texture != NULL)
					{
						UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
						if (Texture2D)
						{
							Texture2D->SetForceMipLevelsToBeResident(30.0f);
							Texture2D->WaitForStreaming();
						}
					}
				}
			}

			for (TMap<FMaterialPropertyEx, FIntPoint>::TConstIterator PropertySizeIterator = CurrentMaterialSettings->PropertySizes.CreateConstIterator(); PropertySizeIterator; ++PropertySizeIterator)
			{
				// They will be stored in the pool and compiled asynchronously
				CreateMaterialProxy(*CurrentMaterialSettings, PropertySizeIterator.Key());
			}
		}
	}

	TAtomic<uint32>    NumTasks(0);
	FStagingBufferPool StagingBufferPool;

	for (int32 Index = 0; Index < NumMaterials; ++Index)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(BakeOneMaterial)

		Progress.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("BakingMaterial", "Baking Material {0}/{1}"), Index, NumMaterials));

		int32 MaterialIndex = ProcessingOrder[Index];
		TMap<FRenderItemKey, FMeshMaterialRenderItem*>* RenderItems;

		{
			TRACE_CPUPROFILER_EVENT_SCOPE(WaitOnPreparedRenderItems)
			RenderItems = PreparedRenderItems[Index % PipelineDepth].Get();
		}

		// Prepare the next render item in advance
		if (NextRenderItem < NumMaterials)
		{
			check((NextRenderItem % PipelineDepth) == (Index % PipelineDepth));
			PreparedRenderItems[NextRenderItem % PipelineDepth] = 
				Async(
					EAsyncExecution::ThreadPool,
					[&PrepareRenderItems_AnyThread, NextMaterialIndex = ProcessingOrder[NextRenderItem]]()
					{
						return PrepareRenderItems_AnyThread(NextMaterialIndex);
					}
				);
			NextRenderItem++;
		}

		const FMaterialDataEx* CurrentMaterialSettings = MaterialSettings[MaterialIndex];
		const FMeshData* CurrentMeshSettings = MeshSettings[MaterialIndex];
		FBakeOutputEx& CurrentOutput = Output[MaterialIndex];

		TArray<FMaterialPropertyEx> MaterialPropertiesToBakeOut;
		CurrentMaterialSettings->PropertySizes.GenerateKeyArray(MaterialPropertiesToBakeOut);

		const int32 NumPropertiesToRender = MaterialPropertiesToBakeOut.Num();
		if (NumPropertiesToRender > 0)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(RenderOneMaterial)

			// Ensure data in memory will not change place passed this point to avoid race conditions
			CurrentOutput.PropertySizes = CurrentMaterialSettings->PropertySizes;
			for (int32 PropertyIndex = 0; PropertyIndex < NumPropertiesToRender; ++PropertyIndex)
			{
				const FMaterialPropertyEx& Property = MaterialPropertiesToBakeOut[PropertyIndex];
				CurrentOutput.PropertyData.Add(Property);
			}

			for (int32 PropertyIndex = 0; PropertyIndex < NumPropertiesToRender; ++PropertyIndex)
			{
				const FMaterialPropertyEx& Property = MaterialPropertiesToBakeOut[PropertyIndex];
				FExportMaterialProxy* ExportMaterialProxy = CreateMaterialProxy(*CurrentMaterialSettings, Property);

				if (!ExportMaterialProxy->IsCompilationFinished())
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(WaitForMaterialProxyCompilation)
					ExportMaterialProxy->FinishCompilation();
				}

				// Lookup gamma and format settings for property, if not found use default values
				bool InForceLinearGamma = PerPropertyGamma.Contains(Property);
				EPixelFormat PixelFormat = PerPropertyFormat.Contains(Property) ? PerPropertyFormat[Property] : PF_B8G8R8A8;

				// It is safe to reuse the same render target for each draw pass since they all execute sequentially on the GPU and are copied to staging buffers before
				// being reused.
				UTextureRenderTarget2D* RenderTarget = CreateRenderTarget(InForceLinearGamma, PixelFormat, CurrentOutput.PropertySizes[Property]);
				if (RenderTarget != nullptr)
				{
					// Perform everything left of the operation directly on the render thread since we need to modify some RenderItem's properties
					// for each render pass and we can't do that without costly synchronization (flush) between the game thread and render thread.
					// Everything slow to execute has already been prepared on the game thread anyway.
					ENQUEUE_RENDER_COMMAND(RenderOneMaterial)(
						[this, RenderItems, RenderTarget, Property, ExportMaterialProxy, &PipelineContext, PipelineIndex, &StagingBufferPool, &NumTasks, bSaveIntermediateTextures, &MaterialSettings, &MeshSettings, MaterialIndex, &Output](FRHICommandListImmediate& RHICmdList)
						{
							const FMaterialDataEx* CurrentMaterialSettings = MaterialSettings[MaterialIndex];
							const FMeshData* CurrentMeshSettings = MeshSettings[MaterialIndex];

							FMeshMaterialRenderItem& RenderItem = *RenderItems->FindChecked(FRenderItemKey(CurrentMeshSettings, FIntPoint(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight())));

							FSceneViewFamily ViewFamily(FSceneViewFamily::ConstructionValues(RenderTarget->GetRenderTargetResource(), nullptr,
								FEngineShowFlags(ESFIM_Game))
								.SetWorldTimes(0.0f, 0.0f, 0.0f)
								.SetGammaCorrection(RenderTarget->GetRenderTargetResource()->GetDisplayGamma()));

							RenderItem.MaterialRenderProxy = ExportMaterialProxy;
							RenderItem.ViewFamily = &ViewFamily;

							FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GetRenderTargetResource();
							FCanvas Canvas(RenderTargetResource, nullptr, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, GMaxRHIFeatureLevel);
							Canvas.SetAllowedModes(FCanvas::Allow_Flush);
							Canvas.SetRenderTargetRect(FIntRect(0, 0, RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight()));
							Canvas.SetBaseTransform(Canvas.CalcBaseTransform2D(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight()));

							// Do rendering
							Canvas.Clear(RenderTarget->ClearColor);
							FCanvas::FCanvasSortElement& SortElement = Canvas.GetSortElement(Canvas.TopDepthSortKey());
							SortElement.RenderBatchArray.Add(&RenderItem);
							Canvas.Flush_RenderThread(RHICmdList);
							SortElement.RenderBatchArray.Empty();

							FTexture2DRHIRef StagingBufferRef = StagingBufferPool.CreateStagingBuffer_RenderThread(RHICmdList, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY(), RenderTarget->GetFormat());
							FGPUFenceRHIRef GPUFence = RHICreateGPUFence(TEXT("MaterialBackingFence"));

							FResolveRect Rect(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
							RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), StagingBufferRef, FResolveParams(Rect));	
							RHICmdList.WriteGPUFence(GPUFence);

							// Prepare a lambda for final processing that will be executed asynchronously
							NumTasks++;
							auto FinalProcessing_AnyThread =
								[&NumTasks, bSaveIntermediateTextures, CurrentMaterialSettings, &StagingBufferPool, &Output, Property, MaterialIndex](FTexture2DRHIRef& StagingBuffer, void * Data, int32 DataWidth, int32 DataHeight)
								{
									TRACE_CPUPROFILER_EVENT_SCOPE(FinalProcessing)

									FBakeOutputEx& CurrentOutput  = Output[MaterialIndex];
									TArray<FColor>& OutputColor = CurrentOutput.PropertyData[Property];
									FIntPoint& OutputSize       = CurrentOutput.PropertySizes[Property];

									OutputColor.SetNum(OutputSize.X * OutputSize.Y);

									if (Property.Type == MP_EmissiveColor)
									{
										// Only one thread will write to CurrentOutput.EmissiveScale since there can be only one emissive channel property per FBakeOutputEx
										FSKMaterialBakingModule::ProcessEmissiveOutput((const FFloat16Color*)Data, DataWidth, OutputSize, OutputColor, CurrentOutput.EmissiveScale);
									}
									else
									{
										TRACE_CPUPROFILER_EVENT_SCOPE(ConvertRawB8G8R8A8DataToFColor)
										
										check(StagingBuffer->GetFormat() == PF_B8G8R8A8);
										ConvertRawB8G8R8A8DataToFColor(OutputSize.X, OutputSize.Y, (uint8*)Data, DataWidth * sizeof(FColor), OutputColor.GetData());
									}

									// We can't unmap ourself since we're not on the render thread
									StagingBufferPool.ReleaseStagingBufferForUnmap_AnyThread(StagingBuffer);

									if (CurrentMaterialSettings->bPerformBorderSmear)
									{
										// This will resize the output to a single pixel if the result is monochrome.
										FMaterialBakingHelpers::PerformUVBorderSmearAndShrink(OutputColor, OutputSize.X, OutputSize.Y);
									}
#if WITH_EDITOR
									// If saving intermediates is turned on
									if (bSaveIntermediateTextures)
									{
										TRACE_CPUPROFILER_EVENT_SCOPE(SaveIntermediateTextures)
										FString TrimmedPropertyName = Property.ToString();

										const FString DirectoryPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() + TEXT("MaterialBaking/"));
										FString FilenameString = FString::Printf(TEXT("%s%s-%d-%s.bmp"), *DirectoryPath, *CurrentMaterialSettings->Material->GetName(), MaterialIndex, *TrimmedPropertyName);
										FFileHelper::CreateBitmap(*FilenameString, CurrentOutput.PropertySizes[Property].X, CurrentOutput.PropertySizes[Property].Y, CurrentOutput.PropertyData[Property].GetData());
									}
#endif // WITH_EDITOR
									NumTasks--;
								};

							// Run previous command if we're going to overwrite it meaning pipeline depth has been reached
							if (PipelineContext[PipelineIndex].ReadCommand)
							{
								PipelineContext[PipelineIndex].ReadCommand(RHICmdList);
							}

							// Generate a texture reading command that will be executed once it reaches the end of the pipeline
							PipelineContext[PipelineIndex].ReadCommand =
								[FinalProcessing_AnyThread, StagingBufferRef = MoveTemp(StagingBufferRef), GPUFence = MoveTemp(GPUFence)](FRHICommandListImmediate& RHICmdList) mutable
								{
									TRACE_CPUPROFILER_EVENT_SCOPE(MapAndEnqueue)

									void * Data = nullptr;
									int32 Width; int32 Height;
									RHICmdList.MapStagingSurface(StagingBufferRef, GPUFence.GetReference(), Data, Width, Height);

									// Schedule the copy and processing on another thread to free up the render thread as much as possible
									Async(
										EAsyncExecution::ThreadPool,
										[FinalProcessing_AnyThread, Data, Width, Height, StagingBufferRef = MoveTemp(StagingBufferRef)]() mutable
										{
											FinalProcessing_AnyThread(StagingBufferRef, Data, Width, Height);
										}
									);
								};
						}
					);

					PipelineIndex = (PipelineIndex + 1) % PipelineDepth;
				}
			}

		}

		// Destroying Render Items must happen on the render thread to ensure
		// they are not used anymore.
		ENQUEUE_RENDER_COMMAND(DestroyRenderItems)(
			[RenderItems](FRHICommandListImmediate& RHICmdList)
			{
				for (auto RenderItem : (*RenderItems))
				{
					delete RenderItem.Value;
				}

				delete RenderItems;
			}
		);
	}

	ENQUEUE_RENDER_COMMAND(ProcessRemainingReads)(
		[&PipelineContext, PipelineDepth, PipelineIndex](FRHICommandListImmediate& RHICmdList)
		{
			// Enqueue remaining reads
			for (int32 Index = 0; Index < PipelineDepth; Index++)
			{
				int32 LocalPipelineIndex = (PipelineIndex + Index) % PipelineDepth;

				if (PipelineContext[LocalPipelineIndex].ReadCommand)
				{
					PipelineContext[LocalPipelineIndex].ReadCommand(RHICmdList);
				}
			}
		}
	);

	// Wait until every tasks have been queued so that NumTasks is only decreasing
	FlushRenderingCommands();

	// Wait for any remaining final processing tasks
	while (NumTasks.Load(EMemoryOrder::Relaxed) > 0)
	{
		FPlatformProcess::Sleep(0.1f);
	}

	// Wait for all tasks to have been processed before clearing the staging buffers
	FlushRenderingCommands();

	ENQUEUE_RENDER_COMMAND(ClearStagingBufferPool)(
		[&StagingBufferPool](FRHICommandListImmediate& RHICmdList)
		{
			StagingBufferPool.Clear_RenderThread(RHICmdList);
		}
	);

	// Wait for StagingBufferPool clear to have executed before exiting the function
	FlushRenderingCommands();

	if (!CVarUseMaterialProxyCaching.GetValueOnAnyThread())
	{
		CleanupMaterialProxies();
	}
}

void FSKMaterialBakingModule::CleanupMaterialProxies()
{
	for (auto Iterator : MaterialProxyPool)
	{
		delete Iterator.Value.Value;
	}
	MaterialProxyPool.Reset();
}

UTextureRenderTarget2D* FSKMaterialBakingModule::CreateRenderTarget(bool bInForceLinearGamma, EPixelFormat InPixelFormat, const FIntPoint& InTargetSize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSKMaterialBakingModule::CreateRenderTarget)

	UTextureRenderTarget2D* RenderTarget = nullptr;
	const int32 MaxTextureSize = 1 << (MAX_TEXTURE_MIP_COUNT - 1); // Don't use GetMax2DTextureDimension() as this is for the RHI only.
	const FIntPoint ClampedTargetSize(FMath::Clamp(InTargetSize.X, 1, MaxTextureSize), FMath::Clamp(InTargetSize.Y, 1, MaxTextureSize));
	auto RenderTargetComparison = [bInForceLinearGamma, InPixelFormat, ClampedTargetSize](const UTextureRenderTarget2D* CompareRenderTarget) -> bool
	{
		return (CompareRenderTarget->SizeX == ClampedTargetSize.X && CompareRenderTarget->SizeY == ClampedTargetSize.Y && CompareRenderTarget->OverrideFormat == InPixelFormat && CompareRenderTarget->bForceLinearGamma == bInForceLinearGamma);
	};

	// Find any pooled render target with suitable properties.
	UTextureRenderTarget2D** FindResult = RenderTargetPool.FindByPredicate(RenderTargetComparison);
	
	if (FindResult)
	{
		RenderTarget = *FindResult;
	}
	else
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(CreateNewRenderTarget)

		// Not found - create a new one.
		RenderTarget = NewObject<UTextureRenderTarget2D>();
		check(RenderTarget);
		RenderTarget->AddToRoot();
		RenderTarget->ClearColor = FLinearColor(1.0f, 0.0f, 1.0f);
		RenderTarget->ClearColor.A = 1.0f;
		RenderTarget->TargetGamma = 0.0f;
		RenderTarget->InitCustomFormat(ClampedTargetSize.X, ClampedTargetSize.Y, InPixelFormat, bInForceLinearGamma);

		RenderTargetPool.Add(RenderTarget);
	}

	checkf(RenderTarget != nullptr, TEXT("Unable to create or find valid render target"));
	return RenderTarget;
}

FExportMaterialProxy* FSKMaterialBakingModule::CreateMaterialProxy(const FMaterialDataEx& MatSettings, const FMaterialPropertyEx& Property)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSKMaterialBakingModule::CreateMaterialProxy)

	FExportMaterialProxy* Proxy = nullptr;

	// Find all pooled material proxy matching this material
	TArray<FMaterialPoolValue> Entries;
	MaterialProxyPool.MultiFind(MatSettings.Material, Entries);

	// Look for the matching property
	for (FMaterialPoolValue& Entry : Entries)
	{
		if (Entry.Key == Property && Entry.Value->GetBlendMode() == MatSettings.BlendMode)
		{
			Proxy = Entry.Value;
			break;
		}
	}

	// Not found, create a new entry
	if (Proxy == nullptr)
	{
		Proxy = new FExportMaterialProxy(MatSettings.Material, Property.Type, Property.CustomOutput.ToString(), false /* bInSynchronousCompilation */, MatSettings.BlendMode);
		MaterialProxyPool.Add(MatSettings.Material, FMaterialPoolValue(Property, Proxy));
	}

	return Proxy;
}

void FSKMaterialBakingModule::ProcessEmissiveOutput(const FFloat16Color* Color16, int32 Color16Pitch, const FIntPoint& OutputSize, TArray<FColor>& OutputColor, float& EmissiveScale)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSKMaterialBakingModule::ProcessEmissiveOutput)

	const int32 NumThreads = [&]()
	{
		return FPlatformProcess::SupportsMultithreading() ? FPlatformMisc::NumberOfCores() : 1;
	}();

	float* MaxValue = new float[NumThreads];
	FMemory::Memset(MaxValue, 0, NumThreads * sizeof(MaxValue[0]));
	const int32 LinesPerThread = FMath::CeilToInt((float)OutputSize.Y / (float)NumThreads);

	// Find maximum float value across texture
	ParallelFor(NumThreads, [&Color16, LinesPerThread, MaxValue, OutputSize, Color16Pitch](int32 Index)
	{
		const int32 EndY = FMath::Min((Index + 1) * LinesPerThread, OutputSize.Y);			
		float& CurrentMaxValue = MaxValue[Index];
		const FFloat16Color MagentaFloat16 = FFloat16Color(FLinearColor(1.0f, 0.0f, 1.0f));
		for (int32 PixelY = Index * LinesPerThread; PixelY < EndY; ++PixelY)
		{
			const int32 SrcYOffset = PixelY * Color16Pitch;
			for (int32 PixelX = 0; PixelX < OutputSize.X; PixelX++)
			{
				const FFloat16Color& Pixel16 = Color16[PixelX + SrcYOffset];
				// Find maximum channel value across texture
				if (!(Pixel16 == MagentaFloat16))
				{
					CurrentMaxValue = FMath::Max(CurrentMaxValue, FMath::Max3(Pixel16.R.GetFloat(), Pixel16.G.GetFloat(), Pixel16.B.GetFloat()));
				}
			}
		}
	});

	const float GlobalMaxValue = [&MaxValue, NumThreads]
	{
		float TempValue = 0.0f;
		for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ++ThreadIndex)
		{
			TempValue = FMath::Max(TempValue, MaxValue[ThreadIndex]);
		}

		return TempValue;
	}();
		
	if (GlobalMaxValue <= 0.01f)
	{
		// Black emissive, drop it			
	}

	// Now convert Float16 to Color using the scale
	OutputColor.SetNumUninitialized(OutputSize.X * OutputSize.Y);
	const float Scale = 255.0f / GlobalMaxValue;
	ParallelFor(NumThreads, [&Color16, LinesPerThread, &OutputColor, OutputSize, Color16Pitch, Scale](int32 Index)
	{
		const FFloat16Color MagentaFloat16 = FFloat16Color(FLinearColor(1.0f, 0.0f, 1.0f));

		const int32 EndY = FMath::Min((Index + 1) * LinesPerThread, OutputSize.Y);
		for (int32 PixelY = Index * LinesPerThread; PixelY < EndY; ++PixelY)
		{
			const int32 SrcYOffset = PixelY * Color16Pitch;
			const int32 DstYOffset = PixelY * OutputSize.X;

			for (int32 PixelX = 0; PixelX < OutputSize.X; PixelX++)
			{
				const FFloat16Color& Pixel16 = Color16[PixelX + SrcYOffset];
				FColor& Pixel8 = OutputColor[PixelX + DstYOffset];

				if (Pixel16 == MagentaFloat16)
				{
					Pixel8.R = 255;
					Pixel8.G = 0;
					Pixel8.B = 255;
				}
				else
				{
					Pixel8.R = (uint8)FMath::RoundToInt(Pixel16.R.GetFloat() * Scale);
					Pixel8.G = (uint8)FMath::RoundToInt(Pixel16.G.GetFloat() * Scale);
					Pixel8.B = (uint8)FMath::RoundToInt(Pixel16.B.GetFloat() * Scale);
				}
					
				Pixel8.A = 255;
			}
		}
	});

	// This scale will be used in the proxy material to get the original range of emissive values outside of 0-1
	EmissiveScale = GlobalMaxValue;
}

void FSKMaterialBakingModule::OnObjectModified(UObject* Object)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSKMaterialBakingModule::OnObjectModified)

	if (CVarUseMaterialProxyCaching.GetValueOnAnyThread())
	{
		if (Object && Object->IsA<UMaterialInterface>())
		{
			UMaterialInterface* MaterialToInvalidate = Cast<UMaterialInterface>(Object);

			// Search our proxy pool for materials or material instances that refer to MaterialToInvalidate
			for (auto It = MaterialProxyPool.CreateIterator(); It; ++It)
			{
				TWeakObjectPtr<UMaterialInterface> PoolMaterialPtr = It.Key();

				// Remove stale entries from the pool
				bool bMustDelete = PoolMaterialPtr.IsValid();
				if (!bMustDelete)
				{
					bMustDelete = PoolMaterialPtr == MaterialToInvalidate;
				}

				// No match - Test the MaterialInstance hierarchy
				if (!bMustDelete)
				{
					UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(PoolMaterialPtr);
					while (!bMustDelete && MaterialInstance && MaterialInstance->Parent != nullptr)
					{
						bMustDelete = MaterialInstance->Parent == MaterialToInvalidate;
						MaterialInstance = Cast<UMaterialInstance>(MaterialInstance->Parent);
					}
				}

				// We have a match, remove the entry from our pool
				if (bMustDelete)
				{
					FExportMaterialProxy* Proxy = It.Value().Value;

					ENQUEUE_RENDER_COMMAND(DeleteCachedMaterialProxy)(
						[Proxy](FRHICommandListImmediate& RHICmdList)
						{
							delete Proxy;
						});

					It.RemoveCurrent();
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE //"MaterialBakingModule"
