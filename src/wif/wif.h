#pragma once
#include "common.h"
#include "wchar.h"

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

typedef struct
{
    uint8_t warp_above;
    uint8_t yarn_type; //NOTE(Vidar): We're limited to 256 yarn types now,
                       //should be ok, right?
}PatternEntry;

//NOTE(Vidar):These are the yarn type parameters (except color)
#define YARN_TYPE_PARAMETERS\
	YARN_TYPE_PARAM(umax)\
	YARN_TYPE_PARAM(psi)\
	YARN_TYPE_PARAM(alpha)\
	YARN_TYPE_PARAM(beta)\
	YARN_TYPE_PARAM(delta_x)\
	YARN_TYPE_PARAM(specular_strength)

typedef struct
{
#define YARN_TYPE_PARAM(param) float param;
	YARN_TYPE_PARAMETERS
    float color[3];
}YarnType;

static const
YarnType default_yarn_type =
{
	0.5f,  //umax
	0.5f,  //psi
	0.05f, //alpha
	4.f,   //beta
	0.3f,  //delta_x
	0.4f,  //specular_strength
	0.3f, 0.3f, 0.3f,  //color
};

typedef struct
{
    int num_yarn_types;
    PatternEntry *entries;
    YarnType *yarn_types;
}Pattern;

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
Pattern *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h, 
        float *rw, float *rh);
void wif_free_pattern(PatternEntry *pattern);

