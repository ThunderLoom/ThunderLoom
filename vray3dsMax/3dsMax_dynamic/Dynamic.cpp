// Dynamic.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "dynamic.h"
#include "dbgprint.h"
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"
#include "shadedata_new.h"
#include "woven_cloth.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace VUtils;

void
#ifdef DYNAMIC
eval_diffuse
#else
EvalDiffuseFunc
#endif
(const VUtils::VRayContext &rc,
    wcWeaveParameters *weave_parameters, VUtils::Color *diffuse_color)
{
    if(weave_parameters->pattern_entry == 0){ //Invalid pattern
        *diffuse_color = VUtils::Color(1.f,1.f,0.f);
        return;
    }
    wcIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;
    intersection_data.wi_z = 1.f;

    wcPatternData pattern_data = wcGetPatternData(intersection_data,
        weave_parameters);
    wcColor d = 
        wcEvalDiffuse( intersection_data, pattern_data, weave_parameters);
    float factor = (1.f - weave_parameters->specular_strength);
    diffuse_color->r = factor*d.r;
    diffuse_color->g = factor*d.g;
    diffuse_color->b = factor*d.b;
}

void
#ifdef DYNAMIC
eval_specular
#else
EvalSpecularFunc
#endif
( const VUtils::VRayContext &rc, const VUtils::Vector &direction,
    wcWeaveParameters *weave_parameters, VUtils::Matrix nm,
    VUtils::Color *reflection_color)
{
    if(weave_parameters->pattern_entry == 0){ //Invalid pattern
        *reflection_color = VUtils::Color(0.f,0.f,1.f);
    }
    wcIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    //Convert the view and light directions to the correct coordinate system
    Point3 viewDir, lightDir;
    viewDir.x = -rc.rayparams.viewDir.x;
    viewDir.y = -rc.rayparams.viewDir.y;
    viewDir.z = -rc.rayparams.viewDir.z;
    viewDir = sc.VectorFrom(viewDir,REF_WORLD);
    viewDir = viewDir.Normalize();

    lightDir.x = direction.x;
    lightDir.y = direction.y;
    lightDir.z = direction.z;
    lightDir = sc.VectorFrom(lightDir,REF_WORLD);
    lightDir = lightDir.Normalize();

    // UVW derivatives
    Point3 dpdUVW[3];
    sc.DPdUVW(dpdUVW,1);

    Point3 n_vec = sc.Normal().Normalize();
    Point3 u_vec = dpdUVW[0].Normalize();
    Point3 v_vec = dpdUVW[1].Normalize();
    u_vec = v_vec ^ n_vec;
    v_vec = n_vec ^ u_vec;

    intersection_data.wi_x = DotProd(lightDir, u_vec);
    intersection_data.wi_y = DotProd(lightDir, v_vec);
    intersection_data.wi_z = DotProd(lightDir, n_vec);

    intersection_data.wo_x = DotProd(viewDir, u_vec);
    intersection_data.wo_y = DotProd(viewDir, v_vec);
    intersection_data.wo_z = DotProd(viewDir, n_vec);

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;

    wcPatternData pattern_data = wcGetPatternData(intersection_data,
        weave_parameters);
    float s = weave_parameters->specular_strength *
        wcEvalSpecular(intersection_data, pattern_data, weave_parameters);
    reflection_color->r = s;
    reflection_color->g = s;
    reflection_color->b = s;
}
