#pragma once


#include "SevenZipLibrary.h"
#include "CompressionFormat.h"


namespace SevenZip
{
	class ListCallback
	{
	public:
		/*
		Called for each file found in the archive. Size in bytes.
		*/
		virtual void OnFileFound(const TString& archivePath, const TString& filePath, int size) {}

		/*
		Called when all the files have been listed
		*/
		virtual void OnListingDone(const TString& archivePath) {}
	};
}
