#pragma once
/* A shader model for woven cloth
 * By Vidar Nelson and Peter McEvoy
 *
 * !Read this if you want to integrate the shader with your own renderer!
 */

#define TL_VERSION_MAJOR 0
#define TL_VERSION_MINOR 94
#define TL_VERSION_PATCH 0

#ifndef  TL_FUNC_PREFIX
#define TL_FUNC_PREFIX
#endif


/* --- Basic usage ---
 * 
 * In one of your source files, define TL_THUNDERLOOM_IMPLEMENTATION
 * before including this file, like so:

 * #define TL_THUNDERLOOM_IMPLEMENTATION
 * #include "thunderloom.h"
 *
 * Before rendering, call tl_weave_pattern_from_file to load a weaving pattern.
 * If you supply a pointer to a const char * in error, it will point to a static
 * error message if an error occurs.
 */
typedef struct tlWeaveParameters tlWeaveParameters; //Forward decl.
tlWeaveParameters *tl_weave_pattern_from_file(const char *filename,const char **error);

/* Before each frame, call tl_prepare to apply all changes to parameters
 */
void tl_prepare(tlWeaveParameters *params);

/* During rendering, fill out an instance of tlIntersectionData
 * with the relevant information about the current shading point,
 * and call tl_shade to recieve the reflected color.
 */
typedef struct {float r,g,b;} tlColor;
typedef struct tlIntersectionData tlIntersectionData;
TL_FUNC_PREFIX tlColor tl_shade(tlIntersectionData intersection_data,
        const tlWeaveParameters *params);

/* When you're done, call tl_free_weave_parameters to free the memory
 * used by the weaving pattern.
 */
void tl_free_weave_parameters(tlWeaveParameters *params);

/* --- Fabric Parameters ---
 * When the weaving pattern is loaded, it is assigned default parameter values.
 * There are some parameters which affect the entire fabric. They are stored in
 * the tl_weave_pattern_from_file struct. The parameters are defined by the
 * TL_FABRIC_PARAMETERS X-Macro below
 */
#define TL_FABRIC_PARAMETERS\
	TL_FLOAT_PARAM(uscale)\
	TL_FLOAT_PARAM(vscale)\
	TL_FLOAT_PARAM(uvrotation)\
	TL_FLOAT_PARAM(intensity_fineness)\
	TL_INT_PARAM(realworld_uv)

/* --- Yarn Parameters ---
 * Furthermore, there are some settings which affect the yarn that the cloth
 * consists of.
 * Each color in the ptn file will be separated into its own ''yarn type'', 
 * and can be given separate settings. There is an array of yarn types at
 * tlWeaveParameters.yarn_types, and the number of yarn types is stored in
 * tlWeaveParameters.num_yarn_types.
 *
 * Yarn type 0 is somewhat special since it contains settings which are common
 * for all yarn types. Unless a yarn type explicitly overrides a parameter,
 * by setting [parm name]_enabled to 1, the value stored in yarn type
 * 0 will be used.
 * Example:
 * ...
 * The yarn type parameters can be found in the tlYarnType struct and are
 * defined by TL_YARN_PARAMETERS below
 * Each parameter has three members in the tlYarnType struct,
 * float/tlColor [param], uint8_t [param]_enabled, and void * [param]_texmap
 */
#define TL_YARN_PARAMETERS\
	TL_FLOAT_PARAM(umax)\
	TL_FLOAT_PARAM(yarnsize)\
	TL_FLOAT_PARAM(psi)\
	TL_FLOAT_PARAM(specular_noise)\
    TL_COLOR_PARAM(color) \
	TL_FLOAT_PARAM(color_amount) \
	TL_FLOAT_PARAM(roughness_azimuthal)\
	TL_FLOAT_PARAM(roughness_longitudinal)\
	TL_FLOAT_PARAM(ior)\
	TL_COLOR_PARAM(normal_map)\
	TL_COLOR_PARAM(tangent_map)

//
/* --- Intersection data ---
 * During rendering, before calling tl_shade, the tlIntersectionData struct
 * needs to be filled out. This struct looks like this:
 */
struct tlIntersectionData
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
 * member of tlYarnType to the adress of that texture, or some other identifier
 * which makes sense in your renderer. 
 * During rendering, when the shader needs to evaluate a parameter, it will call
 * either tl_eval_texmap_mono or tl_eval_texmap_color, declared below.
 * The input to these functions are the texmap pointer and the context pointer
 * from tlIntersectionData.
 * If you don't want to implement these callbacks, make sure that
 * TL_NO_TEXTURE_CALLBACKS is defined when you include thunderloom.h
 */
//TODO(Vidar): Send these callbacks as parameters instead??
TL_FUNC_PREFIX float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context);
// Called when a texture is applied to a float parameter.
TL_FUNC_PREFIX float tl_eval_texmap_mono(void *texmap, void *context);
// Called when a texture is applied to a color parameter.
TL_FUNC_PREFIX tlColor tl_eval_texmap_color(void *texmap, void *context);

/* --- Separating diffuse and specular reflection ---
 * ...
 */ 


/* ------------ Implementation --------------------- */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
#define TL_FLOAT_PARAM(name) float name;
#define TL_INT_PARAM(name)  uint8_t name;
#define TL_COLOR_PARAM(name) tlColor name;
TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM

#define TL_FLOAT_PARAM(name) uint8_t name##_enabled;
#define TL_INT_PARAM(name)  uint8_t name##_enabled;
#define TL_COLOR_PARAM(name) uint8_t name##_enabled;
TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM

#define TL_FLOAT_PARAM(name) void *name##_texmap;
#define TL_INT_PARAM(name)  void *name##_texmap;
#define TL_COLOR_PARAM(name) void *name##_texmap;
TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM
}tlYarnType;

typedef struct
{
    uint8_t warp_above;
    uint8_t yarn_type; //NOTE(Vidar): We're limited to 256 yarn types now,
                       //should be ok, right?
}PatternEntry;
#define TL_MAX_YARN_TYPES 256

//TODO(Vidar): Give all parameters default values

struct tlWeaveParameters
{
#define TL_FLOAT_PARAM(name) float name;
#define TL_INT_PARAM(name)  uint8_t name;
#define TL_COLOR_PARAM(name) tlColor name;
TL_FABRIC_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM
// These are set by calling one of the tlWeavePatternFrom* functions
// after all parameters above have been defined
    uint32_t pattern_height;
    uint32_t pattern_width;
    uint32_t num_yarn_types;
    PatternEntry *pattern;
    tlYarnType *yarn_types;
    float specular_normalization;
    float pattern_realheight;
    float pattern_realwidth;
};

typedef struct
{
    uint32_t yarn_type;
    float u, v; //Segment uv coordinates (in angles)
    float length, width; //Segment length and width
    float x, y; //position within segment (in yarn local coordiantes). 
	float sin_rot_angle, cos_rot_angle; //Rotation angle for the yarn direction
    uint32_t total_index_x, total_index_y; //index for elements (not yarn local coordinates). TODO(Peter): perhaps a better name would be good?
    uint8_t warp_above; 
    uint8_t yarn_hit; //True if we hit a yarn. False if we hit space inbetween. 
    uint8_t ext_between_parallel; //True if extensionis between to adjascent parallel yarns. -> bend should be zero
} tlPatternData;

TL_FUNC_PREFIX tlPatternData tl_get_pattern_data(tlIntersectionData intersection_data,
    const tlWeaveParameters *params);
TL_FUNC_PREFIX tlColor tl_eval_diffuse(tlIntersectionData intersection_data,
    tlPatternData data, const tlWeaveParameters *params);
TL_FUNC_PREFIX tlColor tl_eval_specular(tlIntersectionData intersection_data,
    tlPatternData data, const tlWeaveParameters *params);

tlWeaveParameters *tl_weave_pattern_from_data(uint8_t *warp_above,
    uint8_t *yarn_type, uint32_t num_yarn_types, tlColor *yarn_colors,
    uint32_t pattern_width, uint32_t pattern_height);
/* tl_weave_pattern_from_file calles one of the functions below depending on
 * file extension*/
tlWeaveParameters *tl_weave_pattern_from_ptn(unsigned char *data,long len,
                const char **error);

unsigned char * tl_pattern_to_ptn_file(tlWeaveParameters *param,
	long *ret_len);

typedef struct
{
    PatternEntry pattern_entry;
    float length, width; //Segment length and width
    float start_u, start_v; //Coordinates for top left corner of segment in total uv coordinates.
    uint8_t between_parallel;
    uint8_t warp_above;
    uint8_t yarn_hit;
} tlYarnSegment;

static const
tlYarnType tl_default_yarn_type =
{
	0.5f,  //umax
	1.0f,  //yarnsize
	0.5f,  //psi
    0.f,   //specular_noise
    {0.3f, 0.3f, 0.3f},  //color
    1.f,   //color_amount
    0.6f,   //roughness_azimuthal
    0.6f,   //roughness_longitudinal
    1.6f,  //ior
    {0.5f, 0.5f, 1.f},  //normal_map
    {0.5f, 1.0f, 0.5f},  //tangent_map
    0
};

TL_FUNC_PREFIX tlYarnSegment tl_get_yarn_segment(float total_u, float total_v,
	const tlWeaveParameters *params, const tlIntersectionData *intersection_data);

TL_FUNC_PREFIX static float tl_yarn_type_get_lookup_yarnsize(const tlWeaveParameters *p,
        uint32_t i, float u, float v, void* context) {
    tlYarnType yarn_type = p->yarn_types[i];
    float ret;
    if(yarn_type.yarnsize_enabled){
        ret = yarn_type.yarnsize;
        if(yarn_type.yarnsize_texmap){
            ret=tl_eval_texmap_mono_lookup(yarn_type.yarnsize_texmap,u,v,context);
        }
    } else{
        ret = p->yarn_types[0].yarnsize;
        if(p->yarn_types[0].yarnsize_texmap){
            ret=tl_eval_texmap_mono_lookup(p->yarn_types[0].yarnsize_texmap,u,v,context);
            //ret=tl_eval_texmap_mono(p->yarn_types[0].param##_texmap,context);
        }
    }
    return ret;
}
// Getter functions for the yarn type parameters.
// These take into account whether the parameter is enabled or not
// and handle the texmaps
#define TL_FLOAT_PARAM(param) TL_FUNC_PREFIX static float tl_yarn_type_get_##param\
    (const tlWeaveParameters *p, uint32_t i, void* context){\
	tlYarnType yarn_type = p->yarn_types[i];\
    float ret;\
	if(yarn_type.param##_enabled){\
		ret = yarn_type.param;\
		if(yarn_type.param##_texmap){\
			ret=tl_eval_texmap_mono(yarn_type.param##_texmap,context);\
		}\
	} else{\
		ret = p->yarn_types[0].param;\
		if(p->yarn_types[0].param##_texmap){\
			ret=tl_eval_texmap_mono(p->yarn_types[0].param##_texmap,context);\
		}\
	}\
	return ret;}
#define TL_COLOR_PARAM(param) TL_FUNC_PREFIX static tlColor tl_yarn_type_get_##param\
    (const tlWeaveParameters *p, uint32_t i, void* context){\
	tlYarnType yarn_type = p->yarn_types[i];\
    tlColor ret;\
	if(yarn_type.param##_enabled){\
		ret = yarn_type.param;\
		if(yarn_type.param##_texmap){\
			ret=tl_eval_texmap_color(yarn_type.param##_texmap,context);\
		}\
	} else{\
		ret = p->yarn_types[0].param;\
		if(p->yarn_types[0].param##_texmap){\
			ret=tl_eval_texmap_color(p->yarn_types[0].param##_texmap,context);\
		}\
	}\
	return ret;}
#define TL_INT_PARAM(param) 
TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
#undef TL_INT_PARAM

static const int TL_NUM_YARN_PARAMETERS = 0
#define TL_FLOAT_PARAM(name) + 1
#define TL_INT_PARAM(name)  + 1
#define TL_COLOR_PARAM(name) + 1
TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM
;

/* ----------- IMPLEMENTATION --------------- */

#ifdef TL_THUNDERLOOM_IMPLEMENTATION

// For M_PI etc.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

//TODO(Vidar): Do we need this?
#include <stdio.h>

// -- 3D Vector data structure -- //
typedef struct
{
    float x,y,z,w;
} tlVector;

TL_FUNC_PREFIX static tlVector tlvector(float x, float y, float z)
{
    tlVector ret = {x,y,z,0.f};
    return ret;
}

TL_FUNC_PREFIX static float tlVector_magnitude(tlVector v)
{
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

TL_FUNC_PREFIX static tlVector tlVector_normalize(tlVector v)
{
    tlVector ret;
    float inv_mag = 1.f/sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    ret.x = v.x * inv_mag;
    ret.y = v.y * inv_mag;
    ret.z = v.z * inv_mag;
    return ret;
}

TL_FUNC_PREFIX static float tlVector_dot(tlVector a, tlVector b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

TL_FUNC_PREFIX static tlVector tlVector_cross(tlVector a, tlVector b)
{
    tlVector ret;
    ret.x = a.y*b.z - a.z*b.y;
    ret.y = a.z*b.x - a.x*b.z;
    ret.z = a.x*b.y - a.y*b.x;
    return ret;
}

TL_FUNC_PREFIX static tlVector tlVector_add(tlVector a, tlVector b)
{
    tlVector ret;
    ret.x = a.x + b.x;
    ret.y = a.y + b.y;
    ret.z = a.z + b.z;
    return ret;
}

// -- //
#include <stddef.h>

//NOTE(Vidar):These are the building blocks of a PTN file
struct tlPtnEntry  {
    uint32_t type,offset,size,version;
    const char *name;
};
#define TL_PTN_ENTRY(parent_struct,name,size) \
    {2,offsetof(parent_struct, name),\
        size, 1, #name},

//This is what is read from/written to the .PTN files for each struct
tlWeaveParameters tmp_tlWeaveParameters;
struct tlPtnEntry ptn_entry_weave_params[]={
    {1,0,0,1, "tlWeaveParameters"},
#define TL_FLOAT_PARAM(name) TL_PTN_ENTRY(tlWeaveParameters,name,sizeof(float))
#define TL_INT_PARAM(name)  TL_PTN_ENTRY(tlWeaveParameters,name,sizeof(uint8_t))
#define TL_COLOR_PARAM(name) TL_PTN_ENTRY(tlWeaveParameters,name,sizeof(tlColor))
    TL_FABRIC_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM
    TL_PTN_ENTRY(tlWeaveParameters,pattern_height,sizeof(uint32_t))
    TL_PTN_ENTRY(tlWeaveParameters,pattern_width,sizeof(uint32_t))
    TL_PTN_ENTRY(tlWeaveParameters,num_yarn_types,sizeof(uint32_t))
    TL_PTN_ENTRY(tlWeaveParameters,specular_normalization,sizeof(float))
    TL_PTN_ENTRY(tlWeaveParameters,pattern_realheight,sizeof(float))
    TL_PTN_ENTRY(tlWeaveParameters,pattern_realwidth,sizeof(float))
    {0,0,0,0,"end"}
};

tlYarnType tmp_tlYarnType;
struct tlPtnEntry ptn_entry_yarn_type[]={
    {1,0,0,1, "tlYarnType"},
#define TL_FLOAT_PARAM(name) TL_PTN_ENTRY(tlYarnType,name,sizeof(float)) \
    TL_PTN_ENTRY(tlYarnType,name##_enabled,sizeof(uint8_t)) 
#define TL_INT_PARAM(name)  TL_PTN_ENTRY(tlYarnType,name,sizeof(uint8_t))\
    TL_PTN_ENTRY(tlYarnType,name##_enabled,sizeof(uint8_t)) 
#define TL_COLOR_PARAM(name) TL_PTN_ENTRY(tlYarnType,name,sizeof(tlColor))\
    TL_PTN_ENTRY(tlYarnType,name##_enabled,sizeof(uint8_t)) 
    TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_INT_PARAM
#undef TL_COLOR_PARAM
    {0,0,0,0,"end"}
};

//NOTE(Vidar):This is a bit special, the size will be multiplied by 
// the number of entries in the pattern
struct tlPtnEntry ptn_entry_pattern = {3,0,2*sizeof(uint8_t),1, "tlPattern"};


static float tl_clamp(float x, float min, float max)
{
    return (x < min) ? min : (x > max) ? max : x;
}

/* Tiny Encryption Algorithm by David Wheeler and Roger Needham */
/* Taken from mitsuba source code. */
TL_FUNC_PREFIX static uint64_t sample_TEA(uint32_t v0, uint32_t v1, int rounds)
{
	uint32_t sum = 0;

	for (int i=0; i<rounds; ++i) {
		sum += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
		v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
	}

	return ((uint64_t) v1 << 32) + v0;
}

//From Mitsuba
TL_FUNC_PREFIX static float sample_TEA_single(uint32_t v0, uint32_t v1, int rounds)
{
    /* Trick from MTGP: generate an uniformly distributed
    single precision number in [1,2) and subtract 1. */
    union {
        uint32_t u;
        float f;
    } x;
    x.u = ((sample_TEA(v0, v1, rounds) & 0xFFFFFFFF) >> 9) | 0x3f800000UL;
    return x.f - 1.0f;
}

TL_FUNC_PREFIX void calculate_segment_uv_and_normal(tlPatternData *pattern_data,
        const tlWeaveParameters *params,tlIntersectionData *intersection_data)
{
    //Calculate the yarn-segment-local u v coordinates along the curved cylinder
    //NOTE: This is different from how Irawan does it
    /*segment_u = asinf(x*sinf(params->umax));
        segment_v = asinf(y);*/
    //TODO(Vidar): Use a parameter for choosing model?
    float umax;
    if (pattern_data->ext_between_parallel == 1){
        //if segment is extension between to parallel yarns -> bend = 0
        umax = 0.0001f;
    } else {
        umax = tl_yarn_type_get_umax(params,
                pattern_data->yarn_type,
                intersection_data->context);
    }
    // We are assuming that the given pattern_data is already in yarn local
    // coordinates (Y running along the yarn segment).
    float segment_u = pattern_data->y*umax;
    float segment_v = pattern_data->x*(float)M_PI_2;

    pattern_data->u        = segment_u;
    pattern_data->v        = segment_v;
}

void tl_prepare(tlWeaveParameters *params)
{
}

tlWeaveParameters *tl_weave_pattern_from_data(uint8_t *warp_above, uint8_t *yarn_type,
    uint32_t num_yarn_types, tlColor *yarn_colors, uint32_t pattern_width,
    uint32_t pattern_height)
{
    tlWeaveParameters *params =
        (tlWeaveParameters*)calloc(sizeof(tlWeaveParameters),1);
    params->pattern_width = pattern_width;
    params->pattern_height = pattern_height;
    num_yarn_types++;
    params->num_yarn_types = num_yarn_types;
    params->yarn_types = (tlYarnType*)calloc(sizeof(tlYarnType),num_yarn_types);
    params->yarn_types[0] = tl_default_yarn_type;
    for(unsigned int i=1;i<num_yarn_types;i++){
        params->yarn_types[i] = tl_default_yarn_type;
        params->yarn_types[i].color = yarn_colors[i-1];
        params->yarn_types[i].color_enabled = 1;
    }
    uint32_t pattern_size = pattern_width*pattern_height;
    params->pattern = (PatternEntry*)calloc(sizeof(PatternEntry),pattern_size);
    for(unsigned int i=0;i<pattern_size;i++){
        params->pattern[i].warp_above = warp_above[i];
        params->pattern[i].yarn_type = yarn_type[i];
    }
    return params;
}

#ifndef TL_NO_FILES

tlWeaveParameters *tl_weave_pattern_from_file(const char *filename,const char **error)
{
	tlWeaveParameters *param = 0;
	int len = 0;
	while(filename[len] != 0){
		len++;
	}
	if(len >=5){
		int ptn_ok = 1;
		const char *ptn_ext = ".ptn";
		for(int i=0;i<4;i++){
			char c = filename[len-1-i];
			c = (c <= 'Z' && c >= 'A') ? c + 32 : c;
			if(c != ptn_ext[3-i]){
				ptn_ok = 0;
			}
        }   
        if(ptn_ok){
            FILE *fp = 0;
			fp = fopen(filename,"rb");
            if (!fp) {
				if (error) {
					*error = "File not found.";
				}
                return 0;
            }
            fseek(fp,0,SEEK_END);
            long len = ftell(fp);
            fseek(fp,0,SEEK_SET);
            unsigned char *data = (unsigned char*)calloc(len,1);
            fread(data,1,len,fp);
            fclose(fp);
			param = tl_weave_pattern_from_ptn(data,len,error);
            free(data);
		}
	}
	return param;
}
#endif

struct tlPtnWriteCommand{
    struct tlPtnEntry *entry;
    unsigned char *data;
};

static unsigned char* tl_buffer_from_ptn_write_commands(int num_write_commands,
    struct tlPtnWriteCommand* write_commands, long *ret_len)
{
    int version = 2;
    //NOTE(Vidar):Calculate needed size of buffer
    size_t len = 2*sizeof(int); //Version number & end specifier
    for(int i=0;i<num_write_commands;i++) {
        struct tlPtnEntry *entry = write_commands[i].entry;
        do{
			size_t name_len = strlen(entry->name)+1;
            len+= 4*sizeof(uint32_t) + entry->size+name_len;
            entry++;
        }while((entry-1)->type != 0 && (entry-1)->type != 3);
    }

    //NOTE(Vidar):Allocate buffer
    unsigned char *data = (unsigned char *)calloc(len,1);
    unsigned char *dest=data;

    //NOTE(Vidar):Write version
    *(int*)dest=version;
    dest+=sizeof(int);

    //NOTE(Vidar):Write data
    for(int i=0;i<num_write_commands;i++) {
        struct tlPtnEntry *entry = write_commands[i].entry;
        do{
			uint32_t name_len = (uint32_t)(strlen(entry->name)+1);
            ((uint32_t*)dest)[0]=entry->type;
            ((uint32_t*)dest)[1]=name_len;
            ((uint32_t*)dest)[2]=entry->size;
            ((uint32_t*)dest)[3]=entry->version;
            dest+=4*sizeof(uint32_t);
            memcpy(dest,entry->name,name_len);
            dest+=name_len;
            memcpy(dest,write_commands[i].data + entry->offset,entry->size);
            dest+=entry->size;
            entry++;
        } while((entry-1)->type != 0 && (entry-1)->type != 3);
    }

    *(uint32_t*)dest = 0;
    *ret_len = (long)len;
    return data;
}

unsigned char * tl_pattern_to_ptn_file(tlWeaveParameters *param,
    long *ret_len)
{
    uint32_t pattern_size = param->pattern_width*param->pattern_height;
    int num_write_commands = 2+param->num_yarn_types;
    struct tlPtnWriteCommand *write_commands =
        (struct tlPtnWriteCommand*)calloc(num_write_commands,
        sizeof(struct tlPtnWriteCommand));
    write_commands[0].entry = ptn_entry_weave_params;
    write_commands[0].data  = (unsigned char *)param;
    //NOTE(vidar):We make a copy of the pattern entry so that we can change the
    // size
    struct tlPtnEntry pattern_entry = ptn_entry_pattern;
    pattern_entry.size      *= pattern_size;
    write_commands[1].entry  = &pattern_entry;
    write_commands[1].data   = (unsigned char *)param->pattern;
    int a = 2;
    for(unsigned int i=0;i<param->num_yarn_types;i++){
        write_commands[a+i].entry = ptn_entry_yarn_type;
        write_commands[a+i].data  = (unsigned char *)(param->yarn_types+i);
    }
    unsigned char *data = tl_buffer_from_ptn_write_commands(num_write_commands,
        write_commands, ret_len);
    free(write_commands);
    return data;
}

struct tlPtnConverter
{
    uint32_t src_version, target_version, src_size, target_size;
    const char *src_name,*target_name;
    unsigned char*(*func)(unsigned char *data);
};

static unsigned char* tl_ptn_specular_strength_to_specular_color( unsigned char *data)
{
    float val=*(float*)data;
    tlColor *col=(tlColor*)calloc(1,sizeof(tlColor));
    col->r=val;
    col->g=val;
    col->b=val;
    return (unsigned char *)col;
}

static struct tlPtnConverter tl_ptn_converters[]={
    {1,1,sizeof(float),sizeof(tlColor),"specular_strength","specular_color",
    tl_ptn_specular_strength_to_specular_color}
};
static uint32_t tl_num_ptn_converters=sizeof(tl_ptn_converters)/sizeof(*tl_ptn_converters);

unsigned char *tl_read_ptn_section(void *out, unsigned char* data,
    struct tlPtnEntry *entries)
{
    uint32_t type;
    do{
        type              = ((uint32_t*)data)[0];
        uint32_t name_len = ((uint32_t*)data)[1];
        uint32_t size     = ((uint32_t*)data)[2];
        uint32_t version  = ((uint32_t*)data)[3];
        data += 4*sizeof(uint32_t);
        const char *name = (char*)data;
        printf("  name: %s\n",name);
        data += name_len;
        struct tlPtnEntry *entry = entries;
        int must_free=0;
        unsigned char *entry_data=data;
        data += size;
        for(unsigned int i=0;i<tl_num_ptn_converters;i++){
            struct tlPtnConverter c=tl_ptn_converters[i];
            if(c.src_version==version && c.src_size==size && strcmp(c.src_name,name)==0){
                version=c.target_version;
                size=c.target_size;
                name=c.target_name;
                unsigned char *old_entry_data=entry_data;
                entry_data=c.func(entry_data);
                if(must_free){
                    free(old_entry_data);
                }
                must_free=1;
            }
        }
        while(entry->type != 0){
            if(strcmp(entry->name,name)==0 && entry->version == version){
                if(size == entry->size){
                    memcpy((unsigned char*)out+entry->offset,entry_data,size);
                }else{
                    printf("Warning: %s had the wrong size, not read\n", name);
                }
            }
            entry++;
        }
        if(must_free){
            free(entry_data);
        }
    } while(type != 0);
    return data;
}

static tlWeaveParameters *tl_pattern_from_ptn_file_v2(unsigned char *data,
    long len,const char **error)
{
    printf("loading PTN file version 2\n");
	tlWeaveParameters *param =
        (tlWeaveParameters*)calloc(sizeof(tlWeaveParameters),1);
    unsigned int num_read_yarn_types = 0;
	unsigned int num_read_pattern_entries = 0;
    while(1){
        uint32_t type     = ((uint32_t*)data)[0];
        if(type==0){
            break;
        }
        uint32_t name_len = ((uint32_t*)data)[1];
        uint32_t size     = ((uint32_t*)data)[2];
        uint32_t version  = ((uint32_t*)data)[3];
        data += 4*sizeof(uint32_t);
        char *name = (char*)data;
        printf("type: %d size: %d version %d name: %s\n",type,size,version,name);
        data += name_len;
        data += size;
        //NOTE(Vidar):Right now we assume that tlWeaveParameters is the first
        // section written, and that all info is correct
        if(strcmp(name,"tlWeaveParameters") == 0){
            data = tl_read_ptn_section(param,data,ptn_entry_weave_params);
            param->yarn_types = (tlYarnType*)calloc(param->num_yarn_types,
                    sizeof(tlYarnType));
			for (int i = 0; i < param->num_yarn_types; i++) {
				param->yarn_types[i] = tl_default_yarn_type;
			}
            uint32_t pattern_size = param->pattern_width*param->pattern_height;
            param->pattern = (PatternEntry*)calloc(pattern_size,
                    sizeof(PatternEntry));
        }
        if(strcmp(name,"tlYarnType") == 0){
            if(num_read_yarn_types < param->num_yarn_types){
                data = tl_read_ptn_section(&param->yarn_types[num_read_yarn_types],
                    data,ptn_entry_yarn_type);
                num_read_yarn_types++;
            }
        }
        if(strcmp(name,"tlPattern") == 0){
            if(version == 1){
                memcpy(param->pattern,data-size,size);
            }
            num_read_pattern_entries++;
        }
    }
    return param;
}


static tlWeaveParameters *tl_pattern_from_ptn_file_v1(unsigned char *data,
    long len,const char **error)
{
    int num_yarn_types = *(int*)(data + 24);
    printf("num yarn types: %d\n",num_yarn_types);
    int num_write_commands = 2+num_yarn_types;
    struct tlPtnWriteCommand *write_commands =
        (struct tlPtnWriteCommand*)calloc(num_write_commands,sizeof(struct tlPtnWriteCommand));
    int pattern_size = *(int*)(data+16) * *(int*)(data+20);
    printf("Pattern size: %d\n",pattern_size);

    //NOTE(Vidar):These are the structures as of v 0.91
    struct tlPtnEntry ptn_entry_weave_paramsv091[]={
        {1,  0,  0, 1, "tlWeaveParameters"},
        {2,  0,  4, 1, "uscale"},
        {2,  4,  4, 1, "vscale"},
        {2,  8,  4, 1, "intensity_fineness"},
        {2, 12,  1, 1, "realworld_uv"},
        {2, 16,  4, 1, "pattern_height"},
        {2, 20,  4, 1, "pattern_width"},
        {2, 24,  4, 1, "num_yarn_types"},
        {2, 48,  4, 1, "specular_normalization"},
        {2, 52,  4, 1, "pattern_realheight"},
        {2, 56,  4, 1, "pattern_realwidth"},
        {0,  0,  0, 0, "end"}
    };
    int weave_paramsv091_len = 64;

    struct tlPtnEntry ptn_entry_yarn_typev091[]={
        {1,  0,  0, 1, "tlYarnType"},
        {2,  0,  4, 1, "umax"},
        {2, 44,  1, 1, "umax_enabled"},
        {2,  4,  4, 1, "yarnsize"},
        {2, 45,  1, 1, "yarnsize_enabled"},
        {2,  8,  4, 1, "psi"},
        {2, 46,  1, 1, "psi_enabled"},
        {2, 12,  4, 1, "alpha"},
        {2, 47,  1, 1, "alpha_enabled"},
        {2, 16,  4, 1, "beta"},
        {2, 48,  1, 1, "beta_enabled"},
        {2, 20,  4, 1, "delta_x"},
        {2, 49,  1, 1, "delta_x_enabled"},
        {2, 24,  4, 1, "specular_strength"},
        {2, 50,  1, 1, "specular_strength_enabled"},
        {2, 28,  4, 1, "specular_noise"},
        {2, 51,  1, 1, "specular_noise_enabled"},
        {2, 32, 12, 1, "color"},
        {2, 52,  1, 1, "color_enabled"},
        {0,  0,  0, 0, "end"}
    };
    int yarn_typev091_len = 128;

    write_commands[0].entry = ptn_entry_weave_paramsv091;
    write_commands[0].data = data;
    data += weave_paramsv091_len;
    for(int i=0;i<num_yarn_types;i++){
        write_commands[2+i].entry = ptn_entry_yarn_typev091;
        write_commands[2+i].data = data;
        data += yarn_typev091_len;
    }
    struct tlPtnEntry ptn_entry_patternv091 = {3,0,
        (uint32_t)(2*sizeof(uint8_t)*pattern_size),1, "tlPattern"};
    struct tlPtnEntry pattern_entry = ptn_entry_pattern;
    pattern_entry.size      *= pattern_size;
    write_commands[1].entry  = &ptn_entry_patternv091;
    write_commands[1].data   = data;

    long ptn_v2_len=0;
    unsigned char *ptn_v2_buffer = tl_buffer_from_ptn_write_commands(
        num_write_commands,write_commands,&ptn_v2_len);
    tlWeaveParameters *param = tl_pattern_from_ptn_file_v2(
            ptn_v2_buffer+sizeof(int), ptn_v2_len,error);
    free(ptn_v2_buffer);
	return param;
}

tlWeaveParameters *tl_weave_pattern_from_ptn(unsigned char *data,long len,
    const char **error)
{
	tlWeaveParameters *param = 0;
	int version = 0;
	memcpy(&version,data,sizeof(int));\
	data += sizeof(int);
	len -= sizeof(int);
	switch(version){
	case 1:
		param = tl_pattern_from_ptn_file_v1(data,len,error);
		break;
    case 2:
        param = tl_pattern_from_ptn_file_v2(data,len,error);
        break;
	default:
		if (error) {
			*error = "Unknown PTN file version";
		}
		return 0;
	}
	return param;
}


//TODO(Vidar):This should free params too, right?
void tl_free_weave_parameters(tlWeaveParameters *params)
{
    if (params->yarn_types) {
        free(params->yarn_types);
    }
    if (params->pattern) {
        free(params->pattern);
    }
}

TL_FUNC_PREFIX static float intensity_variation(tlPatternData pattern_data)
{
	//NOTE(Vidar): a fineness of 3 seems to work fine...
	uint32_t intensity_fineness = 3;

    // have index to make a grid of finess*fineness squares 
    // of which to have the same brightness variations.

    uint32_t tindex_x = (uint32_t)pattern_data.total_index_x;
    uint32_t tindex_y = (uint32_t)pattern_data.total_index_y;

	//Switch X and Y for warp, so that we have the yarn going along y
	if (!pattern_data.warp_above) {
		uint32_t tmp = tindex_x;
		tindex_x = tindex_y;
		tindex_y = tmp;
	}

    // Potential overflow of r1 and r2 is acceptable here.
    uint32_t r1 = tindex_x*intensity_fineness - 
        (pattern_data.x*0.5)*pattern_data.width*intensity_fineness;
    uint32_t r2 = tindex_y*intensity_fineness - 
        (pattern_data.y*0.5)*pattern_data.length*intensity_fineness;

    float xi = sample_TEA_single(r1, r2, 8);
    float log_xi = -logf(xi);
    return log_xi < 10.f ? log_xi : 10.f;
}


TL_FUNC_PREFIX static void calculate_length_of_segment(uint8_t warp_above, uint32_t pattern_x,
                uint32_t pattern_y, uint32_t *steps_left,
                uint32_t *steps_right,  uint32_t pattern_width,
                uint32_t pattern_height, PatternEntry *pattern_entries)
{

    uint32_t current_x = pattern_x;
    uint32_t current_y = pattern_y;
    uint32_t *incremented_coord = warp_above ? &current_y : &current_x;
    uint32_t max_size = warp_above ? pattern_height: pattern_width;
    uint32_t initial_coord = warp_above ? pattern_y: pattern_x;
    *steps_right = 0;
    *steps_left  = 0;
    do{
        (*incremented_coord)++;
        if(*incremented_coord == max_size){
            *incremented_coord = 0;
        }
        if((pattern_entries[current_x +
                current_y*pattern_width].warp_above) != warp_above){
            break;
        }
        (*steps_right)++;
    } while(*incremented_coord != initial_coord);

    *incremented_coord = initial_coord;
    do{
        if(*incremented_coord == 0){
            *incremented_coord = max_size;
        }
        (*incremented_coord)--;
        if((pattern_entries[current_x +
                current_y*pattern_width].warp_above) != warp_above){
            break;
        }
        (*steps_left)++;
    } while(*incremented_coord != initial_coord);
}

TL_FUNC_PREFIX static void lookup_pattern_entry(PatternEntry* entry, const tlWeaveParameters* params, const int8_t x, const int8_t y) {
    //function to get pattern entry. Takes care of coordinate wrapping!
    int32_t tmpx = (int32_t)fmodf(x,(float)params->pattern_width);
    int32_t tmpy = (int32_t)fmodf(y,(float)params->pattern_height);
    if (tmpx < 0.f) {
        tmpx = params->pattern_width + tmpx;
    }
    if (tmpy < 0.f) {
        tmpy = params->pattern_height + tmpy;
    }
    *entry = params->pattern[tmpx + tmpy*params->pattern_width];
}

TL_FUNC_PREFIX static int32_t tl_repeat_index(const int32_t coord, const int32_t size) {
    //coord = -0.124*2 = -0.248
    float tmpcoord = (float)fmodf((float)coord, (float)size);
    if (tmpcoord < 0.f) {
        tmpcoord = size + tmpcoord;
    }
    return (int32_t)tmpcoord;
}

TL_FUNC_PREFIX static float get_yarn_segment_size(int32_t total_pattern_x, int32_t total_pattern_y,
		const tlWeaveParameters *params, const tlIntersectionData *intersection_data) {
        //remove scaling from coords
		float lookup_u = (total_pattern_x + 0.5f)/((float)(params->pattern_width)*params->uscale);
		float lookup_v = (total_pattern_y + 0.5f)/((float)(params->pattern_height)*params->vscale);
		
        PatternEntry yrntype;
		lookup_pattern_entry(&yrntype, params, total_pattern_x, total_pattern_y);

		//Sample potential yarnsize texmap in middle of cell
        float size = tl_yarn_type_get_lookup_yarnsize(params, yrntype.yarn_type,
                lookup_u, lookup_v, intersection_data->context);
		//float size = tl_yarn_type_get_yarnsize(params, yrntype.yarn_type, intersection_data->context);
		return size;
	}

TL_FUNC_PREFIX tlYarnSegment tl_get_yarn_segment(float total_u, float total_v,
		const tlWeaveParameters *params, const tlIntersectionData *intersection_data) {
    //total_u and total_v are scaled and non_repeating

    float u = fmodf(total_u,1.f);
    float v = fmodf(total_v,1.f);
    if (u < 0.f) {
        u = u - floor(u);
    }
    if (v < 0.f) {
        v = v - floor(v);
    }

    uint32_t pattern_width = params->pattern_width;
    uint32_t pattern_height = params->pattern_height;
    //TODO fmodf repeat uvs, to avoiding the u or v == 1 check.
    int32_t pattern_x = (int32_t)floor((total_u*(float)(pattern_width)));
    int32_t pattern_y = (int32_t)floor((total_v*(float)(pattern_height)));
    
    int32_t pattern_repeat_x = tl_repeat_index(pattern_x, pattern_width);
    int32_t pattern_repeat_y = tl_repeat_index(pattern_y, pattern_height);
    
    //The origin is the pattern entry from which size of segment is calculated. 
    //The origin entry changes if we miss a thin yarn.
    int32_t origin_x = pattern_x;
    int32_t origin_y = pattern_y;
    PatternEntry origin_entry = params->pattern[pattern_repeat_x + 
        pattern_repeat_y*pattern_width];
    uint8_t warp_above = origin_entry.warp_above;

    int32_t current_x = origin_x;
    int32_t current_y = origin_y;
    int32_t *incremented_coord_across = warp_above ? &current_x : &current_y;
    float cell_x = (u == 1.f) ? 1 : u*(float)(pattern_width) - pattern_repeat_x;
    float cell_y = (v == 1.f) ? 1 : v*(float)(pattern_height) - pattern_repeat_y;
    float *cell_coord_along = warp_above ? &cell_y : &cell_x;
    float *cell_coord_across = warp_above ? &cell_x : &cell_y;
    uint32_t max_size_across = warp_above ? pattern_width: pattern_height;
    uint32_t initial_coord_across = warp_above ? pattern_x: pattern_y;

    //Get segment size of yarn in current position in pattern matrix.
    float origin_yarnsize = get_yarn_segment_size(origin_x,
            origin_y, params, intersection_data);
    
    //init some flags
    uint8_t yarn_hit = 0;			// If we have hit the yarn.
    uint8_t between_parallel = 0;	// For later, if we are between parallel yarns.
    uint8_t extension = 0;          // If what we hit is an extention of an adjacent yarn.
    int32_t origin_offset = 0;      // With how much the origin is offset.
    if (fabsf(2*(*cell_coord_across)-1.f) <= origin_yarnsize) {
        yarn_hit = 1;
    } else {
        //Did not hit yarn, look for extension...
        int8_t direction = (*cell_coord_across >= 0.5) ? 1 : -1;
        PatternEntry tmp_pe; 
        *incremented_coord_across = initial_coord_across;
        uint8_t found_extension_entry = 1; //initialize flag
        do{
            (*incremented_coord_across) += direction;
            lookup_pattern_entry(&tmp_pe, params, current_x, current_y);
            if ((*incremented_coord_across) == initial_coord_across +
                    direction && tmp_pe.warp_above == warp_above) {
                between_parallel = 1;
            }
            if (abs((int)*incremented_coord_across - (int)initial_coord_across) == max_size_across) {
                found_extension_entry = 0;
                break;
            }
        } while (tmp_pe.warp_above == warp_above);
            
        //Need yarnsize sampled at center of yarnsegment, which is extention...
        float tmp_yarnsize = get_yarn_segment_size(current_x,
                current_y, params, intersection_data);
        
        //If there is yarn that can be used as extension. Did we hit it?
        if (found_extension_entry && fabsf(2*(*cell_coord_along)-1.f) <=
                tmp_yarnsize) {
            //Yes we hit an extention. Use this pattern entry as new origin!
            origin_entry = tmp_pe;
            origin_offset = ((!warp_above) ? ((int)origin_y - current_y) : ((int)origin_x - current_x));
            warp_above = origin_entry.warp_above;
            origin_x = current_x;
            origin_y = current_y;

            extension = 1;
            yarn_hit = 1;
        }
    }

    //Next need to determine yarnsegment dimensions...
    
    //look right and left from origin until we hit cell that is not current yarn weft/warp.
    uint32_t steps_right = 0;
    uint32_t steps_left  = 0;
    calculate_length_of_segment(origin_entry.warp_above,
            tl_repeat_index(origin_x, pattern_width),
            tl_repeat_index(origin_y, pattern_height),
            &steps_left, &steps_right, params->pattern_width,
            params->pattern_height, params->pattern);

    PatternEntry border_yarn_left;
    PatternEntry border_yarn_right;
    float border_yarn_size_left;
    float border_yarn_size_right;
    if (origin_entry.warp_above) {
        lookup_pattern_entry(&border_yarn_left, params, current_x,
                (current_y - steps_left - 1));        
        lookup_pattern_entry(&border_yarn_right, params, current_x,
                (current_y + steps_right + 1));        
        border_yarn_size_left = get_yarn_segment_size(current_x, 
                (current_y - steps_left - 1), params, intersection_data);
        border_yarn_size_right = get_yarn_segment_size(current_x, 
                (current_y + steps_right + 1), params, intersection_data);
    } else {
        lookup_pattern_entry(&border_yarn_left, params,
                (current_x - steps_left - 1), current_y);
        lookup_pattern_entry(&border_yarn_right, params,
                (current_x + steps_right + 1), current_y);
        border_yarn_size_left = get_yarn_segment_size(
                (current_x - steps_left - 1), current_y, params, intersection_data);
        border_yarn_size_right = get_yarn_segment_size(
                (current_x + steps_right + 1), current_y, params, intersection_data);
    }
    
    /*float border_yarn_size_left = params->
        yarn_types[border_yarn_left.yarn_type].yarnsize;
    float border_yarn_size_right = params->
        yarn_types[border_yarn_right.yarn_type].yarnsize;
        */
        
    /*float width = tl_yarn_type_get_yarnsize(params, origin_entry.yarn_type,
            intersection_data->context);*/
    float width = get_yarn_segment_size(origin_x, origin_y, params, intersection_data);
    float length = steps_left + steps_right + 1.f +
        ((1.f-border_yarn_size_left) + (1.f-border_yarn_size_right))/2.f;
    
    /*
    ========== TODO ========== !!!!!!!!!!!!!!!!!!!!!!
        Add variables so that we can sample using get_yarn_segment_size...
        origin_x and orign_y are repeating! Must use non reapeating coords to sample get_yarn_segment_size with!
        */

    //If current segment is between two parallel yarns. Do not count self.
    if(between_parallel) length -= 1;
    
    //Determine coordinates for top left corner of segment.
    float start_u, start_v;
    {
        float distance_left = steps_left + (1.f - border_yarn_size_left)/2.f;
        if (!between_parallel) distance_left += origin_offset;
        if (between_parallel && *cell_coord_across >= 0.5) {
            PatternEntry tmp_pe = params->pattern[
                tl_repeat_index(pattern_x, pattern_width) +
                tl_repeat_index(pattern_y, pattern_height)*pattern_width];
            distance_left = -(0.5f + tl_yarn_type_get_yarnsize(params, tmp_pe.yarn_type, intersection_data->context)/2.f);
        } 

        float distance_top = - (1.f-width)/2.f;

        if (!warp_above) {
            start_u = total_u + (-cell_x - distance_left)/pattern_width;
            start_v = total_v + (-cell_y - distance_top)/pattern_height;
        } else {
            start_u = total_u + (-cell_x - distance_top)/pattern_width;
            start_v = total_v + (-cell_y - distance_left)/pattern_height;
        }
    }

    tlYarnSegment yarn;
    yarn.length = length;
    yarn.width = width;
    yarn.start_u = start_u;
    yarn.start_v = start_v;
    yarn.warp_above = origin_entry.warp_above;
    yarn.pattern_entry = origin_entry;
    yarn.yarn_hit = yarn_hit;
    yarn.between_parallel = between_parallel;
    return yarn;
}

/*
 * Given intersection_data, tl_get_pattern_data determines the current yarn
 * segment and associated paramters needed for shading.
 * Determination of yarn segment is made more complicated by the fact that 
 * yarnsizes can vary. This results in a certain number of special cases.
 */
TL_FUNC_PREFIX tlPatternData tl_get_pattern_data(tlIntersectionData intersection_data,
        const tlWeaveParameters *params) {
	// NOTE(Vidar):Only allow pattern map in yarn type 0
	// TODO(Vidar): Should probably be a fabric parameter, but that requires more work... ;)
	tlYarnType yarn_type = params->yarn_types[0];
	if(yarn_type.normal_map_texmap && yarn_type.tangent_map_texmap){
		tlColor normal_col = tl_eval_texmap_color(yarn_type.normal_map_texmap, intersection_data.context);
		tlColor tangent_col = tl_eval_texmap_color(yarn_type.tangent_map_texmap, intersection_data.context);
		
		float normal_x = normal_col.r*2.f - 1.;
		float normal_y = normal_col.g*2.f - 1.;
		float normal_z = normal_col.b*2.f - 1.;
		
		float tangent_x = tangent_col.r*2.f - 1.;
		float tangent_y = tangent_col.g*2.f - 1.;
		float tangent_z = tangent_col.b*2.f - 1.;

		float  normal_m = sqrtf( normal_x* normal_x +  normal_y *  normal_y +  normal_z *  normal_z);
		float tangent_m = sqrtf(tangent_x*tangent_x + tangent_y * tangent_y + tangent_z * tangent_z);
		tlVector N = {  normal_x/normal_m ,  normal_y/normal_m ,  normal_z/normal_m , };
		tlVector T = { tangent_x/tangent_m, tangent_y/tangent_m, tangent_z/tangent_m, };
		tlVector B = tlVector_cross(N, T);

		//return the results
		tlPatternData ret_data;
		ret_data.u = asinf(T.z);
		ret_data.v = asinf(B.z);

		float tangent_2d_m = sqrtf(tangent_x*tangent_x + tangent_y * tangent_y);
		ret_data.sin_rot_angle = tangent_x / tangent_2d_m;
		ret_data.cos_rot_angle = tangent_y / tangent_2d_m;


		ret_data.yarn_hit = 1;
		ret_data.ext_between_parallel = 0;
		ret_data.yarn_type = 0;
		ret_data.length = 1.f; 
		ret_data.width  = 1.; 
		ret_data.x = ret_data.v/(float)M_PI_2; 
		ret_data.y = ret_data.u/(float)M_PI_2; 
		ret_data.warp_above = 0; 

		ret_data.total_index_x = intersection_data.uv_x*params->uscale;
		ret_data.total_index_y = intersection_data.uv_y*params->vscale;
		return ret_data;
	}
	else {
		if(params->pattern == 0){
			tlPatternData data = {0};
			return data;
		}
	   
		//Generate scaled uv coordinates from intersection_data
		float uv_x = intersection_data.uv_x;
		float uv_y = intersection_data.uv_y;
		float u_scale, v_scale;
		if (params->realworld_uv) {
			//the user parameters uscale, vscale change roles when realworld_uv
			// is true. If true they are then used to tweak the realworld scales
			u_scale = params->uscale/params->pattern_realwidth; 
			v_scale = params->vscale/params->pattern_realheight;
		} else {
			u_scale = params->uscale;
			v_scale = params->vscale;
		}

		// Apply uv rotation
		{
			float rot=params->uvrotation/180.f*(float)M_PI;
			float tmp_u=uv_x;
			float tmp_v=uv_y;
			uv_x=(tmp_u*cosf(rot)-tmp_v*sinf(rot))*u_scale;
			uv_y=(tmp_u*sinf(rot)+tmp_v*cosf(rot))*v_scale;
		}

		//scaled and non-repeating uv.
		float total_u = uv_x;
		float total_v = uv_y;

		float u_repeat = fmodf(uv_x,1.f);
		float v_repeat = fmodf(uv_y,1.f);
		if (u_repeat < 0.f) {
			u_repeat = u_repeat - floor(u_repeat);
		}
		if (v_repeat < 0.f) {
			v_repeat = v_repeat - floor(v_repeat);
		}
	//non-repeating pattern index (used for specular noise)
		uint32_t total_pattern_x = (uint32_t)((int32_t)(uv_x*params->pattern_width));
		uint32_t total_pattern_y = (uint32_t)((int32_t)(uv_y*params->pattern_height));

		//Get yarnsegment dimensions
		tlYarnSegment yarnsegment = tl_get_yarn_segment(total_u, total_v, params, &intersection_data);
		
		if (!yarnsegment.yarn_hit) {
			//No hit, No need to do more calculations.
			tlPatternData ret_data;
			ret_data.yarn_hit = 0;
			ret_data.yarn_type = 0;
			return ret_data;
		}

		float l = yarnsegment.length;
		float w = yarnsegment.width;
		float x, y;
		if (!yarnsegment.warp_above) {
			x = (total_u - yarnsegment.start_u)*params->pattern_width / l;
			y = (total_v - yarnsegment.start_v)*params->pattern_height / w;
		}
		else {
			x = (total_u - yarnsegment.start_u)*params->pattern_width / w;
			y = (total_v - yarnsegment.start_v)*params->pattern_height / l;
		}
		//Rescale x, y to [-1,1], w,v scaled by 2
		x = x*2.f - 1.f;
		y = y*2.f - 1.f;
		w = w*2.f;
		l = l*2.f;


		//Swap X and Y for warp, so that we always have the yarn going along y.
		if(!yarnsegment.warp_above){
			float tmp1 = x;
			x = -y;
			y = tmp1;
		}

		//return the results
		tlPatternData ret_data;
		ret_data.yarn_hit = yarnsegment.yarn_hit;
		ret_data.ext_between_parallel = yarnsegment.between_parallel;
		ret_data.yarn_type = yarnsegment.pattern_entry.yarn_type;
		ret_data.length = l; 
		ret_data.width  = w; 
		ret_data.x = x; 
		ret_data.y = y; 
		ret_data.warp_above = yarnsegment.warp_above; 
		calculate_segment_uv_and_normal(&ret_data, params, &intersection_data);
		ret_data.total_index_x = total_pattern_x; //total x index of wrapped pattern matrix
		ret_data.total_index_y = total_pattern_y; //total y index of wrapped pattern matrix

		if (yarnsegment.warp_above) {
			ret_data.sin_rot_angle = 1.f;
			ret_data.cos_rot_angle = 0.f;
		}
		else {
			ret_data.sin_rot_angle = 0.f;
			ret_data.cos_rot_angle = 1.f;
		}
		return ret_data;
	}
    
}

//NOTE(Vidar): From https://www.atnf.csiro.au/computing/software/gipsy/sub/bessel.c
TL_FUNC_PREFIX static double bessi0(double x)
/*------------------------------------------------------------*/
/* PURPOSE: Evaluate modified Bessel function In(x) and n=0.  */
/*------------------------------------------------------------*/
{
	double ax, ans;
	double y;


	if ((ax = fabs(x)) < 3.75) {
		y = x / 3.75, y = y * y;
		ans = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
			+ y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))));
	}
	else {
		y = 3.75 / ax;
		ans = (exp(ax) / sqrt(ax))*(0.39894228 + y * (0.1328592e-1
			+ y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2
				+ y * (-0.2057706e-1 + y * (0.2635537e-1 + y * (-0.1647633e-1
					+ y * 0.392377e-2))))))));
	}
	return ans;
}

TL_FUNC_PREFIX float logistic_distribution(float x, float _s)
{
    float s = _s*sqrtf(M_PI/8.f);
    float inv_norm = 1.f/(1.f+expf(-M_PI/s)) - 1.f/(1.f+expf(M_PI/s));
    float t = expf(-x/s);
    float val = t/(s*(1.f + t)*(1.f + t));
    return val/inv_norm;
}

TL_FUNC_PREFIX float fresnel(float prev_ior, float next_ior, float cos_theta_i)
{
	float sin_theta_i_2 = 1.f - cos_theta_i * cos_theta_i;
	float eta = prev_ior / next_ior;
	float sin_theta_i = sqrtf(sin_theta_i_2);
	float sin_theta_t = sin_theta_i * eta;
	if (sin_theta_t >= 1.f) return 1.f; //NOTE(Vidar): Total internal reflection
	float cos_theta_t = sqrtf(1.f - sin_theta_t*sin_theta_t);

	float r_parallel = (prev_ior*cos_theta_i - next_ior * cos_theta_t) /
		(prev_ior*cos_theta_i + next_ior * cos_theta_t);
	float r_perperdicular = (next_ior*cos_theta_i - prev_ior * cos_theta_t) /
		(next_ior*cos_theta_i + prev_ior * cos_theta_t);
	return (r_parallel*r_parallel + r_perperdicular * r_perperdicular)*0.5f;
}

TL_FUNC_PREFIX tlColor new_lighting(float u, float v, tlVector wi, tlVector wo, float spec_or_diff,
	const tlWeaveParameters *params ,void *context, int yarn_type)
{
	tlVector normal = tlVector_normalize(tlvector(sinf(v),
		sinf(u)*cosf(v), cosf(u)*cosf(v)));
	tlVector tangent = tlVector_normalize(tlvector(cosf(v), 
		-sinf(u)*sinf(v), -cosf(u)*sinf(v)));
	tlVector fiber_direction = tlVector_normalize(tlvector(0.f, 
		cosf(u), -sinf(u)));

	//TODO(Vidar): This should be the input parameters
	tlVector wi_fiber = tlvector(tlVector_dot(wi,tangent),
		tlVector_dot(wi,fiber_direction), tlVector_dot(wi,normal));
	tlVector wo_fiber = tlvector(tlVector_dot(wo,tangent),
		tlVector_dot(wo,fiber_direction), tlVector_dot(wo,normal));
	//TODO(Vidar): Eliminate h parameter?? Should be possible to replace it with normal direction I think
    float h = -v/M_PI_2;

	//TODO(Vidar):There should be a smarter way of doing this...
	float phi_i = atan2f(wi_fiber.x, wi_fiber.z);
	float phi_o = atan2f(wo_fiber.x, wo_fiber.z);
	float phi = phi_o - phi_i;

	float sin_theta_i = wi_fiber.y;
	float cos_theta_i = sqrtf(1.f - sin_theta_i * sin_theta_i);
	float sin_theta_o = wo_fiber.y;
	float cos_theta_o = sqrtf(1.f - sin_theta_o * sin_theta_o);

	float roughness_l = tl_yarn_type_get_roughness_longitudinal(params, yarn_type, context);
	//NOTE(Vidar):Mapping according to Chiang, Bitterli, Tappan and Burley  eq. (7)
	float variance_l = 0.726f*roughness_l + 0.812f*roughness_l*roughness_l + 3.7f*powf(roughness_l, 20.f);
	variance_l *= variance_l;

	float roughness_a = tl_yarn_type_get_roughness_azimuthal(params, yarn_type, context);
	float roughness_a2 = roughness_a * roughness_a;
	float roughness_a3 = roughness_a * roughness_a2;
	float roughness_a4 = roughness_a2 * roughness_a2;
	float roughness_a5 = roughness_a2 * roughness_a3;
	//NOTE(Vidar):Mapping according to Chiang, Bitterli, Tappan and Burley  eq. (8)
	float logistic_scale = 0.265f*roughness_a + 1.194*roughness_a2 + 5.372f*powf(roughness_a, 22.f);

	//Longitudinal scattering function (d.Eon sec. 4.2)
	//TODO(Vidar):Better csch
	float M_p = 
		expf(-sin_theta_i * sin_theta_o / variance_l)* bessi0(cos_theta_i*cos_theta_o / variance_l)
		/(sinhf(1.f / variance_l)*2.f*variance_l);

    //Azimuthal direction
    //Theta_d = (theta_r - theta_i)/2
    float cos_2theta_d = cos_theta_o*cos_theta_i - sin_theta_o*sin_theta_i;
    //TODO(Vidar):What about the minus sign in the half angle formula?
    float sin_theta_d_sq = (1-cos_2theta_d)/2;
    float cos_theta_d = sqrtf((1+cos_2theta_d)/2);

	float ior = tl_yarn_type_get_ior(params, yarn_type, context);
    float mod_ior = sqrtf(ior*ior - sin_theta_d_sq)/cos_theta_d;

	float N_p0 = 0.f;
	float N_p2 = 0.f;

    float gamma_i = asinf(h);
    float gamma_t = asinf(h/mod_ior);
	if((1)){
		float mode = 0.f; // R-0 TT-1 TRT-2 ...
		float phi_refl = 2.f*mode*gamma_t - 2.f*gamma_i + mode * M_PI;

		//TODO(Vidar):Make sure this angle is in the correct interval
		N_p0 = logistic_distribution(phi - phi_refl, logistic_scale);
	}
	{
		float mode = 2.f; // R-0 TT-1 TRT-2 ...
		float phi_refl = 2.f*mode*gamma_t - 2.f*gamma_i;// +mode * M_PI;

		//TODO(Vidar):Make sure this angle is in the correct interval
		N_p2 = logistic_distribution(phi - phi_refl, logistic_scale);
	}

	float sin_theta_t = sin_theta_i / ior;
	float cos_theta_t = sqrtf(1.f - sin_theta_t * sin_theta_t);
	tlColor color = tl_yarn_type_get_color(params, yarn_type, context);
	float absorption[3] = { color.r, color.g, color.b }; 
	for (int i = 0; i < 3; i++) {
		absorption[i] = logf(absorption[i]) / (5.969f - 0.215f*roughness_a + 2.532f*roughness_a2
			- 10.73f*roughness_a3 + 5.574f*roughness_a4 + 0.245f*roughness_a5);
		absorption[i] *= absorption[i];
		absorption[i] /= cos_theta_t;
	}

	float cos_twice_half_angle = tlVector_dot(wi, wo);
	float cos_half_angle = sqrtf(0.5f*(1.f + cos_twice_half_angle));

	float cos_gamma_o = sqrtf(1.f - h * h);
	float cos_transmission_angle = cos_theta_o * cos_gamma_o;
	float f = fresnel(1.f, ior, cos_transmission_angle);
	float f_compl = 1.f - f;
	float f_compl2 = f_compl*f_compl;

	float col[3] = { 0.f };
	for (int i = 0; i < 3; i++) {
		col[i] = M_p * N_p0  * f
			+ M_p * N_p2*expf(-2.f*absorption[i] * (1.f + cosf(2.f*gamma_t)))
			* f_compl2*f;
	}

	tlColor ret = { col[0], col[1], col[2] };

	return ret;
}

TL_FUNC_PREFIX tlColor tl_eval_diffuse(tlIntersectionData intersection_data,
        tlPatternData data, const tlWeaveParameters *params)
{

	tlYarnType *yarn_type = params->yarn_types + data.yarn_type;
    if(!yarn_type->color_enabled || !data.yarn_hit){
        yarn_type = &params->yarn_types[0];
    }

    tlColor color = {
        yarn_type->color.r,
        yarn_type->color.g,
        yarn_type->color.b
    };
	if(yarn_type->color_texmap){
		color = tl_eval_texmap_color(yarn_type->color_texmap,
            intersection_data.context);
	}

	tlVector wi = tlvector(intersection_data.wi_x, intersection_data.wi_y,
		intersection_data.wi_z);
	tlVector wo = tlvector(intersection_data.wo_x, intersection_data.wo_y,
		intersection_data.wo_z);

	float tmp_i = wi.x*data.cos_rot_angle + wi.y*data.sin_rot_angle;
	float tmp_o = wo.x*data.cos_rot_angle + wo.y*data.sin_rot_angle;
	wi.x = -wi.x*data.sin_rot_angle + wi.y*data.cos_rot_angle;
	wo.x = -wo.x*data.sin_rot_angle + wo.y*data.cos_rot_angle;
	wi.y = tmp_i;
	wo.y = tmp_o;

	color = new_lighting(data.u, data.v, wi, wo, 0.f, params, intersection_data.context, data.yarn_type);
	//TODO(Vidar):Temporarily set to 0
	float reflection = 0.f;
	color.r *= reflection;
	color.g *= reflection;
	color.b *= reflection;
    return color;
}

TL_FUNC_PREFIX tlColor tl_eval_specular(tlIntersectionData intersection_data,
        tlPatternData data, const tlWeaveParameters *params)
{
    tlColor ret={0.f,0.f,0.f};
    // Depending on the given psi parameter the yarn is considered
    // staple or filament. They are treated differently in order
    // to work better numerically. 
    float reflection = 0.f;
	if(!data.yarn_hit){
        //have not hit a yarn...
        return ret;
	}

	tlVector wi = tlvector(intersection_data.wi_x, intersection_data.wi_y,
		intersection_data.wi_z);
	tlVector wo = tlvector(intersection_data.wo_x, intersection_data.wo_y,
		intersection_data.wo_z);
	float tmp_i = wi.x*data.cos_rot_angle + wi.y*data.sin_rot_angle;
	float tmp_o = wo.x*data.cos_rot_angle + wo.y*data.sin_rot_angle;
	wi.x = -wi.x*data.sin_rot_angle + wi.y*data.cos_rot_angle;
	wo.x = -wo.x*data.sin_rot_angle + wo.y*data.cos_rot_angle;
	wi.y = tmp_i;
	wo.y = tmp_o;
	ret = new_lighting(data.u, data.v, wi, wo, 1.f, params, intersection_data.context, data.yarn_type);

	float specular_noise=tl_yarn_type_get_specular_noise(params,
		data.yarn_type,intersection_data.context);
	float noise=1.f;
    if(specular_noise > 0.001f){
		float iv = intensity_variation(data);
		noise=(1.f-specular_noise)+specular_noise * iv;
    }
	float factor = noise;
    ret.r*=factor;
    ret.g*=factor;
    ret.b*=factor;
    return ret;
}

TL_FUNC_PREFIX tlColor tl_shade(tlIntersectionData intersection_data,
        const tlWeaveParameters *params)
{
    tlPatternData data = tl_get_pattern_data(intersection_data,params);
    tlColor ret = tl_eval_diffuse(intersection_data,data,params);
    tlColor spec  = tl_eval_specular(intersection_data,data,params);
    ret.r+=spec.r; ret.g+=spec.g; ret.b+=spec.b;
    return ret;
}

#ifdef TL_NO_TEXTURE_CALLBACKS
TL_FUNC_PREFIX float tl_eval_texmap_mono(void *texmap, void *context)
{
    return 1.f;
}

TL_FUNC_PREFIX float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context)
{
    return 1.f;
}

TL_FUNC_PREFIX tlColor tl_eval_texmap_color(void *texmap, void *context)
{
    tlColor ret = {1.f,1.f,1.f};
    return ret;
}
#endif

#endif
