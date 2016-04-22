#pragma once
#include "common.h"
#include "wchar.h"

typedef struct{
    uint32_t num_threads;
    float spacing, thickness;
    uint32_t *colors;
}WarpOrWeftData;

 //TODO(VIdar): A lot of this could probably be 8-bit instead...
typedef struct
{
    WarpOrWeftData warp, weft;
    uint32_t num_shafts, num_treadles, num_colors;
    uint8_t *tieup;
    uint32_t *treadling, *threading; //TODO(Vidar): Move to WarpOrWeftData?
    float *colors;
}WeaveData;

typedef struct
{
    uint8_t warp_above;
    float color[3];
}PaletteEntry;

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
PaletteEntry *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h);
PaletteEntry *wif_build_pattern_from_data(uint8_t *warp_above,
        float *warp_color, float *weft_color, uint32_t w, uint32_t h);
void wif_free_pattern(PaletteEntry *pattern);

