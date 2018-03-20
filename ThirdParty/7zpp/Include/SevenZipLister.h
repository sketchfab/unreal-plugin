#pragma once

#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ListCallback.h"


namespace SevenZip
{
	class SevenZipLister : public SevenZipArchive
	{
	public:
		TString m_archivePath;

		SevenZipLister( const SevenZipLibrary& library, const TString& archivePath );
		virtual ~SevenZipLister();

		virtual bool ListArchive(ListCallback* callback);

	private:
		bool ListArchive(const CComPtr< IStream >& archiveStream, ListCallback* callback);
	};
}
