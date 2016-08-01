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

//TODO(Peter): Find a nice place for this!
//Set SubTexMap ids for ALL yarntype specific texmaps
//These ids will be offsetted with number of
//texmaps times the the current yarn_type id.
//Used when rendering and 

//NOTE(Peter):These are parameters for that can be varied  
//for each uv position. These paramters are good to vary using texture maps
#define TEXMAP_PARAMETERS\

enum {
#define TEXMAP(param) texmaps_##param,
	TEXMAP_PARAMETERS
#undef TEXMAP
	NUMBER_OF_FIXED_TEXMAPS //how many non yarn specific texmaps
};

//TODO(Vidar):Move to the 3ds max plugin
//NOTE(Peter):These are parameters for each yarn type that can be varied  
//for each uv position. These paramters are good to vary using texture maps
#define YARN_TYPE_TEXMAP_PARAMETERS\
	YARN_TYPE_TEXMAP(color,DIFFUSE)\
	YARN_TYPE_TEXMAP(specular_strength,SPECULAR)\
	//YARN_TYPE_TEXMAP(yarnsize)
enum {
#define YARN_TYPE_TEXMAP(param,A) yrn_texmaps_##param,
	YARN_TYPE_TEXMAP_PARAMETERS
#undef YARN_TYPE_PARAM
	NUMBER_OF_YRN_TEXMAPS
};

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
void wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h, 
     float *rw, float *rh, wcWeaveParameters);
//void wif_free_pattern(PatternEntry *pattern);

