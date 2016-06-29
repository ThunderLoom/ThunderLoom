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

VUtils::Color dynamic_eval(const VUtils::VRayContext &rc, const Vector &direction,
                           VUtils::Color &lightColor, VUtils::Color &origLightColor,
                           float probLight, int flags,
                           wcWeaveParameters *weave_parameters, Matrix nm) {
    if(weave_parameters->pattern_entry == 0){
        //Invalid pattern
        return VUtils::Color(1.f,0.f,0.f);
    }
    wcIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    //Convertthe view and light directions to the correct coordinate system
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

    wcColor col = wcShade(intersection_data,weave_parameters);

    VUtils::Color ret(
        col.r * lightColor.r,
        col.g * lightColor.g,
        col.b * lightColor.b);
    return ret;
}
