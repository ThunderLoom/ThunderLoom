#pragma once
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
} WeaveParameters;