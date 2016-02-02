#pragma once
#include "common.h"
typedef struct{
    uint32_t num_threads;
    float spacing, thickness;
}WarpOrWeftData;

 //TODO(VIdar): A lot of this could probably be 8-bit instead...
typedef struct
{
    WarpOrWeftData warp, weft;
    uint32_t num_shafts, num_treadles;
    uint8_t *tieup;
    uint32_t *treadling, *threading;
}WeaveData;

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
uint8_t *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h);

