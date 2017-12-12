## About

An Unreal Importer plugin for Unreal Engine. Compatible with 4.18 onwards. Utilizes the tinygltf library for loading in the GLTF 2.0 file format: https://github.com/syoyo/tinygltf

This is a WIP. There is a lot of source code commented out and still being worked on. No code is final at this stage.

## Installation

Please note that a more in-depth, beginner friendly, description (with screens shots and potentially a video) will be coming.

1. Create a new Unreal C++ Project.
1. Go to the location of the VS project file for your project.
1. Copy the unreal-exporter project to your plugins directory. This readme.md file should now be located in MyProject/Plugins/unreal-exporter/README.md (where MyProject would be the name of your Unreal Project you created in Step 1).
1. Compile your Project in VS using either the "DebugGame Editor" or "Development Editor" configurations.
1. Run your project
1. In the Unreal Editor go to the Content Browser at the bottom.
1. Select Import.
1. Choose a GLTF 2.0 file to loading (*.gltf).
1. Leave "Generate Unique Path Per Mesh" unchecked to load the file in as a single mesh. Uncheck to create a new asset per mesh.
1. Press Import

- "Apply World Transform To Geometry" is currently not finished. Please ignore for now.
- "Materials": This currently does nothing.

## WIP

Please note that this is a work in progress. The files are going to be refactored and possibly also renamed. 

## Current Issues

- Saving a scene will currently not save materials. Material package paths will be fixed soon. For now just load in files for testing, do not use the assets in any real scenes.
- Meshes, materials and textures are all currently loaded into the same current path location when importing. These may be moved into separate package paths later.

## What Is Currently Supported

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

## How To Debug In Visual Studio

1. In your VS Project add a folder under Plugins called "UGLTFPlugin"
1. Under that new folder add a Private and Public folder.
1. Add all the files from the unreal-exporter/source/UGLTFPlugin/Private to the private folder in VS.
1. Add all the files from the unreal-exporter/source/UGLTFPlugin/Public to the public folder in VS.
1. Make sure your configuration is set to DebugGame Editor.
1. Do a full recompile of your project.
1. Set a break point in the StaticMeshImporter.cpp -> FGLTFStaticMeshImporter::ImportStaticMesh(...) method.
1. Run the project and import a GLTF file to hit the break point.
