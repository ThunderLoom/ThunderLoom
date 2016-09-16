ThunderLoom plugin for Mitsuba
===
*ThunderLoom is a physically based shader for woven cloth with added flexibility for artists. https://github.com/vidarn/ThunderLoom*
*It's shading model is based on the work by Piti Irawan https://www.cs.cornell.edu/~srm/publications/IrawanThesis.pdf*

This plugin gives mitsuba the `thunderloom` bsdf which uses a weaving pattern given by a WIF file. Many types of patterns are freely available from sites such as http://handweaving.net.

Note: This plugin has limitations compared to the 3dsMax version. Individual control over different yarn types is currently not supported.

#Install
If the compiled dynamic library file is available, installation is simple.
Simply move the library file to the mitsuba `plugins` directory. 

Usually this means,

* For linux, move `thunderloom_mitsuba.so` into `path-to-mitsuba/plugins`
* For mac, move `thunderloom_mitsuba.dylib` into `/Applications/Mitsuba.app/plugins`
* For windows, move `thunderloom_mitsuba.dll` into `/Applications/Mitsuba.app/plugins`

Now the `thunderloom` bsdf should be avaiable the next time you start mitsuba. See the `example_scenes` folder for examples on how to use the bsdf.

#Building
In order to build the plugin, you first need the mitsuba source code and need to make sure that you are able to compile mitsuba itself. See https://www.mitsuba-renderer.org/releases/current/documentation.pdf for a guide on how to set up your system to compile Mitsuba. 

Once you are able to succesfully compile mitsuba you can proceed.


For linux and macOS, make sure that the mitsuba environment variables are set. 
```
source /path-to-mitsubasrc/setpath.sh
``` 

For windows, you will manually have to set an environment variable with,
```
MITSUBA_DIR=C:\Path-to-mitsubasrc\
```

Next simply run `scons` in the root directory for the mitsuba thunderLoom plugin (where this file is!).

This should compile a shared library and will be plased in a build folder.