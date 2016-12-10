#include "woven_cloth.h"
#ifndef WC_NO_FILES
#define REALWORLD_UV_WIF_TO_MM 10.0
#include "wif/wif.cpp"
#include "wif/ini.cpp"
#endif

// For M_PI etc.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include <stdio.h>

// -- 3D Vector data structure -- //
typedef struct
{
    float x,y,z,w;
} wcVector;

static wcVector wcvector(float x, float y, float z)
{
    wcVector ret = {x,y,z,0.f};
    return ret;
}

static float wcVector_magnitude(wcVector v)
{
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static wcVector wcVector_normalize(wcVector v)
{
    wcVector ret;
    float inv_mag = 1.f/sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    ret.x = v.x * inv_mag;
    ret.y = v.y * inv_mag;
    ret.z = v.z * inv_mag;
    return ret;
}

static float wcVector_dot(wcVector a, wcVector b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static wcVector wcVector_cross(wcVector a, wcVector b)
{
    wcVector ret;
    ret.x = a.y*b.z - a.z*b.y;
    ret.y = a.z*b.x - a.x*b.z;
    ret.z = a.x*b.y - a.y*b.x;
    return ret;
}

static wcVector wcVector_add(wcVector a, wcVector b)
{
    wcVector ret;
    ret.x = a.x + b.x;
    ret.y = a.y + b.y;
    ret.z = a.z + b.z;
    return ret;
}

// -- //

//atof equivalent function wich is not dependent
//on local. Heavily inspired by inplementation in K & R.
static double str2d(char s[]) {
    double val, power;
    int i, sign;

    for (i = 0; isspace(s[i]); i++)
        ;
    sign = (s[i] == '-') ? -1 : 1;
    if (s[i] == '+' || s[i] == '-')
        i++;
    for (val = 0.0; isdigit(s[i]); i++)
        val = 10.0 * val + (s[i] - '0');
    if (s[i] == '.')
        i++;
    for (power = 1.0; isdigit(s[i]); i++) {
        val = 10.0 * val + (s[i] - '0');
        power *= 10.0;
    }
    return sign * val / power;
}

static float wcClamp(float x, float min, float max)
{
    return (x < min) ? min : (x > max) ? max : x;
}

/* Tiny Encryption Algorithm by David Wheeler and Roger Needham */
/* Taken from mitsuba source code. */
static uint64_t sampleTEA(uint32_t v0, uint32_t v1, int rounds)
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
static float sampleTEASingle(uint32_t v0, uint32_t v1, int rounds)
{
    /* Trick from MTGP: generate an uniformly distributed
    single precision number in [1,2) and subtract 1. */
    union {
        uint32_t u;
        float f;
    } x;
    x.u = ((sampleTEA(v0, v1, rounds) & 0xFFFFFFFF) >> 9) | 0x3f800000UL;
    return x.f - 1.0f;
}

void sample_cosine_hemisphere(float sample_x, float sample_y, float *p_x,
        float *p_y, float *p_z)
{
    //sample uniform disk concentric
    // From mitsuba warp.cpp
	float r1 = 2.0f*sample_x - 1.0f;
	float r2 = 2.0f*sample_y - 1.0f;

// Modified concencric map code with less branching (by Dave Cline), see
// http://psgraphics.blogspot.ch/2011/01/improved-code-for-concentric-map.html
	float phi, r;
	if (r1 == 0 && r2 == 0) {
		r = phi = 0;
	} else if (r1*r1 > r2*r2) {
		r = r1;
		phi = (M_PI/4.0f) * (r2/r1);
	} else {
		r = r2;
		phi = (M_PI_2) - (r1/r2) * (M_PI/4.0f);
	}

    *p_x = r * cosf(phi);
    *p_y = r * sinf(phi);
    *p_z = sqrtf(1.0f - (*p_x)*(*p_x) - (*p_y)*(*p_y));
}

void sample_uniform_hemisphere(float sample_x, float sample_y, float *p_x,
        float *p_y, float *p_z)
{
    //Source: http://mathworld.wolfram.com/SpherePointPicking.html
    float theta = M_PI*2.f*sample_x;
    float phi   = acos(2.f*sample_y - 1.f);
    *p_x = cosf(theta)*cosf(phi);
    *p_y = sinf(theta)*cosf(phi);
    *p_z = sinf(phi);
}

void calculate_segment_uv_and_normal(wcPatternData *pattern_data,
        const wcWeaveParameters *params,wcIntersectionData *intersection_data)
{
    //Calculate the yarn-segment-local u v coordinates along the curved cylinder
    //NOTE: This is different from how Irawan does it
    /*segment_u = asinf(x*sinf(params->umax));
        segment_v = asinf(y);*/
    //TODO(Vidar): Use a parameter for choosing model?
    float umax;
    if (pattern_data->ext_between_parallel == 1){
        //if segment is extension between to parallel yarns -> bend = 0
        umax = 0.0001;
    } else {
        umax = wc_yarn_type_get_umax(params,
                pattern_data->yarn_type,
                intersection_data->context);
    }
    float segment_u = pattern_data->y*umax;
    float segment_v = pattern_data->x*M_PI_2;

    //Calculate the normal in yarn-local coordinates
    float normal_x = sinf(segment_v);
    float normal_y = sinf(segment_u)*cosf(segment_v);
    float normal_z = cosf(segment_u)*cosf(segment_v);
    
    //TODO(Vidar): This is pretty weird, right? Now x and y are not yarn-local
    // anymore...
    // Transform the normal back to shading space
    if(!pattern_data->warp_above){
        float tmp = normal_x;
        normal_x  = normal_y;
        normal_y  = -tmp;
    }

    pattern_data->u        = segment_u;
    pattern_data->v        = segment_v;
    pattern_data->normal_x = normal_x;
    pattern_data->normal_y = normal_y;
    pattern_data->normal_z = normal_z;
}


static const int halton_4_base[] = {2, 3, 5, 7};
static const float halton_4_base_inv[] =
        {1.f/2.f, 1.f/3.f, 1.f/5.f, 1.f/7.f};

//Sets the elements of val to the n-th 4-dimensional point
//in the Halton sequence
static void halton_4(int n, float val[]){
    int j;
    for(j=0;j<4;j++){
        float f = 1.f;
        int i = n;
        val[j] = 0.f;
        while(i>0){
            f = f * halton_4_base_inv[j];
            val[j] = val[j] + f * (i % halton_4_base[j]);
            i = i / halton_4_base[j];
        }
    }
}


void wcFinalizeWeaveParameters(wcWeaveParameters *params)
{
    //Calculate normalization factor for the specular reflection
	if (params->pattern) {
		size_t nLocationSamples = 100;
		size_t nDirectionSamples = 1000;
		params->specular_normalization = 1.f;

		float highest_result = 0.f;

		// Temporarily disable speuclar noise...
		float tmp_specular_noise[WC_MAX_YARN_TYPES];
		for(int i=0;i<params->num_yarn_types;i++){
			tmp_specular_noise[i] = params->yarn_types[i].specular_noise;
			params->yarn_types[i].specular_noise = 0.f;
		}

		// Normalize by the largest reflection across all uv coords and
		// incident directions
		for( uint32_t yarn_type = 0; yarn_type < params->num_yarn_types;
			yarn_type++){
			for (size_t i = 0; i < nLocationSamples; i++) {
				float result = 0.0f;
				float halton_point[4];
				halton_4(i + 50, halton_point);
				wcPatternData pattern_data;
				// Pick a random location on a segment rectangle...
				pattern_data.x = -1.f + 2.f*halton_point[0];
				pattern_data.y = -1.f + 2.f*halton_point[1];
				pattern_data.length = 1.f;
				pattern_data.width = 1.f;
				pattern_data.warp_above = 0;
				pattern_data.yarn_type = yarn_type;
				pattern_data.yarn_hit = 1;
				wcIntersectionData intersection_data;
				intersection_data.context=0;
				calculate_segment_uv_and_normal(&pattern_data, params,
					&intersection_data);
				pattern_data.total_index_x = 0;
				pattern_data.total_index_y = 0;

				sample_uniform_hemisphere(halton_point[2], halton_point[3],
					&intersection_data.wi_x, &intersection_data.wi_y,
					&intersection_data.wi_z);

				for (size_t j = 0; j < nDirectionSamples; j++) {
					float halton_direction[4];
					halton_4(j + 50 + nLocationSamples, halton_direction);
					// Since we use cosine sampling here, we can ignore the cos term
					// in the integral
					sample_cosine_hemisphere(halton_direction[0], halton_direction[1],
						&intersection_data.wo_x, &intersection_data.wo_y,
						&intersection_data.wo_z);
					result += wcEvalSpecular(intersection_data, pattern_data, params);
				}
				if (result > highest_result) {
					highest_result = result;
				}
			}
		}

		if (highest_result <= 0.0001f) {
			params->specular_normalization = 0.f;
		}
		else {
			params->specular_normalization =
				(float)nDirectionSamples / highest_result;
		}
		for(int i=0;i<params->num_yarn_types;i++){
			params->yarn_types[i].specular_noise = tmp_specular_noise[i];
		}
	}
}

wcWeaveParameters *wcWeavePatternFromData(uint8_t *warp_above, uint8_t *yarn_type,
    uint32_t num_yarn_types, wcColor *yarn_colors, uint32_t pattern_width,
    uint32_t pattern_height)
{
    wcWeaveParameters *params =
        (wcWeaveParameters*)calloc(sizeof(wcWeaveParameters),1);
    params->pattern_width = pattern_width;
    params->pattern_height = pattern_height;
    num_yarn_types++;
    params->num_yarn_types = num_yarn_types;
    params->yarn_types = (wcYarnType*)calloc(sizeof(wcYarnType),num_yarn_types);
    params->yarn_types[0] = wc_default_yarn_type;
    for(int i=1;i<num_yarn_types;i++){
        params->yarn_types[i] = wc_default_yarn_type;
        params->yarn_types[i].color = yarn_colors[i-1];
        params->yarn_types[i].color_enabled = 1;
    }
    uint32_t pattern_size = pattern_width*pattern_height;
    params->pattern = (PatternEntry*)calloc(sizeof(PatternEntry),pattern_size);
    for(int i=0;i<pattern_size;i++){
        params->pattern[i].warp_above = warp_above[i];
        params->pattern[i].yarn_type = yarn_type[i];
    }
    return params;
}

#ifndef WC_NO_FILES

wcWeaveParameters *wcWeavePatternFromFile(const char *filename,const char **error)
{
	wcWeaveParameters *param = 0;
	int len = 0;
	while(filename[len] != 0){
		len++;
	}
	if(len >=5){
		bool wif_ok = true;
		bool ptn_ok = true;
		const char *wif_ext = ".wif";
		const char *ptn_ext = ".ptn";
		for(int i=0;i<4;i++){
			char c = filename[len-1-i];
			c = (c <= 'Z' && c >= 'A') ? c + 32 : c;
			if(c != wif_ext[3-i]){
				wif_ok = false;
			}
			if(c != ptn_ext[3-i]){
				ptn_ok = false;
			}
        }   
        if(wif_ok || ptn_ok){
            FILE *fp;
            if(wif_ok){
             fp = fopen(filename,"rt");
            }
            if(ptn_ok){
             fp = fopen(filename,"rb");
            }
            fseek(fp,0,SEEK_END);
            long len = ftell(fp);
            fseek(fp,0,SEEK_SET);
            unsigned char *data = (unsigned char*)calloc(len,1);
            fread(data,1,len,fp);
            fclose(fp);
            if(wif_ok){
                param = wcWeavePatternFromWIF(data,len,error);
            }
            if(ptn_ok){
                param = wcWeavePatternFromPTN(data,len,error);
            }
            free(data);
		}
	}
	return param;
}
#ifdef WC_WCHAR
wcWeaveParameters *wcWeavePatternFromFile_wchar(const wchar_t *filename,const char **error)
{
	wcWeaveParameters *param = 0;
	int len = 0;
	while(filename[len] != 0){
		len++;
	}
	if(len >=5){
		bool wif_ok = true;
		bool ptn_ok = true;
		wchar_t *wif_ext = L".wif";
		wchar_t *ptn_ext = L".ptn";
		for(int i=0;i<4;i++){
			wchar_t c = filename[len-1-i];
			c = towlower(c);
			if(c != wif_ext[3-i]){
				wif_ok = false;
			}
			if(c != ptn_ext[3-i]){
				ptn_ok = false;
			}
		}
		if(wif_ok || ptn_ok){
			FILE *fp;
			if(wif_ok){
			 fp = _wfopen(filename,L"rt");
			}
			if(ptn_ok){
			 fp = _wfopen(filename,L"rb");
			}
			fseek(fp,0,SEEK_END);
			long len = ftell(fp);
			fseek(fp,0,SEEK_SET);
			unsigned char *data = (unsigned char*)calloc(len,1);
			fread(data,1,len,fp);
			fclose(fp);
			if(wif_ok){
				param = wcWeavePatternFromWIF(data,len,error);
			}
			if(ptn_ok){
				param = wcWeavePatternFromPTN(data,len,error);
			}
			free(data);
		}else{
			*error = "Unknown file format";
		}
	}else{
		*error = "Unknown file format";
	}
	return param;
}
#endif

wcWeaveParameters *wcWeavePatternFromWIF(unsigned char *data,long len,const char **error)
{
    WeaveData *weave_data = wif_read((char*)data,len,error);
    if(weave_data){
        wcWeaveParameters *params = (wcWeaveParameters*)calloc(sizeof(wcWeaveParameters),1);
        wif_get_pattern(params, weave_data,
            &params->pattern_width, &params->pattern_height,
            &params->pattern_realwidth, &params->pattern_realheight);
        wif_free_weavedata(weave_data);
        return params;
    }
    return 0;
}
#endif

static wcWeaveParameters *wc_pattern_from_ptn_file_v1(unsigned char *data,long len,const char **error){
#define WC_PTN_READ_ENTRY(name,type,num)\
	if(len < sizeof(type)*num){ *error = "unexpected end of data"; return 0; }\
	memcpy(name,data,sizeof(type)*num);\
	data += sizeof(type)*num; len -= sizeof(type)*num;

	wcWeaveParameters *param = (wcWeaveParameters*)calloc(sizeof(wcWeaveParameters),1);
	WC_PTN_READ_ENTRY(param,wcWeaveParameters,1);

	param->yarn_types = (wcYarnType*)calloc(sizeof(wcYarnType),param->num_yarn_types);
	WC_PTN_READ_ENTRY(param->yarn_types,wcYarnType,param->num_yarn_types);

	int pattern_size = param->pattern_width*param->pattern_height;
	param->pattern = (PatternEntry*)calloc(sizeof(PatternEntry),pattern_size);
	WC_PTN_READ_ENTRY(param->pattern,PatternEntry,pattern_size);

#undef WC_PTN_READ_ENTRY
	return param;
}

wcWeaveParameters *wcWeavePatternFromPTN(unsigned char *data,long len,const char **error){
	wcWeaveParameters *param = 0;
	int version = 0;
	memcpy(&version,data,sizeof(int));\
	data += sizeof(int);
	len -= sizeof(int);
	switch(version){
	case 1:
		param = wc_pattern_from_ptn_file_v1(data,len,error);
		break;
	default:
		*error = "Unknown PTN file version";
		return 0;
	}
	return param;
}

void wcFreeWeavePattern(wcWeaveParameters *params)
{
    if (params->yarn_types) {
        free(params->yarn_types);
    }
    if (params->pattern) {
        free(params->pattern);
    }
}

static float intensityVariation(wcPatternData pattern_data)
{
	//NOTE(Vidar): a fineness of 3 seems to work fine...
	float intensity_fineness=3.f;

    // have index to make a grid of finess*fineness squares 
    // of which to have the same brightness variations.

    uint32_t tindex_x = pattern_data.total_index_x;
    uint32_t tindex_y = pattern_data.total_index_y;

    //Switch X and Y for warp, so that we have the yarn going along y
    if(!pattern_data.warp_above){
        float tmp = tindex_x;
        tindex_x = tindex_y;
        tindex_y = tmp;
    }

    //segment start x,y
    float centerx = tindex_x - (pattern_data.x*0.5f)*pattern_data.width;
    float centery = tindex_y - (pattern_data.y*0.5f)*pattern_data.length;
    
    uint32_t r1 = (uint32_t) ((centerx + tindex_x) 
            * intensity_fineness);
    uint32_t r2 = (uint32_t) ((centery + tindex_y) 
            * intensity_fineness);
    
    float xi = sampleTEASingle(r1, r2, 8);
    float log_xi = -logf(xi);
    return log_xi < 10.f ? log_xi : 10.f;
}

static void calculateLengthOfSegment(uint8_t warp_above, uint32_t pattern_x,
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

static float vonMises(float cos_x, float b) {
    // assumes a = 0, b > 0 is a concentration parameter.
    float I0, absB = fabsf(b);
    if (fabsf(b) <= 3.75f) {
        float t = absB / 3.75f;
        t = t * t;
        I0 = 1.0f + t*(3.5156229f + t*(3.0899424f + t*(1.2067492f
            + t*(0.2659732f + t*(0.0360768f + t*0.0045813f)))));
    } else {
        float t = 3.75f / absB;
        I0 = expf(absB) / sqrtf(absB) * (0.39894228f + t*(0.01328592f
            + t*(0.00225319f + t*(-0.00157565f + t*(0.00916281f
            + t*(-0.02057706f + t*(0.02635537f + t*(-0.01647633f
            + t*0.00392377f))))))));
    }

    return expf(b * cos_x) / (2 * M_PI * I0);
}

void lookupPatternEntry(PatternEntry* entry, const wcWeaveParameters* params, const int8_t x, const int8_t y) {
    //function to get pattern entry. Takes care of coordinate wrapping!
    int32_t tmpx = (int32_t)fmod(x,(float)params->pattern_width);
    int32_t tmpy = (int32_t)fmod(y,(float)params->pattern_height);
    if (tmpx < 0.f) {
        tmpx = params->pattern_width + tmpx;
    }
    if (tmpy < 0.f) {
        tmpy = params->pattern_height + tmpy;
    }
    *entry = params->pattern[tmpx + tmpy*params->pattern_width];
}

int32_t wcRepeatIndex(const int32_t coord, const int32_t size) {
    //coord = -0.124*2 = -0.248
    float tmpcoord = (float)fmod((float)coord, (float)size);
    if (tmpcoord < 0.f) {
        tmpcoord = size + tmpcoord;
    }
    return (int32_t)tmpcoord;
}

static float getYarnSegmentSize(int32_t total_pattern_x, int32_t total_pattern_y,
		const wcWeaveParameters *params, const wcIntersectionData *intersection_data) {
        //remove scaling from coords
		float lookup_u = (total_pattern_x + 0.5)/((float)(params->pattern_width)*params->uscale);
		float lookup_v = (total_pattern_y + 0.5)/((float)(params->pattern_height)*params->vscale);
		
        PatternEntry yrntype;
		lookupPatternEntry(&yrntype, params, total_pattern_x, total_pattern_y);

		//Sample potential yarnsize texmap in middle of cell
        float size = wc_yarn_type_get_lookup_yarnsize(params, yrntype.yarn_type,
                lookup_u, lookup_v, intersection_data->context);
		//float size = wc_yarn_type_get_yarnsize(params, yrntype.yarn_type, intersection_data->context);
		return size;
	};

wcYarnSegment wcGetYarnSegment(float total_u, float total_v,
		const wcWeaveParameters *params, const wcIntersectionData *intersection_data) {
    //total_u and total_v are scaled and non_repeating

    float u = fmod(total_u,1.f);
    float v = fmod(total_v,1.f);
    if (u < 0.f) {
        u = u - floor(u);
    }
    if (v < 0.f) {
        v = v - floor(v);
    }

    //TODO(Peter): Remove uneccessary variables...

    PatternEntry *pattern_entries = params->pattern;
    uint32_t pattern_width = params->pattern_width;
    uint32_t pattern_height = params->pattern_height;
    //TODO fmod repeat uvs, to avoiding the u or v == 1 check.
    int32_t pattern_x = (int32_t)floor((total_u*(float)(pattern_width)));
    int32_t pattern_y = (int32_t)floor((total_v*(float)(pattern_height)));
    
    //non-repeating pattern index (used for specular noise)
    //int32_t total_pattern_x = total_u*pattern_width;
    //int32_t total_pattern_y = total_v*pattern_height;
    
    int32_t pattern_repeat_x = wcRepeatIndex(pattern_x, pattern_width);
    int32_t pattern_repeat_y = wcRepeatIndex(pattern_y, pattern_height);
    //int32_t pattern_repeat_x = (u == 1.f) ? pattern_width-1 : (uint32_t)(u*(pattern_width));
    //int32_t pattern_repeat_y = (v == 1.f) ? pattern_height-1 : (uint32_t)(v*(pattern_height));

    //The origin is the pattern entry from which size of segment is calculated. 
    //The origin entry changes if we miss a thin yarn.
    int32_t origin_x = pattern_x;
    int32_t origin_y = pattern_y;
    PatternEntry origin_entry = params->pattern[pattern_repeat_x + 
        pattern_repeat_y*pattern_width];
    uint8_t warp_above = origin_entry.warp_above;

    float unscaled_u = intersection_data->uv_x;
    float unscaled_v = intersection_data->uv_y;
    //float toffset_u = unscaled_u - u/params->uscale;
    //float toffset_v = unscaled_v - v/params->vscale;
    
    int32_t current_x = origin_x;
    int32_t current_y = origin_y;
    int32_t *incremented_coord_along = warp_above ? &current_y : &current_x;
    int32_t *incremented_coord_across = warp_above ? &current_x : &current_y;
    float cell_x = (u == 1.f) ? 1 : u*(float)(pattern_width) - pattern_repeat_x;
    float cell_y = (v == 1.f) ? 1 : v*(float)(pattern_height) - pattern_repeat_y;
    float *cell_coord_along = warp_above ? &cell_y : &cell_x;
    float *cell_coord_across = warp_above ? &cell_x : &cell_y;
    uint32_t max_size_along = warp_above ? pattern_height: pattern_width;
    uint32_t max_size_across = warp_above ? pattern_width: pattern_height;
    uint32_t initial_coord_along = warp_above ? pattern_y: pattern_x;
    uint32_t initial_coord_across = warp_above ? pattern_x: pattern_y;

    //Get segment size of yarn in current position in pattern matrix.
	//float yarnsize = getYarnSegmentSize(total_pattern_x, total_pattern_y, params, intersection_data);
    float origin_yarnsize = getYarnSegmentSize(origin_x,
            origin_y, params, intersection_data);
    
    //init some flags
    uint8_t yarn_hit = 0;			//Have we hit the yarn?
    uint8_t between_parallel = 0;	//for later, are we between parallel yarns?
    uint8_t extension = 0;          //is what we hit an extention of an adjasent yarn?
    int32_t origin_offset = 0;      //With how much is the origin offset?
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
            lookupPatternEntry(&tmp_pe, params, current_x, current_y);
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
        float tmp_yarnsize = getYarnSegmentSize(current_x,
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
    calculateLengthOfSegment(origin_entry.warp_above,
            wcRepeatIndex(origin_x, pattern_width),
            wcRepeatIndex(origin_y, pattern_height),
            &steps_left, &steps_right, params->pattern_width,
            params->pattern_height, params->pattern);

    PatternEntry border_yarn_left;
    PatternEntry border_yarn_right;
    float border_yarn_size_left;
    float border_yarn_size_right;
    if (origin_entry.warp_above) {
        lookupPatternEntry(&border_yarn_left, params, current_x,
                (current_y - steps_left - 1));        
        lookupPatternEntry(&border_yarn_right, params, current_x,
                (current_y + steps_right + 1));        
        border_yarn_size_left = getYarnSegmentSize(current_x, 
                (current_y - steps_left - 1), params, intersection_data);
        border_yarn_size_right = getYarnSegmentSize(current_x, 
                (current_y + steps_right + 1), params, intersection_data);
    } else {
        lookupPatternEntry(&border_yarn_left, params,
                (current_x - steps_left - 1), current_y);
        lookupPatternEntry(&border_yarn_right, params,
                (current_x + steps_right + 1), current_y);
        border_yarn_size_left = getYarnSegmentSize(
                (current_x - steps_left - 1), current_y, params, intersection_data);
        border_yarn_size_right = getYarnSegmentSize(
                (current_x + steps_right + 1), current_y, params, intersection_data);
    }
    
    /*float border_yarn_size_left = params->
        yarn_types[border_yarn_left.yarn_type].yarnsize;
    float border_yarn_size_right = params->
        yarn_types[border_yarn_right.yarn_type].yarnsize;
        */
        
    /*float width = wc_yarn_type_get_yarnsize(params, origin_entry.yarn_type,
            intersection_data->context);*/
    float width = getYarnSegmentSize(origin_x, origin_y, params, intersection_data);
    float length = steps_left + steps_right + 1.f +
        ((1.f-border_yarn_size_left) + (1.f-border_yarn_size_right))/2.f;
    
    /*
    ========== TODO ========== !!!!!!!!!!!!!!!!!!!!!!
        Add variables so that we can sample using getYarnSegmentSize...
        origin_x and orign_y are repeating! Must use non reapeating coords to sample getYarnSegmentSize with!
        */

    //If current segment is between two parallel yarns. Do not count self.
    if(between_parallel) length -= 1;
    
    /* Debug */
    /*
    printf("--Debug calculateSegment--\n");
    printf("steps_left: %d, steps_right: %d\n", steps_left, steps_right);
    printf("border_yarn_type_left: %d, border_yarn_type_right: %d\n", border_yarn_left.yarn_type, border_yarn_right.yarn_type);
    printf("border_yarn_size_left: %f, border_yarn_size_right: %f\n", border_yarn_size_left, border_yarn_size_right);
    printf("border_yarn_left warp_above: %d, border_yarn_right warp_above: %d\n", border_yarn_left.warp_above, border_yarn_right.warp_above);
    printf("tmp width: %f, length: %f \n", width, length);
    printf("between parallel: %d\n", between_parallel);
    printf("cell_x: %f, cell_y: %f \n", cell_x, cell_y);
    printf("pattern_repeat_x: %d, pattern_repeat_y: %d \n", pattern_repeat_x, pattern_repeat_y);
    printf("total_u: %f, total_v: %f \n", total_u, total_v);
    printf("u: %f, v: %f \n", u, v);
    */

    //Determine coordinates for top left corner of segment.
    float start_u, start_v;
    {
        float distance_left = steps_left + (1.f - border_yarn_size_left)/2.f;
        if (!between_parallel) distance_left += origin_offset;
        if (between_parallel && *cell_coord_across >= 0.5) {
            PatternEntry tmp_pe = params->pattern[
                wcRepeatIndex(pattern_x, pattern_width) +
                wcRepeatIndex(pattern_y, pattern_height)*pattern_width];
            //distance_left = -(0.5 + wc_yarn_type_get_yarnsize(params, tmp_pe.yarn_type, intersection_data->context)/2.f);
            distance_left = -(0.5 + wc_yarn_type_get_yarnsize(params, tmp_pe.yarn_type, intersection_data->context)/2.f);
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

    wcYarnSegment yarn;
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
 * Given intersection_data, wcGetPatternData determines the current yarn
 * segment and associated paramters needed for shading.
 * Determination of yarn segment is made more complicated by the fact that 
 * yarnsizes can vary. This results in a certain number of special cases.
 */

wcPatternData wcGetPatternData(wcIntersectionData intersection_data,
        const wcWeaveParameters *params) {
    if(params->pattern == 0){
        wcPatternData data = {0};
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

    //scaled and non-repeating uv patterns.
    float total_u = uv_x*u_scale;
    float total_v = uv_y*v_scale;

    float u_repeat = fmod(uv_x*u_scale,1.f);
    float v_repeat = fmod(uv_y*v_scale,1.f);
    if (u_repeat < 0.f) {
        u_repeat = u_repeat - floor(u_repeat);
    }
    if (v_repeat < 0.f) {
        v_repeat = v_repeat - floor(v_repeat);
    }
    //non-repeating pattern index (used for specular noise)
    uint32_t total_pattern_x = uv_x*u_scale*params->pattern_width;
    uint32_t total_pattern_y = uv_y*v_scale*params->pattern_height;
    //pattern index
    uint32_t pattern_x = (uint32_t)(u_repeat*(float)(params->pattern_width));
    uint32_t pattern_y = (uint32_t)(v_repeat*(float)(params->pattern_height));
    //Coordinate within the current pattern cell
    float cell_x = ((u_repeat*(float)(params->pattern_width) - (float)pattern_x));
    float cell_y = ((v_repeat*(float)(params->pattern_height) - (float)pattern_y));

    //Get yarnsegment dimensions
	wcYarnSegment yarnsegment = wcGetYarnSegment(total_u, total_v, params, &intersection_data);
    
    if (!yarnsegment.yarn_hit) {
        //No hit, No need to do more calculations.
        wcPatternData ret_data;
        ret_data.yarn_hit = 0;
        ret_data.yarn_type = 0;
        return ret_data;
    }

    float l = yarnsegment.length;
	float w = yarnsegment.width;
    float x, y;
    if (!yarnsegment.warp_above) {
        x = (total_u - yarnsegment.start_u)*params->pattern_width/l;
        y = (total_v - yarnsegment.start_v)*params->pattern_height/w;
    } else {
        x = (total_u - yarnsegment.start_u)*params->pattern_width/w;
        y = (total_v - yarnsegment.start_v)*params->pattern_height/l;
    }
    
    //Rescale x, y to [-1,1], w,v scaled by 2
    x = x*2.f - 1.f;
    y = y*2.f - 1.f;
    w = w*2.f;
    l = l*2.f;

    /* Debug */
    /*
    printf("yarnsegment.width: %f, yarnsegment.length: %f\n", yarnsegment.width, yarnsegment.length);
    printf("w: %f, l: %f\n", w, l);
    printf("x: %f, y: %f\n", x, y);
    printf("v_repeat: %f, u_repeat: %f\n", v_repeat, u_repeat);
    printf("total_u - yarnsegment.start_u: %f - %f = %f\n", total_u, yarnsegment.start_u, (total_u - yarnsegment.start_u));
    printf("total_v - yarnsegment.start_v: %f - %f = %f\n", total_v, yarnsegment.start_v, (total_v - yarnsegment.start_v));
    printf("pattern_x: %d, pattern_y: %d\n", pattern_x, pattern_y);
    */

    //Swap X and Y for warp, so that we always have the yarn going along y.
    if(!yarnsegment.warp_above){
        float tmp1 = x;
        x = -y;
        y = tmp1;
    }

    //return the results
    wcPatternData ret_data;
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
    return ret_data;
}

float wcEvalFilamentSpecular(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params)
{
    wcVector wi = wcvector(intersection_data.wi_x, intersection_data.wi_y,
        intersection_data.wi_z);
    wcVector wo = wcvector(intersection_data.wo_x, intersection_data.wo_y,
        intersection_data.wo_z);

    if(!data.warp_above){
        float tmp2 = wi.x;
        float tmp3 = wo.x;
        wi.x = -wi.y; wi.y = tmp2;
        wo.x = -wo.y; wo.y = tmp3;
    }
    wcVector H = wcVector_normalize(wcVector_add(wi,wo));

    float v = data.v;
    float y = data.y;

    //TODO(Peter): explain from where these expressions come.
    //compute v from x using (11). Already done. We have it from data.
    //compute u(wi,v,wr) -- u as function of v. using (4)...
    float specular_u = atan2f(-H.z, H.y) + M_PI_2; //plus or minus in last t.
    //TODO(Peter): check that it indeed is just v that should be used 
    //to calculate Gu (6) in Irawans paper.
    //calculate yarn tangent.

    float reflection = 0.f;
    float umax;
    if (data.ext_between_parallel == 1){
        //if segment is extension between to parallel yarns -> bend = 0
        umax = 0.0001;
    } else {
        umax = wc_yarn_type_get_umax(params,data.yarn_type,
                intersection_data.context);
    }
    
    if (fabsf(specular_u) < umax){
        // Make normal for highlights, uses v and specular_u
        wcVector highlight_normal = wcVector_normalize(wcvector(sinf(v),
                    sinf(specular_u)*cosf(v),
                    cosf(specular_u)*cosf(v)));

        // Make tangent for highlights, uses v and specular_u
        wcVector highlight_tangent = wcVector_normalize(wcvector(0.f, 
                    cosf(specular_u), -sinf(specular_u)));

        //get specular_y, using irawans transformation.
        float specular_y = specular_u/umax;
        // our transformation TODO(Peter): Verify!
        //float specular_y = sinf(specular_u)/sinf(m_umax);

        float delta_x = wc_yarn_type_get_delta_x(params, data.yarn_type,
			intersection_data.context);
        //Clamp specular_y TODO(Peter): change name of m_delta_x to m_delta_h
        specular_y = specular_y < 1.f - delta_x ? specular_y :
            1.f - delta_x;
        specular_y = specular_y > -1.f + delta_x ? specular_y :
            -1.f + delta_x;

        //this takes the role of xi in the irawan paper.
        if (fabsf(specular_y - y) < delta_x) {
            // --- Set Gu, using (6)
            float a = 1.f; //radius of yarn
            float R = 1.f/(sin(umax)); //radius of curvature
            float Gu = a*(R + a*cosf(v)) /(
                wcVector_magnitude(wcVector_add(wi,wo)) *
                fabsf((wcVector_cross(highlight_tangent,H)).x));

            float alpha = wc_yarn_type_get_alpha(params, data.yarn_type,
				intersection_data.context);
            float beta = wc_yarn_type_get_beta(params, data.yarn_type,
				intersection_data.context);
            // --- Set fc
            float cos_x = -wcVector_dot(wi, wo);
            float fc = alpha + vonMises(cos_x, beta);

            // --- Set A
            float widotn = wcVector_dot(wi, highlight_normal);
            float wodotn = wcVector_dot(wo, highlight_normal);
            widotn = (widotn < 0.f) ? 0.f : widotn;   
            wodotn = (wodotn < 0.f) ? 0.f : wodotn;   
            float A = 0.f;
            if(widotn > 0.f && wodotn > 0.f){
                A = 1.f / (4.0 * M_PI) * (widotn*wodotn)/(widotn + wodotn);
                //TODO(Peter): Explain from where the 1/4*PI factor comes from
            }
            float l = 2.f;
            //TODO(Peter): Implement As, -- smoothes the dissapeares of the
            // higlight near the ends. Described in (9)
            reflection = 2.f*l*umax*fc*Gu*A/delta_x;
        }
    }
    return reflection;
}

float wcEvalStapleSpecular(wcIntersectionData intersection_data,
    wcPatternData data, const wcWeaveParameters *params)
{
    wcVector wi = wcvector(intersection_data.wi_x, intersection_data.wi_y,
        intersection_data.wi_z);
    wcVector wo = wcvector(intersection_data.wo_x, intersection_data.wo_y,
        intersection_data.wo_z);

    if(!data.warp_above){
        float tmp2 = wi.x;
        float tmp3 = wo.x;
        wi.x = -wi.y; wi.y = tmp2;
        wo.x = -wo.y; wo.y = tmp3;
    }
    wcVector H = wcVector_normalize(wcVector_add(wi, wo));

    float psi = wc_yarn_type_get_psi(params,data.yarn_type,
		intersection_data.context);

    float u = data.u;
    float x = data.x;
    float D;
    {
        float a = H.y*sinf(u) + H.z*cosf(u);
        D = (H.y*cosf(u)-H.z*sinf(u))/(sqrtf(H.x*H.x + a*a))/
			tanf(psi);
    }
    float reflection = 0.f;
            
    //Plus eller minus i sista termen?
    float specular_v = atan2f(-H.y*sinf(u) - H.z*cosf(u), H.x) + acosf(D);
    //TODO(Vidar): Clamp specular_v, do we need it?
    // Make normal for highlights, uses u and specular_v
    wcVector highlight_normal = wcVector_normalize(wcvector(sinf(specular_v),
        sinf(u)*cosf(specular_v), cosf(u)*cosf(specular_v)));

    if (fabsf(specular_v) < M_PI_2 && fabsf(D) < 1.f) {
        //we have specular reflection
        //get specular_x, using irawans transformation.
        float specular_x = specular_v/M_PI_2;
        // our transformation
        //float specular_x = sinf(specular_v);

        float delta_x = wc_yarn_type_get_delta_x(params,data.yarn_type,
			intersection_data.context);
        float umax;
        if (data.ext_between_parallel){
            //if segment is extension between to parallel yarns -> bend = 0
            umax = 0.001;
        } else {
            umax = wc_yarn_type_get_umax(params,data.yarn_type,
                    intersection_data.context);
        }

        //Clamp specular_x
        specular_x = specular_x < 1.f - delta_x ? specular_x :
            1.f - delta_x;
        specular_x = specular_x > -1.f + delta_x ? specular_x :
            -1.f + delta_x;

        if (fabsf(specular_x - x) < delta_x) {

            float alpha = wc_yarn_type_get_alpha(params,data.yarn_type,
				intersection_data.context);
            float beta  = wc_yarn_type_get_beta( params,data.yarn_type,
				intersection_data.context);

            // --- Set Gv
            float a = 1.f; //radius of yarn
            float R = 1.f/(sin(umax)); //radius of curvature
            float Gv = a*(R + a*cosf(specular_v))/(
                wcVector_magnitude(wcVector_add(wi,wo)) *
                wcVector_dot(highlight_normal,H) * fabsf(sinf(psi)));
            // --- Set fc
            float cos_x = -wcVector_dot(wi, wo);
            float fc = alpha + vonMises(cos_x, beta);
            // --- Set A
            float widotn = wcVector_dot(wi, highlight_normal);
            float wodotn = wcVector_dot(wo, highlight_normal);
            widotn = (widotn < 0.f) ? 0.f : widotn;   
            wodotn = (wodotn < 0.f) ? 0.f : wodotn;   
            //TODO(Vidar): This is where we get the NAN
            float A = 0.f;
            if(widotn > 0.f && wodotn > 0.f){
                A = 1.f / (4.0 * M_PI) * (widotn*wodotn)/(widotn + wodotn);
                //TODO(Peter): Explain from where the 1/4*PI factor comes from
            }
            float w = 2.f;
            reflection = 2.f*w*umax*fc*Gv*A/delta_x;
        }
    }
    return reflection;
}

wcColor wcEvalDiffuse(wcIntersectionData intersection_data,
        wcPatternData data, const wcWeaveParameters *params)
{
    float value = intersection_data.wi_z;

	wcYarnType *yarn_type = params->yarn_types + data.yarn_type;
    if(!yarn_type->color_enabled || !data.yarn_hit){
        yarn_type = &params->yarn_types[0];
    }

    wcColor color = {
        yarn_type->color.r * value,
        yarn_type->color.g * value,
        yarn_type->color.b * value
    };
	if(yarn_type->color_texmap){
		color = wc_eval_texmap_color(yarn_type->color_texmap,
            intersection_data.context);
		color.r*=value; color.g*=value; color.b*=value;
	}

    return color;
}

float wcEvalSpecular(wcIntersectionData intersection_data,
        wcPatternData data, const wcWeaveParameters *params)
{
    // Depending on the given psi parameter the yarn is considered
    // staple or filament. They are treated differently in order
    // to work better numerically. 
    float reflection = 0.f;
    if(params->pattern == 0){
        return 0.f;
    }
	if(!data.yarn_hit){
        //have not hit a yarn...
        return 0.f;
	}
    float psi = wc_yarn_type_get_psi(params, data.yarn_type,
		intersection_data.context);
    if (psi <= 0.001f) {
        //Filament yarn
        reflection = wcEvalFilamentSpecular(intersection_data, data, params); 
    } else {
        //Staple yarn
        reflection = wcEvalStapleSpecular(intersection_data, data, params); 
    }
	float specular_noise=wc_yarn_type_get_specular_noise(params,
		data.yarn_type,intersection_data.context);
	float noise=1.f;
    if(specular_noise > 0.001f){
		float iv = intensityVariation(data);
		noise=(1.f-specular_noise)+specular_noise * iv;
    }
	return reflection * params->specular_normalization * noise;
}

wcColor wcShade(wcIntersectionData intersection_data,
        const wcWeaveParameters *params)
{
    wcPatternData data = wcGetPatternData(intersection_data,params);
    wcColor ret = wcEvalDiffuse(intersection_data,data,params);
    float spec  = wcEvalSpecular(intersection_data,data,params);
    float specular_strength = wc_yarn_type_get_specular_strength(params,
        data.yarn_type,
		intersection_data.context);
    ret.r = ret.r*(1.f-specular_strength) + specular_strength*spec;
    ret.g = ret.g*(1.f-specular_strength) + specular_strength*spec;
    ret.b = ret.b*(1.f-specular_strength) + specular_strength*spec;
    return ret;
    }
