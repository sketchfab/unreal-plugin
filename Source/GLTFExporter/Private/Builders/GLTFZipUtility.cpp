// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFZipUtility.h"

#include <fstream>
#include <vector>

THIRD_PARTY_INCLUDES_START
#include "ThirdParty/zlib/zlib-1.2.5/Inc/zlib.h"
#if _WIN32 // Minizip is part of Windows UE zlib distribution
#include "ThirdParty/zlib/zlib-1.2.5/Src/contrib/minizip/unzip.h"
#include "ThirdParty/zlib/zlib-1.2.5/Src/contrib/minizip/zip.h"
#elif __APPLE__ || __linux__ // linux and mac, use minizip headers we packaged
#include "minizip/unzip.h"
#include "minizip/zip.h"
#endif
THIRD_PARTY_INCLUDES_END

TArray<FString> FGLTFZipUtility::GetAllFiles(const FString& ArchiveFile)
{
	const unzFile ZipHandle = unzOpen64(TCHAR_TO_UTF8(*ArchiveFile));
	if (ZipHandle == nullptr)
	{
		return {};
	}

	if (unzGoToFirstFile(ZipHandle) != UNZ_OK)
	{
		unzClose(ZipHandle);
		return {};
	}

	TArray<FString> Filenames;

	do
	{
		const FString Filename = GetCurrentFilename(ZipHandle);
		if (Filename.IsEmpty())
		{
			unzClose(ZipHandle);
			return {};
		}

		Filenames.Add(Filename);
	} while (unzGoToNextFile(ZipHandle) == UNZ_OK);

	unzClose(ZipHandle);
	return Filenames;
}

bool FGLTFZipUtility::ExtractAllFiles(const FString& ArchiveFile, const FString& TargetDirectory)
{
	const unzFile ZipHandle = unzOpen64(TCHAR_TO_UTF8(*ArchiveFile));
	if (ZipHandle == nullptr)
	{
		return false;
	}

	if (unzGoToFirstFile(ZipHandle) != UNZ_OK)
	{
		unzClose(ZipHandle);
		return false;
	}

	do
	{
		const FString Filename = GetCurrentFilename(ZipHandle);
		if (Filename.IsEmpty())
		{
			unzClose(ZipHandle);
			return false;
		}

		if (!ExtractCurrentFile(ZipHandle, TargetDirectory / Filename))
		{
			unzClose(ZipHandle);
			return false;
		}
	} while (unzGoToNextFile(ZipHandle) == UNZ_OK);

	unzClose(ZipHandle);
	return true;
}

bool FGLTFZipUtility::ExtractOneFile(const FString& ArchiveFile, const FString& FileToExtract, const FString& TargetDirectory)
{
	const unzFile ZipHandle = unzOpen64(TCHAR_TO_UTF8(*ArchiveFile));
	if (ZipHandle == nullptr)
	{
		return false;
	}

	if (unzGoToFirstFile(ZipHandle) != UNZ_OK)
	{
		unzClose(ZipHandle);
		return false;
	}

	if (unzLocateFile(ZipHandle, TCHAR_TO_UTF8(*FileToExtract), 0) != UNZ_OK)
	{
		unzClose(ZipHandle);
		return false;
	}

	if (!ExtractCurrentFile(ZipHandle, TargetDirectory / FileToExtract))
	{
		unzClose(ZipHandle);
		return false;
	}

	unzClose(ZipHandle);
	return true;
}

FString FGLTFZipUtility::GetCurrentFilename(void* ZipHandle)
{
	unz_file_info64 FileInfo;
	if (unzGetCurrentFileInfo64(ZipHandle, &FileInfo, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK)
	{
		return {};
	}

	TArray<char> Filename;
	Filename.Init('\0', FileInfo.size_filename + 1);

	if (unzGetCurrentFileInfo64(ZipHandle, nullptr, Filename.GetData(), FileInfo.size_filename, nullptr, 0, nullptr, 0) != UNZ_OK)
	{
		return {};
	}

	return UTF8_TO_TCHAR(Filename.GetData());
}

bool FGLTFZipUtility::ExtractCurrentFile(void* ZipHandle, const FString& TargetFile)
{
	const FString Filename = GetCurrentFilename(ZipHandle);
	if (Filename.IsEmpty())
	{
		return false;
	}

	if (TargetFile.EndsWith(TEXT("/")) || TargetFile.EndsWith(TEXT("\\")))
	{
		IFileManager::Get().MakeDirectory(*TargetFile, true);
	}
	else
	{
		FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*TargetFile);
		if (FileWriter == nullptr)
		{
			return false;
		}

		if (unzOpenCurrentFile(ZipHandle) != UNZ_OK)
		{
			return false;
		}

		uint8 ReadBuffer[8192];
		while (const int ReadSize = unzReadCurrentFile(ZipHandle, ReadBuffer, sizeof(ReadBuffer)))
		{
			if (ReadSize < 0)
			{
				unzCloseCurrentFile(ZipHandle);
				return false;
			}

			FileWriter->Serialize(ReadBuffer, ReadSize);
		}

		if (unzCloseCurrentFile(ZipHandle) != UNZ_OK)
		{
			return false;
		}

		FileWriter->Close();
	}

	return true;
}

int FGLTFZipUtility::CompressDirectory(const FString& ArchiveFile, const FString& TargetDirectory)
{
	// From https://stackoverflow.com/questions/11370908/how-do-i-use-minizip-on-zlib
	std::string archive = std::string(TCHAR_TO_UTF8(*ArchiveFile));
	std::string fileToZip = std::string(TCHAR_TO_UTF8(*TargetDirectory));

	zipFile zf = zipOpen64(archive.c_str(), APPEND_STATUS_CREATE);
	if (zf == NULL)
		return 1;

	bool _return = true;

	std::fstream file(fileToZip, std::ios::binary | std::ios::in);
	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		long size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		if (size == 0 || file.read(&buffer[0], size))
		{
			zip_fileinfo zfi = {{ 0 }};
			std::string fileName = fileToZip.substr(fileToZip.rfind('\\') + 1);

			if (0 == zipOpenNewFileInZip(zf, std::string(fileName.begin(), fileName.end()).c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
			{
				if (zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], size))
					_return = false;

				if (zipCloseFileInZip(zf))
					_return = false;

				file.close();
			}
		}
		file.close();
	}
	//_return = false;

	if (zipClose(zf, NULL))
		return 3;

	/*if (!_return)
		return 4;
	*/
	return 0;

}

