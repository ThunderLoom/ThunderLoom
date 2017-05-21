ThunderLoom plugin V-Ray for Maya
===
This plugin adds the ThunderLoom material to V-Ray and registers a material 
node in Maya. 

# Installing
* Install the V-Ray plugin. Either move the vray plugin `vray_thunderloom.dll` 
to /mayainstall/vray/vrayplugins/ or add the path to the containing folder to 
the environment variable `VRAY_FOR_MAYAnnnn_PLUGINS_x64`. Where `nnnn` is the
Maya version (2011, 2012 etc).
* Copy the V-Ray shader translation file `vraythunderloommtl.txt` to
/mayainstall/vray/shaders/.
* Change the module file `ThunderLoom.mod` to point to the module
`thunderloom_maya_module` folder. Copy the file to /mayainstall/modules/

The plugin should now show up in the Maya plug-ins manager.

# Building
Follow the instructions at http://help.autodesk.com/cloudhelp/2017/ENU/Maya-SDK/files/Setting_up_your_build_environment.htm
for your platform to setup the Maya SDK.

In order to build the material plugin from source you will need...

* Maya and the Maya SDK corresponding to your 3dsMax version
* V-Ray version 3.x (for your Maya version) installed, together with its SDK.
* Visual Studio 2012

The Visual Studio Solution file `targets/mcvs/VRayMayaThunderLoom.sln` has
configurations for Maya versions 2015, 2016 and 2017. It assumes default 
install locations for 3dsMax and V-Ray. 

Should you need to change these, such as compiling for a new 3dsMax version
or you have non-standard install paths, see section "VS Compile parameters".

To compile open the solution file in Visual Studio 2012, set the 
configuration for the two included projects `VRayStandaloneThunderLoom` and 
`VRayMayaThunderLoom`. This will produce a dll for V-Ray in 
`../frontends/vray/build/x64/V-Ray for Maya20xx/vray_thunderloom.dll` and a 
mll file in `build/x64/Maya20xx/VRayMayaThunderLoom.mll`.

## VS compile parameters
If you need to compile against a different Maya version than is included
or you have non-standard install paths, the following build parameters in
Visual Studio should be checked that they fit.

Check the parameters by opening the `.snl` file in Visual studio 2012, 
right click the included projects and and select properties.

The relevant paramters are under `Configuration Properties`.

* `C/C++ -> General -> Additional Include Directories`
* `Linker -> General -> Output File`
* `Linker -> General -> Additional Library Directories`
* `Linker -> Input -> Additional Dependencies`
