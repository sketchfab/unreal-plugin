#include "WindowsFileUtilityPrivatePCH.h"

class FWindowsFileUtility : public IWindowsFileUtility
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{

	}

	virtual void ShutdownModule() override
	{

	}
};

IMPLEMENT_MODULE(IWindowsFileUtility, WindowsFileUtility)