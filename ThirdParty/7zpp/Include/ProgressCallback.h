#pragma once


#include "SevenZipLibrary.h"
#include "CompressionFormat.h"


namespace SevenZip
{
	class ProgressCallback
	{
	public:

		/*
		Called at beginning
		*/
		virtual void OnStartWithTotal(const TString& archivePath, unsigned __int64 totalBytes) {}

		/*
		Called Whenever progress has updated with a bytes complete
		*/
		virtual void OnProgress(const TString& archivePath, unsigned __int64 bytesCompleted) {}


		/*
		Called When progress has reached 100%
		*/
		virtual void OnDone(const TString& archivePath) {}

		/*
		Called When single file progress has reached 100%, returns the filepath that completed
		*/
		virtual void OnFileDone(const TString& archivePath, const TString& filePath, unsigned __int64 bytesCompleted) {}
	};
}
