#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>

//TODO(Vidar):Define these in another file
//#define TL_NO_TEXTURE_CALLBACKS
//#define TL_THUNDERLOOM_IMPLEMENTATION
#include "thunderloom.h"


struct tlRaytracerParameters
{
	tlWeaveParameters *d_weave_params;
	tlYarnType *d_yarn_types;
	PatternEntry *d_pattern;
};

struct tlRaytracerParameters *tl_raytracer_load_parameters(tlWeaveParameters *params)
{
    return 0;
}

float saturate(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}


void tl_raytracer_render_to_memory(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, uint8_t *pixels)
{
    int w = res;
    int h = res;
	float sun_z = 1.f - sqrtf(sun_x*sun_x + sun_y*sun_y);
    for(int y=0;y<res;y++){
        for(int x=0;x<res;x++){
            int i = x + y * w;
            float fx = 2.f*(float)x/(float)w - 1.f;
            float fy = 2.f*(float)y/(float)h - 1.f;

            tlIntersectionData intersection_data;
            intersection_data.uv_x = fx;
            intersection_data.uv_y = fy;
            intersection_data.wi_x = sun_x;
            intersection_data.wi_y = sun_y;
            intersection_data.wi_z = sun_z;

            intersection_data.wo_x = 0.f;
            intersection_data.wo_y = 0.f;
            intersection_data.wo_z = 1.f;
            intersection_data.context = 0;

            tlColor col = tl_shade(intersection_data, params);
            
            pixels[i * 4 + 0] = (uint8_t)(saturate(col.r)*255.f);
            pixels[i * 4 + 1] = (uint8_t)(saturate(col.g)*255.f);
            pixels[i * 4 + 2] = (uint8_t)(saturate(col.b)*255.f);
            pixels[i * 4 + 3] = 255;
        }
    }
}


void tl_raytracer_render_to_opengl_pbo(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, unsigned int pbo_gl)
{
}

