ThunderLoom plugin for VRay standalone 3.x
===
(Under development!)

....

A BRDF plugin for the VRay standalone renderer. The plugin reads the parameters
from a .vrscene file. A section from an example file is shown below.

    Node pCubeShape1@node {
      transform=TransformHex("...");
      material=BRDFThunderLoom@mtlsingle;
      ...
    }

    MtlSingleBRDF BRDFThunderLoom@mtlsingle {
      brdf=BRDFThunderLoom;
      allow_negative_colors=1;
    }

    BRDFThunderLoom BRDFThunderLoom {
      filepath="/path/to/pattern/8452.wif";
      uscale=1;
      vscale=1;
      bend=0.5;
      yarnsize=1;
      twist=0.5;
      specular_strength=List(0, ramp1@color::alpha, checker1@color::alpha);
      specular_strength_on=List(0, 1, 1);
      specular_noise=checker2@color::alpha;
      highlight_width=0.4;
      diffuse_color=Color(0, 0.3, 0);
    }

All parameters except `filepath`, `uscale` and `vscale` can be assigned to
get their values from a texturemap, such as with `specular_noise` in the 
example.

When loading patterns that have multiple yarn types, again, all parameters 
except `filepath`, `uscale` and `vscale` can be set either with float values
or texturemaps on a per yarntype basis. This is done by specifiying a list, 
where item with index 0 is the value for the default/global yarn settings while 
item with index 1 applies to the first yarn type and so on. When making 
yarn type specific configurations, the user must indicate that the values should
be used, this is done via the `parametername_on` parameter. See 
`specular_strength` and `specular_strength_on` in the example where a pattern 
with 2 yarntypes is loaded.



