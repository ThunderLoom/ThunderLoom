ThunderLoom
===
![teaser8](https://cloud.githubusercontent.com/assets/116268/18617674/92d092c4-7dd5-11e6-9e2d-04d64c712c40.png)
*A physically based shader for woven cloth*

This projects consits of three main parts:

* Irawan shading model

At its core is an implementation of the 'Irawan' shading model, as presented in
the [phd thesis](http://www.cs.cornell.edu/~srm/publications/IrawanThesis.pdf)
by Piti Irawan and in the paper by 
[Irawan and Marschner (2012)](http://www.cs.cornell.edu/~srm/publications/TOG12-cloth.html).

* Library with API

The shading model is made accessible for rendering engines with added
flexibilty and artistic control through a library with a simple API. This 
allows the model and extra improvements to quickly be integrated into custom
rendering pipelines or created as plugins to more established renderers.

The library allows fast and flexible control over many parameters of how the
woven cloth is to behave. The actual pattern is determined using standard 
weaving-drafts defined in `WIF` files.
Many types of patterns are freely available from fine sites
such as [handweaving.net](http://handweaving.net).

* Frontends/plugins

Three plugins using this library have been made. V-Ray for 3dsMax, 
V-Ray for Maya and a limited plugin for Mitsuba, which includes a patch for the
`mtslbend` plugin for Blender. There is also a pattern editor
which can be used on its own to create or alter weaving patterns.
See the subfolders for these frontends for
more information. 

## Installing the frontends
Precompiled binaries can be found under the [releases](https://github.com/vidarn/ThunderLoom/releases) tab. The zip file
includes plugins for V-Ray for 3dsMax (2014/2015/2016/2017) and for the mitsuba renderer.

See the respective README files for specific install instructions.

* [plugin for V-Ray for 3dsMax](https://github.com/vidarn/ThunderLoom/tree/master/frontends/vray3dsMax)
* [plugin for V-Ray for Maya](https://github.com/vidarn/ThunderLoom/tree/master/frontends/vraymaya)
* [plugin for V-Ray Standalone](https://github.com/vidarn/ThunderLoom/tree/master/frontends/vray)
* [plugin for Mistuba](https://github.com/vidarn/ThunderLoom/tree/master/frontends/mitsuba)
* [patch for mtsblend](https://github.com/vidarn/ThunderLoom/tree/master/frontends/mitsuba/blender_mtsblend)

## Library
The source code for the library and core is found in the 'src' folder. 
The file 'thunderloom.h' describes the API of the shader.

For an example implementaion see [frontends/api_demo/](https://github.com/vidarn/ThunderLoom/tree/master/frontends/api_demo).

## Licence
The source code is released under the MIT Licence.

But if you happen to make something cool using this, a nice render or an 
improved tool, please let us know! We would love to see it! :)

---
[![3 pillars with different types of cloth rendered using thunderLoom in vray/3dsMax](https://cloud.githubusercontent.com/assets/116268/22116260/3cb6fe54-de70-11e6-9d68-91ce3ddc4cb8.png)](https://cloud.githubusercontent.com/assets/116268/22116160/eef3283c-de6f-11e6-9c51-76b47e08fd79.png)
