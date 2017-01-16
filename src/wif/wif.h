#pragma once
#include "common.h"
#include "wchar.h"

//TODO(Vidar): Move into the woven_cloth.cpp file?

typedef struct{
    uint32_t num_threads;
    float spacing, thickness;
    uint32_t *colors;
}WarpOrWeftData;

typedef struct
{
    WarpOrWeftData warp, weft;
    uint32_t num_shafts, num_treadles, num_colors;
    uint32_t current_section;
    uint32_t read_sections; //NOTE(Vidar): Which sections have been read?
    uint32_t read_keys; //NOTE(Vidar): Which keys have been read in the current
                        // section?
    uint8_t *tieup;
    uint32_t *treadling, *threading; //TODO(Vidar): Move to WarpOrWeftData?
    float *colors;
}WeaveData;

// Read a WIF file from disk
WeaveData *wif_read(char *in_data, long len, const char **error);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
void wif_get_pattern(tlWeaveParameters *param, WeaveData *data, uint32_t *w, uint32_t *h, 
     float *rw, float *rh);
//void wif_free_pattern(PatternEntry *pattern);

