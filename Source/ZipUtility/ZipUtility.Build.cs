// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ZipUtility : ModuleRules
{
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/")); }
    }
    private string SevenZppPath
    {
        get { return Path.GetFullPath(Path.Combine(ThirdPartyPath, "7zpp")); }
    }

	private string ATLPath
	{
		get { return "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.26.28801/atlmfc"; }
	}

	public ZipUtility(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                "ZipUtility/Private",
                Path.Combine(SevenZppPath, "Include"),
				Path.Combine(ATLPath, "include"),
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Engine",
                "WindowsFileUtility"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Projects"
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        PrivatePCHHeaderFile = "Private/ZipUtilityPrivatePCH.h";

        LoadLib(Target);
    }
    public bool LoadLib(ReadOnlyTargetRules Target)
    {
        bool isLibrarySupported = false;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;

            string PlatformSubPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "Win64" : "Win32";
            string LibrariesPath = Path.Combine(SevenZppPath, "Lib");
            string DLLPath = Path.Combine(SevenZppPath, "dll");

            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, PlatformSubPath, "atls.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, PlatformSubPath, "7zpp_u.lib"));

            PublicDelayLoadDLLs.Add("7z.dll");
            RuntimeDependencies.Add(Path.Combine(DLLPath, PlatformSubPath, "7z.dll"));
        }

        if (isLibrarySupported)
        {
            // Include path
            //PublicIncludePaths.Add(Path.Combine(SevenZppPath, "Include"));
        }

        return isLibrarySupported;
    }
}
