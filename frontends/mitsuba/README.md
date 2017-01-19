ThunderLoom plugin for Mitsuba
===
*ThunderLoom is a physically based shader for woven cloth with added flexibility for artists. https://github.com/vidarn/ThunderLoom*
*It's shading model is based on the work by Piti Irawan https://www.cs.cornell.edu/~srm/publications/IrawanThesis.pdf*

This plugin gives mitsuba the `thunderloom_mitsuba` bsdf which uses a weaving pattern given by a WIF file. Many types of patterns are freely available from sites such as http://handweaving.net.

Note: This plugin has limitations compared to the 3dsMax version. Individual control over different yarn types is currently not supported.

##Install

Binaries of the plugin need to be built against the version of mitsuba that is going to be used.
Under [releases](https://github.com/vidarn/ThunderLoom/releases) are libraries 
compiled against version 0.5.0 of mitsuba available at [mitsuba-renderer.org](mitsuba-renderer.org).


If the dynamic library file for your mitsuba build is available,
simply move the library file to the mitsuba `plugins` directory. 

Usually this means,

* For linux, move `thunderloom_mitsuba.so` into `path-to-mitsuba/plugins`
* For mac, move `thunderloom_mitsuba.dylib` into `/Applications/Mitsuba.app/plugins`
* For windows, move `thunderloom_mitsuba.dll` into `path-to-mitsuba/plugins`

Now the `thunderloom_mitsuba` bsdf should be avaiable the next time you start mitsuba. See the `example_scenes` folder for examples on how to use the bsdf.


#Building
In order to build the plugin, you first need the mitsuba source code and need to make sure that you are able to compile mitsuba itself. See https://www.mitsuba-renderer.org/releases/current/documentation.pdf for a guide on how to set up your system to compile Mitsuba. 

Once you are able to successfully compile mitsuba you can proceed.

* Option 1. Compile mitsuba with plugin
Make a symbolic link in the mitsuba bsdf source directory `ln -s path-to-ThunderLoom/frontends/mitsuba/src mitsubasrc/src/bsdfs/thunderloom`.
Next add the line `plugins += env.SharedLibrary('thunderloom_mitsuba', ['thunderloom/thunderloom_mitsuba.cpp'])` to `mitsubasrc/src/bsdfs/SConscript`. Then compile mitsuba as normal. 


The compiled mitsuba version will now have the bsdf.

* Option 2. Build only library

For linux and macOS, make sure that the mitsuba environment variables are set. 
```
source /path-to-mitsubasrc/setpath.sh
``` 

For windows, you will manually have to set an environment variable with,
```
MITSUBA_DIR=C:\Path-to-mitsubasrc\
```

Next simply run `scons` in the root directory for the mitsuba thunderLoom plugin (where this file is!).

This should compile a shared library and will be placed in a build folder.

(If there are problems, check `src/SConstruct` for troubling libraries, such as the MacOS.sdk which might not fit your system version.)

![monkey towel with a single light source](https://github.com/vidarn/ThunderLoom/raw/master/frontends/mitsuba/example_scenes/monkeytowel/towel_single_light.png) ![monkey towel with a image based lighting](https://github.com/vidarn/ThunderLoom/raw/master/frontends/mitsuba/example_scenes/monkeytowel/towel_envmap.png)
