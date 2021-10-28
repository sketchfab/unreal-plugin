#include "ImageLoader.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

#include "Modules/ModuleManager.h"
#include "Async/Async.h"
#include "Async/Future.h"
#include "Misc/FileHelper.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

// Change the UE_LOG log category name below to whichever log category you want to use.
#define UIL_LOG(Verbosity, Format, ...)	UE_LOG(LogTemp, Verbosity, Format, __VA_ARGS__)

// Module loading is not allowed outside of the main thread, so we load the ImageWrapper module ahead of time.
static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

UImageLoader* UImageLoader::LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath)
{
	// This simply creates a new ImageLoader object and starts an asynchronous load.
	UImageLoader* Loader = NewObject<UImageLoader>();
	Loader->LoadImageAsync(Outer, ImagePath);
	return Loader;
}

void UImageLoader::LoadImageAsync(UObject* Outer, const FString& ImagePath)
{
	// The asynchronous loading operation is represented by a Future, which will contain the result value once the operation is done.
	// We store the Future in this object, so we can retrieve the result value in the completion callback below.
	Future = LoadImageFromDiskAsync(Outer, ImagePath, [this]()
	{
		// This is the same Future object that we assigned above, but later in time.
		// At this point, loading is done and the Future contains a value.
		if (Future.IsValid())
		{
			// Notify listeners about the loaded texture on the game thread.
			AsyncTask(ENamedThreads::GameThread, [this]() { LoadCompleted.Broadcast(Future.Get()); });
		}
	});
}

TFuture<UTexture2D*> UImageLoader::LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath, TFunction<void()> CompletionCallback)
{
	// Run the image loading function asynchronously through a lambda expression, capturing the ImagePath string by value.
	// Run it on the thread pool, so we can load multiple images simultaneously without interrupting other tasks.
	return Async(EAsyncExecution::ThreadPool, [=]() { return LoadImageFromDisk(Outer, ImagePath); }, CompletionCallback);
}

UTexture2D* UImageLoader::LoadImageFromDisk(UObject* Outer, const FString& ImagePath)
{
	// Check if the file exists first
	if (!FPaths::FileExists(ImagePath))
	{
		UIL_LOG(Error, TEXT("File not found: %s"), *ImagePath);
		return nullptr;
	}

	// Load the compressed byte data from the file
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
	{
		UIL_LOG(Error, TEXT("Failed to load file: %s"), *ImagePath);
		return nullptr;
	}

	// Detect the image type using the ImageWrapper module
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		UIL_LOG(Error, TEXT("Unrecognized image file format: %s"), *ImagePath);
		return nullptr;
	}

	// Create an image wrapper for the detected image format
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid())
	{
		UIL_LOG(Error, TEXT("Failed to create image wrapper for file: %s"), *ImagePath);
		return nullptr;
	}

	// Decompress the image data
	TArray64<uint8> RawData;
	ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num());
	bool success = ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
	if (!success)
	{
		UIL_LOG(Error, TEXT("Failed to decompress image file: %s"), *ImagePath);
		return nullptr;
	}

	// Create the texture and upload the uncompressed image data
	FString TextureBaseName = TEXT("Texture_") + FPaths::GetBaseFilename(ImagePath);
	return CreateTexture(Outer, RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8, FName(*TextureBaseName));
}

UTexture2D* UImageLoader::CreateTexture(UObject* Outer, const TArray64<uint8>& PixelData, int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, FName BaseName)
{
	// Shamelessly copied from UTexture2D::CreateTransient with a few modifications
	if (InSizeX <= 0 || InSizeY <= 0 ||
		(InSizeX % GPixelFormats[InFormat].BlockSizeX) != 0 ||
		(InSizeY % GPixelFormats[InFormat].BlockSizeY) != 0)
	{
		UIL_LOG(Warning, TEXT("Invalid parameters specified for UImageLoader::CreateTexture()"), nullptr);
		return nullptr;
	}

	// Most important difference with UTexture2D::CreateTransient: we provide the new texture with a name and an owner
	FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
	//UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);
	UTexture2D* NewTexture = NewObject<UTexture2D>(); //KB: Changed, may cause a leek

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = InSizeX;
	NewTexture->PlatformData->SizeY = InSizeY;
	NewTexture->PlatformData->PixelFormat = InFormat;

	// Allocate first mipmap and upload the pixel data
	int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
	int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = InSizeX;
	Mip->SizeY = InSizeY;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* TextureData = Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
	FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num());
	Mip->BulkData.Unlock();

	NewTexture->UpdateResource();
	return NewTexture;
}