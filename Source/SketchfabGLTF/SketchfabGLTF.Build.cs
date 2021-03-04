// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SketchfabGLTF : ModuleRules
	{
		public SketchfabGLTF(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
                );

			PrivateIncludePaths.AddRange(
				new string[] {
                    "SketchfabGLTF/Private",
					// ... add other private include paths required here ...
				}
                );

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "UnrealEd",
                    "InputCore",
                    "SlateCore",
                    "PropertyEditor",
                    "Slate",
                    "RawMesh",
                    "GeometryCache",
                    "MeshUtilities",
                    "RenderCore",
                    "RHI",
                    "MessageLog",
                    "JsonUtilities",
					"ZipUtility" //https://github.com/GameLogicDesign/ZipUtility-ue4
					// ... add other public dependencies that you statically link with here ...
				}
				);

			// DeveloperSettings was not a module until 4.26
			#if UE_4_26_OR_LATER
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"DeveloperSettings"
				}
			);
			#endif

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

            PrivatePCHHeaderFile = "Private/GLTFImporterPrivatePCH.h";
        }
	}
}
