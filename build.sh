#!/bin/bash
# Useful to keep the plugin code out of the project source
# --from /c/.../MyProject copy plugin Binaries from a project
# --to /c/.../MyProject   replace the existing "Sketchfab" plugin from the project
# no argument:            build a zip in releases/, without the sources, but with Binaries
#
# On WSL for instance:
# ./build.sh --to   "/mnt/c/Users/loicn/Documents/Unreal Projects/MyProject"
# ./build.sh --from "/mnt/c/Users/loicn/Documents/Unreal Projects/MyProject"
# ./build.sh

# If requested, apply the patch on Khronos' submodule (glTF-Blender-IO/)
if [[ $* == *--to* ]]
then
	echo "Installing plugin to $2"
	# Create a directory for the plugin
	project_plugin=$2/Plugins/Sketchfab
	rm -rf "$project_plugin"
	mkdir -p "$project_plugin"
	# Copy the files to the directory
	cp -r Source/ Content/ Resources/ ThirdParty/ Sketchfab.uplugin ZipUtility_License.txt "$project_plugin"
elif [[ $* == *--from* ]]
then
	echo "Updating repository from $2"
	# Create a directory for the plugin
	project_plugin=$2/Plugins/Sketchfab
	# Copy the Binaries from the plugin
	rm Binaries/WIN64/*
	cp "$project_plugin"/Binaries/win64/*.dll             ./Binaries/Win64/
	cp "$project_plugin"/Binaries/win64/UE4Editor.modules ./Binaries/Win64/
else
	version=$(cat Sketchfab.uplugin | grep -o -P '(?<="VersionName" : ").*(?=",)' | sed 's/\./-/g')
	echo "Creating releases for version $version"
	# Create a releases/ directory and copy plugin files
	release_dir="releases/SketchfabUnrealPlugin-$version"
	mkdir -p $release_dir
	cp -r Binaries/ Resources/ Content/ ThirdParty/ Sketchfab.uplugin ZipUtility_License.txt $release_dir
	# Zip everything
	cd releases/
	zip -r -q SketchfabUnrealPlugin-$version.zip SketchfabUnrealPlugin-$version/
	cd -
	echo "Releases available in $(pwd)/releases/"
fi