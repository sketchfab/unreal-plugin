#pragma once

#include "ListCallback.h"
#include "ProgressCallback.h"

#include "PreWindowsApi.h"
#include "AllowWindowsPlatformTypes.h"
#include "AllowWindowsPlatformAtomics.h"

#pragma warning(push)
#pragma warning(disable: 4191)
#pragma warning(disable: 4996)

#ifndef DeleteFile
#define DeleteFile DeleteFileW
#endif
#ifndef MoveFile
#define MoveFile MoveFileW
#endif

#ifndef LoadString
#define LoadString LoadStringW
#endif

#ifndef GetMessage
#define GetMessage GetMessageW
#endif

#include <atlbase.h>
#include <sphelper.h>

#undef DeleteFile
#undef MoveFile

#include "SevenZipCompressor.h"
#include "SevenZipExtractor.h"
#include "SevenZipLister.h"

#include "HideWindowsPlatformAtomics.h"
#include "HideWindowsPlatformTypes.h"
#include "PostWindowsApi.h"
#pragma warning(pop)

// Version of this library
#define SEVENZIP_VERSION L"0.2.0-20160117.1"
#define SEVENZIP_BRANCH L"master"
