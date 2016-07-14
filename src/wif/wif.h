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
	YARN_TYPE_PARAM(umax, UMAX)\
	YARN_TYPE_PARAM(psi, PSI)\
	YARN_TYPE_PARAM(alpha, ALPHA)\
	YARN_TYPE_PARAM(beta, BETA)\
	YARN_TYPE_PARAM(delta_x, DELTAX)\
	YARN_TYPE_PARAM(specular_strength, SPECULAR)

typedef struct
{
#define YARN_TYPE_PARAM(param,A) float param;
	YARN_TYPE_PARAMETERS
    float color[3];
#define YARN_TYPE_PARAM(param,A) uint8_t param##_enabled;
	YARN_TYPE_PARAMETERS
    uint8_t color_enabled;
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
    1,1,1,1,1,1,1, // Everything enabled
};

typedef struct
{
    int num_yarn_types;
    YarnType common_yarn;
    PatternEntry *entries;
    YarnType *yarn_types;
}Pattern;
// Getter functions for the yarn type parameters.
// These take into account whether the parameter is enabled or not
#define YARN_TYPE_PARAM(param,A) static inline float yarn_type_get_##param\
(Pattern *p, uint32_t i){\
YarnType yarn_type = p->yarn_types[i];\
return yarn_type.param##_enabled ? yarn_type.param : p->common_yarn.param;}

YARN_TYPE_PARAMETERS

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
Pattern *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h, 
        float *rw, float *rh);
void wif_free_pattern(PatternEntry *pattern);

