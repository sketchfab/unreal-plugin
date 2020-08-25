#pragma once

#include "Engine.h"
#include "ImageLoader.generated.h"

// Forward declarations
class UTexture2D;

/**
Utility class for asynchronously loading an image into a texture.
Allows Blueprint scripts to request asynchronous loading of an image and be notified when loading is complete.
*/
UCLASS(BlueprintType)
class UImageLoader : public UObject
{
	GENERATED_BODY()

public:
	/**
	Loads an image file from disk into a texture on a worker thread. This will not block the calling thread.
	@return An image loader object with an OnLoadCompleted event that users can bind to, to get notified when loading is done.
	*/
	UFUNCTION(BlueprintCallable, Category = ImageLoader, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		static UImageLoader* LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath);

	/**
	Loads an image file from disk into a texture on a worker thread. This will not block the calling thread.
	@return A future object which will hold the image texture once loading is done.
	*/
	static TFuture<UTexture2D*> LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath, TFunction<void()> CompletionCallback);

	/**
	Loads an image file from disk into a texture. This will block the calling thread until completed.
	@return A texture created from the loaded image file.
	*/
	UFUNCTION(BlueprintCallable, Category = ImageLoader, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		static UTexture2D* LoadImageFromDisk(UObject* Outer, const FString& ImagePath);

public:
	/**
	Declare a broadcast-style delegate type, which is used for the load completed event.
	Dynamic multicast delegates are the only type of event delegates that Blueprint scripts can bind to.
	*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImageLoadCompleted, UTexture2D*, Texture);

	/** This accessor function allows C++ code to bind to the event. */
	FOnImageLoadCompleted& OnLoadCompleted()
	{
		return LoadCompleted;
	}

private:
	/** Helper function that initiates the loading operation and fires the event when loading is done. */
	void LoadImageAsync(UObject* Outer, const FString& ImagePath);

	/** Helper function to dynamically create a new texture from raw pixel data. */
	static UTexture2D* CreateTexture(UObject* Outer, const TArray64<uint8>& PixelData, int32 InSizeX, int32 InSizeY, EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8, FName BaseName = NAME_None);

private:
	/**
	Holds the load completed event delegate.
	Giving Blueprint access to this private variable allows Blueprint scripts to bind to the event.
	*/
	UPROPERTY(BlueprintAssignable, Category = ImageLoader, meta = (AllowPrivateAccess = true))
		FOnImageLoadCompleted LoadCompleted;

	/** Holds the future value which represents the asynchronous loading operation. */
	TFuture<UTexture2D*> Future;
};