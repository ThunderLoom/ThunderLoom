#pragma once
#include <stdint.h>
#include "wif.h"

typedef struct
{
    PaletteEntry * pattern_entry;
    uint32_t pattern_height;
    uint32_t pattern_width;
    float uscale;
    float vscale;
    float umax;
    float psi;
    float alpha;
    float beta;
    float delta_x;
    float specular_strength;
    float specular_normalization;
} wcWeaveParameters;

typedef struct
{
    float color_r, color_g, color_b;
    float normal_x, normal_y, normal_z;
    float u, v; //Segment uv coordinates (in angles)
    float x, y; //position within segment. 
    bool warp_above; 
} wcPatternData;

typedef struct
{
    float uv_x, uv_y;
    float wi_x, wi_y, wi_z;
    float wo_x, wo_y, wo_z;
} wcIntersectionData;

wcPatternData wcGetPatternData(wcIntersectionData intersection_data,
    wcWeaveParameters *params);

float wcEvalStapleSpecular(wcIntersectionData intersection_data,
    wcPatternData data, wcWeaveParameters *params);

float wcEvalFilamentSpecular(wcIntersectionData intersection_data,
    wcPatternData data, wcWeaveParameters *params);
