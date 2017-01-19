ThunderLoom plugin for VRay for 3dsMax
===
*ThunderLoom is a physically based shader for woven cloth with added flexibility for artists. https://github.com/vidarn/ThunderLoom*
*It's shading model is based on the work by Piti Irawan https://www.cs.cornell.edu/~srm/publications/IrawanThesis.pdf*

This plugin adds the ThunderLoom material to the VRay renderer and 3dsMax.
This includes a user interface for controlling the material in the 3dsMax material editor
and a pattern editor. 

#Install
If the compiled material file (`thunderLoom.dlt`) is available, installation is simple.
Simply move the `.dlt` file to the `vrayplugins` folder in 3dsMax. 
Make sure that the `.dlt` file is meant for your version of 3dsMax!

The usual path for the plugins folder is `C:\Program Files\Autodesk\3dsMax (VERSION)\plugins\vrayplugins`.

#Building
In order to build the material plugin from source you will need...

* the 3dsMax SDK corresponding to your 3dsMax version
* vray version 3.x (for your 3dsMax version) installed, together with its SDK.
* Visual Studio 2010 (for 3dsMax 2014)
* Visual Studio 2012 (for 3dsMax versions later than 2014)

The Visual Studio Solution file `vray3dsMaxThunderLoom.sln` has
configurations for 3dsMax versions 2014, 2015 and 2016. It assumes default 
install locations for 3dsMax and VRay. 

Should you need to change these, such as compiling for a new 3dsMax version
or you have non-standard install paths, see section "VS Compile parameters".

To compile run `msbuild` with the configuration that fits your needs.

```
msbuild vray3dsMaxThunderLoom.sln /p:Configuration="max 2016 release" /p:Platform="x64"
```

The resulting `.dlt` file will in this case be put in `build\3dsMax2016\thunderLoom.dlt`.


##VS compile parameters
If you need to compile against a different 3dsMax version than is included
or you have non-standard install paths, the following build parameters in
Visual Studio should be checked that they fit.

Check the parameters by opening the `.snl` file in Visual studio 2012, 
right click `3dsMaxPlugin` and select properties.

The relevant paramters are under `Configuration Properties`.

* `C/C++ -> General -> Additional Include Directories`
* `Linker -> General -> Output File`
* `Linker -> General -> Additional Library Directories`
* `Linker -> Input -> Additional Dependencies`
