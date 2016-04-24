#pragma once
#include <stdint.h>
#include "wif/wif.h"

#ifndef WC_PREFIX
#define WC_PREFIX
#endif

typedef struct
{
// These are the parameters to the model
    float uscale;
    float vscale;
    float umax;
    float psi;
    float alpha;
    float beta;
    float delta_x;
    float specular_strength;
    float intensity_fineness;
    float yarnvar_amplitude;
    float yarnvar_xscale;
    float yarnvar_yscale;
    float yarnvar_persistance;
    uint32_t yarnvar_octaves;

// These are set by calling one of the wcWeavePatternFrom* functions
// after all parameters above have been defined
    uint32_t pattern_height;
    uint32_t pattern_width;
    PatternEntry * pattern_entry;
    float specular_normalization;
} wcWeaveParameters;

typedef struct
{
    float color_r, color_g, color_b;
    float normal_x, normal_y, normal_z; //(not yarn local coordinates)
    float u, v; //Segment uv coordinates (in angles)
    float length, width; //Segment length and width
    float x, y; //position within segment (in yarn local coordiantes). 
    uint32_t total_index_x, total_index_y; //index for elements (not yarn local coordinates). TODO(Peter): perhaps a better name would be good?
    uint8_t warp_above; 
} wcPatternData;

typedef struct
{
    float uv_x, uv_y;
    float wi_x, wi_y, wi_z;
    float wo_x, wo_y, wo_z;
} wcIntersectionData;

WC_PREFIX
void wcWeavePatternFromData(wcWeaveParameters *params, uint8_t *warp_above,
    float *warp_color, float *weft_color, uint32_t pattern_width,
    uint32_t pattern_height);
WC_PREFIX
void wcWeavePatternFromFile(wcWeaveParameters *params, const char *filename);
WC_PREFIX
void wcWeavePatternFromFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename);
/* wcWeavePatternFromFile calles one of the functions below depending on
 * file extension*/
WC_PREFIX
void wcWeavePatternFromWIF(wcWeaveParameters *params, const char *filename);
WC_PREFIX
void wcWeavePatternFromWIF_wchar(wcWeaveParameters *params,
    const wchar_t *filename);
WC_PREFIX
void wcWeavePatternFromWeaveFile(wcWeaveParameters *params, const char *filename);
WC_PREFIX
void wcWeavePatternFromWeaveFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename);


WC_PREFIX
void wcFreeWeavePattern(wcWeaveParameters *params);
WC_PREFIX
wcPatternData wcGetPatternData(wcIntersectionData intersection_data,
    const wcWeaveParameters *params);
WC_PREFIX
float wcEvalDiffuse(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params);
WC_PREFIX
float wcEvalSpecular(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params);

