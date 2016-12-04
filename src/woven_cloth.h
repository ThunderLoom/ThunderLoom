#pragma once
//TODO(Vidar): Consistent naming of functions!
/* A shader model for woven cloth
 * By Vidar Nelson and Peter McEvoy
 *
 * !Read this if you want to integrate the shader with your own renderer!
 */

/* --- Basic usage ---
 * Before rendering, call wcWeavePatternFromFile to load a weaving pattern.
 */
typedef struct wcWeaveParameters wcWeaveParameters; //Forward decl.
void wcWeavePatternFromFile(wcWeaveParameters *params, const char *filename);
#ifdef WC_WCHAR
void wcWeavePatternFromFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename);
#endif

/* Before each frame, call wcPrepare to apply all changes to parameters
 */
void wcFinalizeWeaveParameters(wcWeaveParameters *params);

/* During rendering, fill out an instance of wcIntersectionData
 * with the relevant information about the current shading point,
 * and call wcShade to recieve the reflected color.
 */
typedef struct {float r,g,b;} wcColor;
typedef struct wcIntersectionData wcIntersectionData;
wcColor wcShade(wcIntersectionData intersection_data,
        const wcWeaveParameters *params);

/* When you're done, call wcFreeWeavePattern to free the memory
 * used by the weaving pattern.
 */
void wcFreeWeavePattern(wcWeaveParameters *params);

/* --- Fabric Parameters ---
 * When the weaving pattern is loaded, it is assigned default parameter values.
 * There are some parameters which affect the entire fabric. They are stored in
 * the wcWeavePatternFromFile struct. The parameters are defined by the
 * WC_FABRIC_PARAMETERS X-Macro below
 */
#define WC_FABRIC_PARAMETERS\
	WC_FLOAT_PARAM(uscale)\
	WC_FLOAT_PARAM(vscale)\
	WC_FLOAT_PARAM(intensity_fineness)\
	WC_INT_PARAM(realworld_uv)

/* --- Yarn Parameters ---
 * Furthermore, there are some settings which affect the yarn that the cloth
 * consists of.
 * Each color in the wif file will be separated into its own ''yarn type'', 
 * and can be given separate settings. There is an array of yarn types at
 * wcWeaveParameters.yarn_types, and the number of yarn types is stored in
 * wcWeaveParameters.num_yarn_types.
 *
 * Yarn type 0 is somewhat special since it contains settings which are common
 * for all yarn types. Unless a yarn type explicitly overrides a parameter,
 * by setting [parm name]_enabled to 1, the value stored in yarn type
 * 0 will be used.
 * Example:
 * ...
 * The yarn type parameters can be found in the wcYarnType struct and are
 * defined by WC_YARN_PARAMETERS below
 * Each parameter has three members in the wcYarnType struct,
 * float/wcColor [param], uint8_t [param]_enabled, and void * [param]_texmap
 */
#define WC_YARN_PARAMETERS\
	WC_FLOAT_PARAM(umax)\
	WC_FLOAT_PARAM(yarnsize)\
	WC_FLOAT_PARAM(psi)\
	WC_FLOAT_PARAM(alpha)\
	WC_FLOAT_PARAM(beta)\
	WC_FLOAT_PARAM(delta_x)\
	WC_FLOAT_PARAM(specular_strength)\
	WC_FLOAT_PARAM(specular_noise)\
    WC_COLOR_PARAM(color)

/* --- Intersection data ---
 * During rendering, before calling wcShade, the wcIntersectionData struct
 * needs to be filled out. This struct looks like this:
 */
struct wcIntersectionData
{
    float uv_x, uv_y;       // Texture coordinates
    float wi_x, wi_y, wi_z; // Incident direction 
    float wo_x, wo_y, wo_z; // Outgoing direction
	void *context;          // User data sent to texturing callbacks
};
/* The coordinate system for the directions needs to be defined such that
 * x points along dP/du, y points along dP/dv, and z points along the normal.
 * Here, dP/du and dP/dv are the derivatives of the current shading point with
 * respect to the u and v texture coordinates, respectively. I.e. vectors
 * which point in the direction of increasing u or v along the surface.
 * The ''context'' member is explained below.
 */

/* --- Texturing ---
 * Any yarn type parameter can be controlled using textures. This is
 * implemented via callbacks.
 * To assign a texture to a parameter, set the [param]_texture
 * member of wcYarnType to the adress of that texture, or some other identifier
 * which makes sense in your renderer. 
 * During rendering, when the shader needs to evaluate a parameter, it will call
 * either wc_eval_texmap_mono or wc_eval_texmap_color, declared below.
 * The input to these functions are the texmap pointer and the context pointer
 * from wcIntersectionData.
 * To implement these callbacks, make sure that WC_TEXTURE_CALLBACKS is defined
 * when you compile woven_cloth.cpp.
 */
//TODO(Vidar): Send these callbacks as parameters instead??
float wc_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context);
// Called when a texture is applied to a float parameter.
float wc_eval_texmap_mono(void *texmap, void *context);
// Called when a texture is applied to a color parameter.
wcColor wc_eval_texmap_color(void *texmap, void *context);

/* --- Separating diffuse and specular reflection ---
 * ...
 */ 


/* ------------ Implementation --------------------- */

#include <stdint.h>


typedef struct
{
#define WC_FLOAT_PARAM(name) float name;
#define WC_INT_PARAM(name)  uint8_t name;
#define WC_COLOR_PARAM(name) wcColor name;
WC_YARN_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_INT_PARAM
#undef WC_COLOR_PARAM

#define WC_FLOAT_PARAM(name) uint8_t name##_enabled;
#define WC_INT_PARAM(name)  uint8_t name##_enabled;
#define WC_COLOR_PARAM(name) uint8_t name##_enabled;
WC_YARN_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_INT_PARAM
#undef WC_COLOR_PARAM

#define WC_FLOAT_PARAM(name) void *name##_texmap;
#define WC_INT_PARAM(name)  void *name##_texmap;
#define WC_COLOR_PARAM(name) void *name##_texmap;
WC_YARN_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_INT_PARAM
#undef WC_COLOR_PARAM
}wcYarnType;

typedef struct
{
    uint8_t warp_above;
    uint8_t yarn_type; //NOTE(Vidar): We're limited to 256 yarn types now,
                       //should be ok, right?
}PatternEntry;
#define WC_MAX_YARN_TYPES 256

//TODO(Vidar): Give all parameters default values

struct wcWeaveParameters
{
#define WC_FLOAT_PARAM(name) float name;
#define WC_INT_PARAM(name)  uint8_t name;
#define WC_COLOR_PARAM(name) wcColor name;
WC_FABRIC_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_INT_PARAM
#undef WC_COLOR_PARAM
// These are set by calling one of the wcWeavePatternFrom* functions
// after all parameters above have been defined
//TODO(Vidar): Move to the pattern struct?
    uint32_t pattern_height;
    uint32_t pattern_width;
    uint32_t num_yarn_types;
    PatternEntry *pattern;
    wcYarnType *yarn_types;
    float specular_normalization;
    float pattern_realheight;
    float pattern_realwidth;
};

typedef struct
{
    uint32_t yarn_type;
    float normal_x, normal_y, normal_z; //(not yarn local coordinates)
    float u, v; //Segment uv coordinates (in angles)
    float length, width; //Segment length and width
    float x, y; //position within segment (in yarn local coordiantes). 
    uint32_t total_index_x, total_index_y; //index for elements (not yarn local coordinates). TODO(Peter): perhaps a better name would be good?
    uint8_t warp_above; 
    uint8_t yarn_hit; //True if we hit a yarn. False if we hit space inbetween. 
    uint8_t ext_between_parallel; //True if extensionis between to adjascent parallel yarns. -> bend should be zero
} wcPatternData;

wcPatternData wcGetPatternData(wcIntersectionData intersection_data,
    const wcWeaveParameters *params);
wcColor wcEvalDiffuse(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params);
float wcEvalSpecular(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params);

void wcWeavePatternFromData(wcWeaveParameters *params, uint8_t *warp_above,
    float *warp_color, float *weft_color, uint32_t pattern_width,
    uint32_t pattern_height);
/* wcWeavePatternFromFile calles one of the functions below depending on
 * file extension*/
void wcWeavePatternFromWIF(wcWeaveParameters *params, const char *filename);
void wcWeavePatternFromWeaveFile(wcWeaveParameters *params, const char *filename);
#ifdef WC_WCHAR
void wcWeavePatternFromWIF_wchar(wcWeaveParameters *params,
    const wchar_t *filename);
void wcWeavePatternFromWeaveFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename);
#endif

typedef struct
{
    PatternEntry pattern_entry;
    float length, width; //Segment length and width
    float start_u, start_v; //Coordinates for top left corner of segment in total uv coordinates.
    uint8_t between_parallel;
    uint8_t warp_above;
    uint8_t yarn_hit;
} wcYarnSegment;

wcYarnSegment wcGetYarnSegment(float total_u, float total_v,
	const wcWeaveParameters *params, const wcIntersectionData *intersection_data);

static const
wcYarnType wc_default_yarn_type =
{
	0.5f,  //umax
	1.0f,  //yarnsize
	0.5f,  //psi
	0.05f, //alpha
	4.f,   //beta
	0.3f,  //delta_x
	0.4f,  //specular_strength
    0.f,   //specular_noise
    {0.3f, 0.3f, 0.3f},  //color
    0
};

static float wc_yarn_type_get_lookup_yarnsize(const wcWeaveParameters *p,
        uint32_t i, float u, float v, void* context) {
    wcYarnType yarn_type = p->yarn_types[i];
    float ret;
    if(yarn_type.yarnsize_enabled){
        ret = yarn_type.yarnsize;
        if(yarn_type.yarnsize_texmap){
            ret=wc_eval_texmap_mono_lookup(yarn_type.yarnsize_texmap,u,v,context);
        }
    } else{
        ret = p->yarn_types[0].yarnsize;
        if(p->yarn_types[0].yarnsize_texmap){
            ret=wc_eval_texmap_mono_lookup(p->yarn_types[0].yarnsize_texmap,u,v,context);
            //ret=wc_eval_texmap_mono(p->yarn_types[0].param##_texmap,context);
        }
    }
    return ret;
}
// Getter functions for the yarn type parameters.
// These take into account whether the parameter is enabled or not
// and handle the texmaps
#define WC_FLOAT_PARAM(param) static float wc_yarn_type_get_##param\
    (const wcWeaveParameters *p, uint32_t i, void* context){\
	wcYarnType yarn_type = p->yarn_types[i];\
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
#define WC_COLOR_PARAM(param) static wcColor wc_yarn_type_get_##param\
    (const wcWeaveParameters *p, uint32_t i, void* context){\
	wcYarnType yarn_type = p->yarn_types[i];\
    wcColor ret;\
	if(yarn_type.param##_enabled){\
		ret = yarn_type.param;\
		if(yarn_type.param##_texmap){\
			ret=wc_eval_texmap_color(yarn_type.param##_texmap,context);\
		}\
	} else{\
		ret = p->yarn_types[0].param;\
		if(p->yarn_types[0].param##_texmap){\
			ret=wc_eval_texmap_color(p->yarn_types[0].param##_texmap,context);\
		}\
	}\
	return ret;}
#define WC_INT_PARAM(param) 
WC_YARN_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_COLOR_PARAM
#undef WC_INT_PARAM

static const int WC_NUM_YARN_PARAMETERS = 0
#define WC_FLOAT_PARAM(name) + 1
#define WC_INT_PARAM(name)  + 1
#define WC_COLOR_PARAM(name) + 1
WC_YARN_PARAMETERS
#undef WC_FLOAT_PARAM
#undef WC_INT_PARAM
#undef WC_COLOR_PARAM
;
