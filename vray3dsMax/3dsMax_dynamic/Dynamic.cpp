//#include "stdafx.h"
#include "dynamic.h"
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
    wcWeaveParameters *weave_parameters, VUtils::Color *diffuse_color,
	YarnType* yarn_type, int* yarn_type_id, int* yarn_hit)
{
    if(weave_parameters->pattern == 0){ //Invalid pattern
        *diffuse_color = VUtils::Color(1.f,1.f,0.f);
		*yarn_type = default_yarn_type;
		*yarn_type_id = 0;
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

    intersection_data.context = &sc;

    wcPatternData pattern_data = wcGetPatternData(intersection_data,
        weave_parameters);
	float specular_strength = 0;
	*yarn_hit = pattern_data.yarn_hit;
	if (pattern_data.yarn_hit) {
		*yarn_type_id = pattern_data.yarn_type;
		*yarn_type = weave_parameters->pattern->yarn_types[pattern_data.yarn_type];
		specular_strength = yarn_type_get_specular_strength(weave_parameters->
        pattern,pattern_data.yarn_type,&sc);
	}
    wcColor d = 
        wcEvalDiffuse( intersection_data, pattern_data, weave_parameters);
    float factor = (1.f - specular_strength);
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
    wcWeaveParameters *weave_parameters, VUtils::Matrix nm, const int yarn_hit,
    VUtils::Color *reflection_color)
{
    if(weave_parameters->pattern == 0 || !yarn_hit){ //Invalid pattern
		float s = 0.1f;
		reflection_color->r = s;
		reflection_color->g = s;
		reflection_color->b = s;
		return;
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

    intersection_data.context = &sc;

    wcPatternData pattern_data = wcGetPatternData(intersection_data,
        weave_parameters);
    float specular_strength = yarn_type_get_specular_strength(weave_parameters->
        pattern,pattern_data.yarn_type,&sc);
    float s = specular_strength *
        wcEvalSpecular(intersection_data, pattern_data, weave_parameters);
    reflection_color->r = s;
    reflection_color->g = s;
    reflection_color->b = s;
}

float wc_eval_texmap_mono(void *texmap, void *data)
{
	if(data){
		ShadeContext *sc=(ShadeContext*)data;
		Texmap *t=(Texmap*)texmap;
		return t->EvalMono(*sc);
	}
	return 1.f;
}

void wc_eval_texmap_color(void *texmap, void *data, float *col)
{
	if(data){
		ShadeContext *sc=(ShadeContext*)data;
		Texmap *t=(Texmap*)texmap;
		Point3 c=t->EvalColor(*sc);
		col[0]=c.x; col[1]=c.y; col[2]=c.z;
	} else{
		col[0]=1.f; col[1]=1.f; col[2]=1.f;
	}
}
