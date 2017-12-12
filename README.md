## About

An Unreal Importer plugin for Unreal Engine. Compatible with 4.18 onwards. Utilizes the tinygltf library for loading in the GLTF 2.0 file format: https://github.com/syoyo/tinygltf

This is a WIP. There is a lot of source code commented out and still being worked on. No code is final at this stage.

## Installation

1. Open the Unreal Editor
1. Choose New Project
1. Choose one of the example project types, such as "First Person". You can use either BluePrint or C++, it does not matter.
1. Choose the location and name of your project
1. Press Create Project.
1. On your computer go to the location when your project was created
1. In your project folder create a new folder called "Plugins".
1. Download this unreal-exporter project form github and extract into this folder
1. You should now have a folder structure which looks something like "MyProject/Plugins/unreal-exporter/..."
1. Shut down the Unreal Editor if its still running
1. Double click the MyProject.uproject file to run Unreal Editor again. ("MyProject/MyProject.uproject")

## How To Import A GLTF File

1. In the Unreal Editor go to the Content Browser at the bottom.
1. Select Import.
1. Choose a GLTF 2.0 file to loading (*.gltf).
1. Leave "Generate Unique Path Per Mesh" unchecked to load the file in as a single mesh. Uncheck to create a new asset per mesh.
1. Press Import

- "Apply World Transform To Geometry" is currently not finished. Please ignore for now.
- "Materials": This currently does nothing.

## Shipping The PreBuilt Plugin

If you wish to give this plugin to someone to use, but not pass on the source code, then just make a copy of this project and remove the "Source" folder. The plugin will still work as normal without this folder.

## What Is Currently Supported

- Only imports the first scene found in the glTF file
- Loading in a GLTF file as a static mesh
- Loading in a GLTF file where every mesh becomes its own static mesh
- PBR materials
- Basic support for Specular/Glossy workflows
- Only supports ascii *.gltf files currently. Binary support coming soon.
- Only supports external textures (ie does not support textures in binary files). Embedded texture support coming soon.
- No sampler support yet (ie uv tiling). This will be coming soon, which is why its listed here.

## Whats Not Supported And Will Not Be In Version 1.0

- No cameras
- No animation
- No skins
- No joints
- No morph targets

## How To Use In Visual Studio

1. In your VS Project add a folder under Plugins called "UGLTFPlugin"
1. Under that new folder add a Private and Public folder.
1. Add all the files from the unreal-exporter/source/UGLTFPlugin/Private to the private folder in VS.
1. Add all the files from the unreal-exporter/source/UGLTFPlugin/Public to the public folder in VS.
1. Make sure your configuration is set to DebugGame Editor.
1. Do a full recompile of your project.
1. Set a break point in the StaticMeshImporter.cpp -> FGLTFStaticMeshImporter::ImportStaticMesh(...) method.
1. Run the project and import a GLTF file to hit the break point.

