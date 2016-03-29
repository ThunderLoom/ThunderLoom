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
#include "weave.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace VUtils;

float dot(Vector a, Vector b){
    return a*b;
}

Vector cross(Vector a, Vector b){
    return a ^ b;
}

void calculateLengthOfSegment(bool warp_above, uint32_t pattern_x,
                uint32_t pattern_y, uint32_t *steps_left,
                uint32_t *steps_right,  uint32_t pattern_width,
                uint32_t pattern_height, PaletteEntry *pattern_entry)
{

    uint32_t current_x = pattern_x;
    uint32_t current_y = pattern_y;
    uint32_t *incremented_coord = warp_above ? &current_y : &current_x;
    uint32_t max_size = warp_above ? pattern_height: pattern_width;
    uint32_t initial_coord = warp_above ? pattern_y: pattern_x;
    *steps_right = 0;
    *steps_left  = 0;
    do{
        (*incremented_coord)++;
        if(*incremented_coord == max_size){
            *incremented_coord = 0;
        }
        if((bool)(pattern_entry[current_x +
                current_y*pattern_width].warp_above) != warp_above){
            break;
        }
        (*steps_right)++;
    } while(*incremented_coord != initial_coord);

    *incremented_coord = initial_coord;
    do{
        if(*incremented_coord == 0){
            *incremented_coord = max_size;
        }
        (*incremented_coord)--;
        if((bool)(pattern_entry[current_x +
                current_y*pattern_width].warp_above) != warp_above){
            break;
        }
        (*steps_left)++;
    } while(*incremented_coord != initial_coord);
}

float vonMises(float cos_x, float b) {
    // assumes a = 0, b > 0 is a concentration parameter.

    float I0, absB = std::abs(b);
    if (std::abs(b) <= 3.75f) {
        float t = absB / 3.75f;
        t = t * t;
        I0 = 1.0f + t*(3.5156229f + t*(3.0899424f + t*(1.2067492f
                        + t*(0.2659732f + t*(0.0360768f + t*0.0045813f)))));
    } else {
        float t = 3.75f / absB;
        I0 = expf(absB) / std::sqrt(absB) * (0.39894228f + t*(0.01328592f
                    + t*(0.00225319f + t*(-0.00157565f + t*(0.00916281f + t*(-0.02057706f
                                    + t*(0.02635537f + t*(-0.01647633f + t*0.00392377f))))))));
    }

    return expf(b * cos_x) / (2 * M_PI * I0);
}

struct PatternData
{
    VUtils::Color color;
    float u, v; //Segment uv coordinates (in angles)
    float x, y; //position within segment. 
    bool warp_above; 
};

PatternData getPatternData(WeaveParameters *params, float uv_x, float uv_y) {

    //Set repeating uv coordinates.
    float u_repeat = fmod(uv_x*params->uscale,1.f);
    float v_repeat = fmod(uv_y*params->vscale,1.f);
    //TODO(Vidar): Check why this crashes sometimes
    if (u_repeat < 0.f) {
        u_repeat = u_repeat - floor(u_repeat);
    }
    if (v_repeat < 0.f) {
        v_repeat = v_repeat - floor(v_repeat);
    }

    uint32_t pattern_x = (uint32_t)(u_repeat*(float)(params->pattern_width));
    uint32_t pattern_y = (uint32_t)(v_repeat*(float)(params->pattern_height));

    PaletteEntry current_point = params->pattern_entry[pattern_x +
        pattern_y*params->pattern_width];        

    //Calculate the size of the segment
    uint32_t steps_left_warp = 0, steps_right_warp = 0;
    uint32_t steps_left_weft = 0, steps_right_weft = 0;
    if (current_point.warp_above) {
        calculateLengthOfSegment(current_point.warp_above, pattern_x,
            pattern_y, &steps_left_warp, &steps_right_warp,
            params->pattern_height, params->pattern_width,
            params->pattern_entry);
    }else{
        calculateLengthOfSegment(current_point.warp_above, pattern_x,
            pattern_y, &steps_left_weft, &steps_right_weft,
            params->pattern_height, params->pattern_width,
            params->pattern_entry);
    }

    //Yarn-segment-local coordinates.
    float l = (steps_left_warp + steps_right_warp + 1.f);
    float y = ((v_repeat*(float)(params->pattern_height) - (float)pattern_y)
            + steps_left_warp)/l;

    float w = (steps_left_weft + steps_right_weft + 1.f);
    float x = ((u_repeat*(float)(params->pattern_width) - (float)pattern_x)
            + steps_left_weft)/w;

    //Rescale x and y to [-1,1]
    x = x*2.f - 1.f;
    y = y*2.f - 1.f;

    //Switch X and Y for warp, so that we always have the yarn
    // cylinder going along the y axis
    if(!current_point.warp_above){
        float tmp1 = x;
        float tmp2 = w;
        x = -y;
        y = tmp1;
        w = l;
        l = tmp2;
    }

    //Calculate the yarn-segment-local u v coordinates along the curved cylinder
    //NOTE: This is different from how Irawan does it
    /*segment_u = asinf(x*sinf(m_umax));
        segment_v = asinf(y);*/
    //TODO(Vidar): Use a parameter for choosing model?
    float segment_u = y*params->umax;
    float segment_v = x*M_PI_2;
  
    PatternData ret_data = {};
    ret_data.color = VUtils::Color(current_point.color[0], current_point.color[1],
        current_point.color[2]);

    ret_data.u = segment_u;
    ret_data.v = segment_v;
    ret_data.x = x; 
    ret_data.y = y; 
    ret_data.warp_above = current_point.warp_above; 


    //return the results
    return ret_data;
}

float evalStapleSpecular(Vector wi, Vector wo, PatternData data, WeaveParameters *params) {
    if(!data.warp_above){
        float tmp2 = wi.x;
        float tmp3 = wo.x;
        wi.x = -wi.y; wi.y = tmp2;
        wo.x = -wo.y; wo.y = tmp3;
    }
    Vector H = normalize(wi + wo);

    float u = data.u;
    float x = data.x;
    float D;
    {
        float a = H.y*sinf(u) + H.z*cosf(u);
        D = (H.y*cosf(u)-H.z*sinf(u))/(sqrtf(H.x*H.x + a*a)) / tanf(params->psi);
    }
    float reflection = 0.f;
            
    float specular_v = atan2f(-H.y*sinf(u) - H.z*cosf(u), H.x) + acosf(D); //Plus eller minus i sista termen.
    //TODO(Vidar): Clamp specular_v, do we need it?
    // Make normal for highlights, uses u and specular_v
    Vector highlight_normal = normalize(Vector(sinf(specular_v), sinf(u)*cosf(specular_v),
        cosf(u)*cosf(specular_v)));

    if (fabsf(specular_v) < M_PI_2 && fabsf(D) < 1.f) {
        //we have specular reflection
        //get specular_x, using irawans transformation.
        float specular_x = specular_v/M_PI_2;
        // our transformation
        //float specular_x = sinf(specular_v);

        //Clamp specular_x
        specular_x = specular_x < 1.f - params->delta_x ? specular_x :
            1.f - params->delta_x;
        specular_x = specular_x > -1.f + params->delta_x ? specular_x :
            -1.f + params->delta_x;

        if (fabsf(specular_x - x) < params->delta_x) { //this takes the role of xi in the irawan paper.
            // --- Set Gv
            float a = 1.f; //radius of yarn
            float R = 1.f/(sin(params->umax)); //radius of curvature
            float Gv = a*(R + a*cosf(specular_v))/((wi + wo).length() * dot(highlight_normal,H) * fabsf(sinf(params->psi)));
            // --- Set fc
            float cos_x = -dot(wi, wo);
            float fc = params->alpha + vonMises(cos_x, params->beta);
            // --- Set A
            float widotn = dot(wi, highlight_normal);
            float wodotn = dot(wo, highlight_normal);
            //float A = m_sigma_s/m_sigma_t * (widotn*wodotn)/(widotn + wodotn);
            widotn = (widotn < 0.f) ? 0.f : widotn;   
            wodotn = (wodotn < 0.f) ? 0.f : wodotn;   
            //TODO(Vidar): This is where we get the NAN
            float A = 0.f; //sigmas are "unimportant"
            if(widotn > 0.f && wodotn > 0.f){
                A = 1.f / (4.0 * M_PI) * (widotn*wodotn)/(widotn + wodotn); //sigmas are "unimportant"
                //TODO(Peter): Explain from where the 1/4*PI factor comes from
            }
            float w = 2.f;
            reflection = 2.f*w*params->umax*fc*Gv*A/params->delta_x;
        }
    }
    return reflection;
}

float evalFilamentSpecular(Vector wi, Vector wo, PatternData data, WeaveParameters *params) {
    if(!data.warp_above){
        float tmp2 = wi.x;
        float tmp3 = wo.x;
        wi.x = -wi.y; wi.y = tmp2;
        wo.x = -wo.y; wo.y = tmp3;
    }
    Vector H = normalize(wi + wo);

    float v = data.v;
    float y = data.y;

    //TODO(Peter): explain from where these expressions come.
    //compute v from x using (11). Already done. We have it from data.
    //compute u(wi,v,wr) -- u as function of v. using (4)...
    float specular_u = atan2f(-H.z, H.y) + M_PI_2; //plus or minus in last t.
    //TODO(Peter): check that it indeed is just v that should be used 
    //to calculate Gu (6) in Irawans paper.
    //calculate yarn tangent.

    float reflection = 0.f;
    if (fabsf(specular_u) < params->umax) {
        // Make normal for highlights, uses v and specular_u
        Vector highlight_normal = normalize(Vector(sinf(v),
                    sinf(specular_u)*cosf(v),
                    cosf(specular_u)*cosf(v)));

        // Make tangent for highlights, uses v and specular_u
        Vector highlight_tangent = normalize(Vector(0.f, 
                    cosf(specular_u), -sinf(specular_u)));

        //get specular_y, using irawans transformation.
        float specular_y = specular_u/params->umax;
        // our transformation TODO(Peter): Verify!
        //float specular_y = sinf(specular_u)/sinf(m_umax);

        //Clamp specular_y TODO(Peter): change name of m_delta_x to m_delta_h
        specular_y = specular_y < 1.f - params->delta_x ? specular_y :
            1.f - params->delta_x;
        specular_y = specular_y > -1.f + params->delta_x ? specular_y :
            -1.f + params->delta_x;

        if (fabsf(specular_y - y) < params->delta_x) { //this takes the role of xi in the irawan paper.
            // --- Set Gu, using (6)
            float a = 1.f; //radius of yarn
            float R = 1.f/(sin(params->umax)); //radius of curvature
            float Gu = a*(R + a*cosf(v)) / 
                ((wi + wo).length() * fabsf((cross(highlight_tangent,H)).x));

            // --- Set fc
            float cos_x = -dot(wi, wo);
            float fc = params->alpha + vonMises(cos_x, params->beta);

            // --- Set A
            float widotn = dot(wi, highlight_normal);
            float wodotn = dot(wo, highlight_normal);
            //float A = m_sigma_s/m_sigma_t * (widotn*wodotn)/(widotn + wodotn);
            widotn = (widotn < 0.f) ? 0.f : widotn;   
            wodotn = (wodotn < 0.f) ? 0.f : wodotn;   
            float A = 0.f;
            if(widotn > 0.f && wodotn > 0.f){
                A = 1.f / (4.0 * M_PI) * (widotn*wodotn)/(widotn + wodotn); //sigmas are "unimportant"
                //TODO(Peter): Explain from where the 1/4*PI factor comes from
            }
            float l = 2.f;
            //TODO(Peter): Implement As, -- smoothes the dissapeares of the
            // higlight near the ends. Described in (9)
            reflection = 2.f*l*params->umax*fc*Gu*A/params->delta_x;
        }
    }

    return reflection;
}


VUtils::Color dynamic_eval(const VUtils::VRayContext &rc, const Vector &direction,
                           VUtils::Color &lightColor, VUtils::Color &origLightColor,
                           float probLight, int flags, WeaveParameters *weave_parameters,
                           Matrix nm) {
    if(weave_parameters->pattern_entry == 0){
        //Invalid pattern
        return VUtils::Color(0.f,0.f,0.f);
    }
    
    float m_delta_x = weave_parameters->delta_x;
    float m_umax = weave_parameters->umax;
    float m_alpha = weave_parameters->alpha;
    float m_beta = weave_parameters->beta;
    float m_psi = weave_parameters->psi;


    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

    VUtils::Vector v;
    Point3 uv = sc.UVW(1);
	v.x = uv.x; v.y = uv.y; v.z = uv.z;

    PatternData dat = getPatternData(weave_parameters,uv.x,uv.y);
    float segment_u = dat.u;
    bool warp_above = dat.warp_above;
    float x = dat.x;


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

    float reflection;
    if(weave_parameters->psi < 0.001f){
        reflection = evalFilamentSpecular(wi,wo,dat,weave_parameters);
    }else{
        reflection = evalStapleSpecular(wi,wo,dat,weave_parameters);
    }

    float specular = weave_parameters->specular_strength;
    float diffuse = 1.f - specular;

    v.x = (specular*reflection + diffuse*dat.color.r) * lightColor.r;
    v.y = (specular*reflection + diffuse*dat.color.g) * lightColor.g;
    v.z = (specular*reflection + diffuse*dat.color.b) * lightColor.b;

    VUtils::Color res(fabsf(v.x), fabsf(v.y), fabsf(v.z));
    return res;
}
