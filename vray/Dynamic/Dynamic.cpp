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
        return VUtils::Color(0.f,0.f,0.f);
    }
    wcIntersectionData intersection_data;
   
    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    Point3 uv = sc.UVW(1);

    //Convert wi and wo to the correct coordinate system

    //NOTE(Vidar): These are in world space...
    Point3 p, d;
    p.x = rc.rayparams.viewDir.x;
    p.y = rc.rayparams.viewDir.y;
    p.z = rc.rayparams.viewDir.z;
    p = sc.VectorFrom(p,REF_WORLD);
    p = p.Normalize();

    d.x = direction.x;
    d.y = direction.y;
    d.z = direction.z;
    d = sc.VectorFrom(d,REF_WORLD);
    d = d.Normalize();

    // UVW derivatives
    Point3 dpdUVW[3];
    sc.DPdUVW(dpdUVW,1);

    //TODO(Vidar): I'm not certain that we want these dot products...
    Point3 n_vec = sc.Normal().Normalize();
    Point3 u_vec = dpdUVW[0].Normalize();
    Point3 v_vec = dpdUVW[1].Normalize();
    //u_vec = v_vec ^ n_vec;

    Matrix3 mat(u_vec, v_vec, n_vec, Point3(0.f,0.f,0.f));
    mat.Invert(); // TODO(Vidar) transposing would be better...

    Point3 localViewDir = (p * mat).Normalize();
    Point3 localDir     = (d * mat).Normalize();

    VUtils::Vector wo;
    wo.x = localViewDir.x;
    wo.y = localViewDir.y;
    wo.z = localViewDir.z;
    VUtils::Vector wi;
    wi.x = localDir.x;
    wi.y = localDir.y;
    wi.z = localDir.z;

    intersection_data.wi_x = wi.x;
    intersection_data.wi_y = wi.y;
    intersection_data.wi_z = wi.z;

    intersection_data.wo_x = wo.x;
    intersection_data.wo_y = wo.y;
    intersection_data.wo_z = wo.z;

    intersection_data.uv_x = uv.x;
    intersection_data.uv_y = uv.y;


    wcPatternData dat = wcGetPatternData(intersection_data,weave_parameters);

    float reflection = wcEvalSpecular(intersection_data,dat,weave_parameters);

    float specular = weave_parameters->specular_strength;
    float diffuse = (1.f - specular) * wcEvalDiffuse(intersection_data,dat,weave_parameters);

    VUtils::Vector v;
    v.x = (specular*reflection + diffuse*dat.color_r) * lightColor.r;
    v.y = (specular*reflection + diffuse*dat.color_g) * lightColor.g;
    v.z = (specular*reflection + diffuse*dat.color_b) * lightColor.b;

    VUtils::Color res(fabsf(v.x), fabsf(v.y), fabsf(v.z));
    return res;
}
