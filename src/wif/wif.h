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
#define WC_MAX_YARN_TYPES 256
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
#undef YARN_TYPE_PARAM
    float color[3];
#define YARN_TYPE_PARAM(param,A) uint8_t param##_enabled;
	YARN_TYPE_PARAMETERS
#undef YARN_TYPE_PARAM
    uint8_t color_enabled;
#define YARN_TYPE_PARAM(param,A) void* param##_texmap;
	YARN_TYPE_PARAMETERS
#undef YARN_TYPE_PARAM
	void *color_texmap;
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
	0,             // No texmaps...
};


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

typedef struct
{
    int num_yarn_types;
    PatternEntry *entries;
	//Yarn type 0 contains the settings which are common for all yarn types
	// unless overridden
    YarnType *yarn_types;
}Pattern;

//These needs to be defined in the implementation...
float wc_eval_texmap_mono(void *texmap, void *context);
void  wc_eval_texmap_color(void *texmap, void *context, float *col);

// Getter functions for the yarn type parameters.
// These take into account whether the parameter is enabled or not
// And handle the texmaps
#define YARN_TYPE_PARAM(param,A) static inline float yarn_type_get_##param\
	(Pattern *p, uint32_t i, void* context){\
	YarnType yarn_type = p->yarn_types[i];\
    float ret;\
	if(yarn_type.param##_enabled){\
		ret = yarn_type.param;\
		if(yarn_type.param##_texmap){\
			ret=wc_eval_texmap_mono(yarn_type.param##_texmap,context);\
		}\
	} else{\
		ret = p->yarn_types[0].param;\
		if(p->yarn_types[0].param##_texmap){\
			ret=wc_eval_texmap_mono(p->yarn_types[0].param##_texmap,context);\
		}\
	}\
	return ret;}
YARN_TYPE_PARAMETERS
#undef YARN_TYPE_PARAM

// Read a WIF file from disk
WeaveData *wif_read(const char *filename);
WeaveData *wif_read_wchar(const wchar_t *filename);
// Free the WeaveData data structure
void wif_free_weavedata(WeaveData *data);
// Allocate and return the pattern from a WIF file
Pattern *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h, 
        float *rw, float *rh);
void wif_free_pattern(PatternEntry *pattern);

