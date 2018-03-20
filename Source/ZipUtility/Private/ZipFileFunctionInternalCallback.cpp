#include "ZipUtilityPrivatePCH.h"
#include "ZipFileFunctionInternalCallback.h"

UZipFileFunctionInternalCallback::UZipFileFunctionInternalCallback()
{
	CompressionFormat = ZipUtilityCompressionFormat::COMPRESSION_FORMAT_UNKNOWN;
	DestinationFolder = FString();
	File = FString();
	FileIndex = 0;
	Callback = NULL;
}

void UZipFileFunctionInternalCallback::OnFileFound_Implementation(const FString& archive, const FString& fileIn, int32 size)
{
	if (!bFileFound && fileIn.ToLower().Contains(File.ToLower()))
	{
		TArray<int32> FileIndices = { FileIndex };
		
		if (bUnzipto)
		{
			UZipFileFunctionLibrary::UnzipFilesTo(FileIndices, archive, DestinationFolder, Callback, CompressionFormat);
		}
		else
		{
			UZipFileFunctionLibrary::UnzipFiles(FileIndices, archive, Callback, CompressionFormat);
		}

		if (bSingleFile)
			bFileFound = true;
	}

	FileIndex++;
}

void UZipFileFunctionInternalCallback::SetCallback(const FString& FileName, UObject* CallbackIn, TEnumAsByte<ZipUtilityCompressionFormat> CompressionFormatIn /*= ZipUtilityCompressionFormat::COMPRESSION_FORMAT_UNKNOWN*/)
{
	File = FileName;
	Callback = CallbackIn;
	CompressionFormat = CompressionFormatIn;
	FileIndex = 0;
}

void UZipFileFunctionInternalCallback::SetCallback(const FString& FileName, const FString& DestinationFolderIn, UObject* CallbackIn, TEnumAsByte<ZipUtilityCompressionFormat> CompressionFormatIn /*= ZipUtilityCompressionFormat::COMPRESSION_FORMAT_UNKNOWN*/)
{
	SetCallback(FileName, CallbackIn, CompressionFormatIn);

	bUnzipto = true;
	DestinationFolder = DestinationFolderIn;
}