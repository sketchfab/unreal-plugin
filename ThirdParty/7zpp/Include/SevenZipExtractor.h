#pragma once


#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ProgressCallback.h"


namespace SevenZip
{
	class SevenZipExtractor : public SevenZipArchive
	{
	public:

		SevenZipExtractor( const SevenZipLibrary& library, const TString& archivePath );
		virtual ~SevenZipExtractor();

		virtual bool ExtractArchive(const TString& directory, ProgressCallback* callback);
		virtual bool ExtractFilesFromArchive(const unsigned int* fileIndices, const unsigned int numberFiles, const TString& directory, ProgressCallback* callback);
	private:

		bool ExtractFilesFromArchive(const CComPtr< IStream >& archiveStream, const unsigned int* fileIndices, const unsigned int numberFiles, const TString& directory, ProgressCallback* callback);
	};
}
