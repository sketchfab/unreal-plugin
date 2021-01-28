# Sketchfab plugin for Unreal Engine

*Unreal plugin, based on [Sketchfab download API](https://sketchfab.com/developers/download-api), to browse and import models from Sketchfab*

Supported versions:

* [Release for 4.26](https://github.com/sketchfab/unreal-plugin/releases/tag/4.26-1.2.0)
* [Release for 4.25](https://github.com/sketchfab/unreal-plugin/releases/tag/4.25-1.2.0)
* [Release for 4.21](https://github.com/sketchfab/unreal-plugin/releases/tag/4.21-1.1.1)
* [Release for 4.20](https://github.com/sketchfab/unreal-plugin/releases/tag/4.20-1.1.1)
* [Release for 4.19](https://github.com/sketchfab/unreal-plugin/releases/tag/4.19-1.1.1)

**A Sketchfab account is required to be able to download content from Sketchfab**

⚠️ **This plugin is only available for Windows** ⚠️ 

### Installation

Download the `.zip` attached to the [release matching your UE version](https://github.com/sketchfab/unreal-plugin/releases) and unzip it into a new plugin folder in your project directory.

If you already have plugins installed, you only need to add a new `Sketchfab` directory, otherwise you will need to create `Plugins` folder manually.

You should end with this structure: `PROJECT_DIRECTORY/Plugins/Sketchfab/[Zip content]`
If Unreal Engine is running, you will need to close and reopen it in order to make it load the plugin.


### Import a Sketchfab model in UE4 project

Once loaded, the Sketchfab asset browser is available in `Window` menu in Sketchfab sub menu:

![Screenshot-0](https://user-images.githubusercontent.com/4066133/60019503-4cd41280-968e-11e9-8483-0c8a5e51d962.JPG)

When clicked, it opens a window and shows the latest free downloadable Sketchfab models.

#### Authentication
You first need to log in with your Sketchfab account in order to be able to download and import Sketchfab models. To do so, use the login button, enter your Sketchfab credentials, and proceed to authentication.

#### Asset Browser UI
Once logged-in, you will be able to browse free Sketchfab downloadable models from the plugin using search and filters, as well as models you have purchased in the [Sketchfab Store](https://sketchfab.com/store).

If you are a PRO user, you can also use the **My Models** filter to search and have a private access to your personal library of models, which includes all your published models (including the private ones).

![Screenshot-1](https://user-images.githubusercontent.com/52042414/104914481-7f8bdd00-598f-11eb-98d1-a01325dcab74.png)


#### Select and import model in UE4 project
Once authenticated, double click on a model thumbnail to display more information.

![Screenshot-2](https://user-images.githubusercontent.com/4066133/60019521-58bfd480-968e-11e9-902d-9fd3ae0247cf.JPG)

Click "Download model" to download the archive locally (a small progress bar under the thumbnail shows the status of the download). 
Then, drag and drop the thumbnail into the Content Browser to import it into your scene: a popup with license info is displayed and must be accepted before importing.

![Screenshot-3](https://user-images.githubusercontent.com/4066133/60019527-5cebf200-968e-11e9-89df-5ba86f8c75c8.JPG)
