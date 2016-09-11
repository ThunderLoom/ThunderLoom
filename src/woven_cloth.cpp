#include "woven_cloth.h"
#ifndef WC_NO_FILES
#define REALWORLD_UV_WIF_TO_MM 10.0
#include "wif/wif.cpp"
#include "wif/ini.cpp"
#include "str2d.h"
#endif

#include "perlin.h"
#include "halton.h"

// For M_PI etc.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

// -- 3D Vector data structure -- //
typedef struct
{
    float x,y,z,w;
} wcVector;

WC_PREFIX
static wcVector wcvector(float x, float y, float z)
{
    wcVector ret = {x,y,z,0.f};
    return ret;
}

WC_PREFIX
static float wcVector_magnitude(wcVector v)
{
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

WC_PREFIX
static wcVector wcVector_normalize(wcVector v)
{
    wcVector ret;
    float inv_mag = 1.f/sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    ret.x = v.x * inv_mag;
    ret.y = v.y * inv_mag;
    ret.z = v.z * inv_mag;
    return ret;
}

WC_PREFIX
static float wcVector_dot(wcVector a, wcVector b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

WC_PREFIX
static wcVector wcVector_cross(wcVector a, wcVector b)
{
    wcVector ret;
    ret.x = a.y*b.z - a.z*b.y;
    ret.y = a.z*b.x - a.x*b.z;
    ret.z = a.x*b.y - a.y*b.x;
    return ret;
}

WC_PREFIX
static wcVector wcVector_add(wcVector a, wcVector b)
{
    wcVector ret;
    ret.x = a.x + b.x;
    ret.y = a.y + b.y;
    ret.z = a.z + b.z;
    return ret;
}

// -- //

WC_PREFIX
static float wcClamp(float x, float min, float max)
{
    return (x < min) ? min : (x > max) ? max : x;
}

/* Tiny Encryption Algorithm by David Wheeler and Roger Needham */
/* Taken from mitsuba source code. */
WC_PREFIX
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
WC_PREFIX
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

WC_PREFIX
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

WC_PREFIX
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

WC_PREFIX
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
    umax = yarn_type_get_umax(params->pattern, pattern_data->yarn_type, 
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



WC_PREFIX
void wcFinalizeWeaveParameters(wcWeaveParameters *params)
{
	if (params->pattern) {
    //Generate distance matrix for the yarns in the pattern.
    /*{
        //each element in this matrix contains the distance in pattern indicies
        //to the beginning of each yarnsegment in the pattern.
        //The distance are floats that allow for yarn types with different sizes.
        //
        //These distances take varying yarn sizes into account!
        //This way only one lookup is required duiring rendering. 

        //Loop through each entry in pattern matrix.
        //Look up yarn type and its corresponding size.
        //Use that size value as a factor to change the yarn size 
        
        //get pattern info
        uint32_t w = pattern->pattern_width;
        uint32_t h = pattern->pattern_height;
        float* segment_distances = (float*)calloc((w)*(h),sizeof(float));
        for(y=0;y<h;y++){
            for(x=0;x<w;x++){
                PatternEntry current_point = params->pattern->
                    entries[x + y*params->pattern_width];
                
                //for each element in pattern matrix
                //Calculate the size of the segment
                uint32_t steps_left_warp = 0, steps_right_warp = 0;
                uint32_t steps_left_weft = 0, steps_right_weft = 0;
                if (current_point.warp_above) {
                    calculateLengthOfSegment(current_point.warp_above, x,
                            y, &steps_left_warp, &steps_right_warp,
                            params->pattern_width, params->pattern_height,
                            params->pattern->entries);
                } else{
                    calculateLengthOfSegment(current_point.warp_above, x,
                            y, &steps_left_weft, &steps_right_weft,
                            params->pattern_width, params->pattern_height,
                            params->pattern->entries);
                }

                //Look at crossing neighbors, 
                //Use their yarnsize to increase distances a bit.
                if (current_point.warp_above) {
                    //look at pm y
                } else{
                    //look at pm x
                }

                //segment_distances[x+y*w].warp_above = 
                


                //Look at crossing neighbors, Use their yarnsize to increase distances a bit.

            }
        }
        
        //Would like a way to specify value between 0 to 1 (1 is full wifth of patern)
        //and in return get the yarn and position in yarn.
        //Non matrix base representation of yarns?
        //Have float base representation of yarn segments and a function that finds one based on x,y input?
        
    }
    */

    //Calculate normalization factor for the specular reflection
		size_t nLocationSamples = 100;
		size_t nDirectionSamples = 1000;
		params->specular_normalization = 1.f;

		float highest_result = 0.f;

		// Temporarily disable speuclar noise...
		float tmp_specular_noise[WC_MAX_YARN_TYPES];
		for(int i=0;i<params->pattern->num_yarn_types;i++){
			tmp_specular_noise[i] = params->pattern->yarn_types[i].specular_noise;
			params->pattern->yarn_types[i].specular_noise = 0.f;
		}

		// Normalize by the largest reflection across all uv coords and
		// incident directions
		for( uint32_t yarn_type = 0; yarn_type < params->pattern->num_yarn_types;
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
				pattern_data.yarn_hit = 1; //TODO: Verify that this is okay (always hit)
				pattern_data.yarn_type = yarn_type;
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
		for(int i=0;i<params->pattern->num_yarn_types;i++){
			params->pattern->yarn_types[i].specular_noise = tmp_specular_noise[i];
		}
	}
}

                                                                                                             
WC_PREFIX
static Pattern *build_pattern_from_data(uint8_t *warp_above,
	uint32_t *yarn_type, YarnType *yarn_types, uint32_t num_yarn_types,
	uint32_t w, uint32_t h)
{
    uint32_t x,y,c;
    Pattern *pattern;
    pattern = (Pattern*)calloc(1,sizeof(Pattern));
    pattern->yarn_types = (YarnType*)calloc(num_yarn_types,sizeof(YarnType));
	pattern->num_yarn_types = num_yarn_types;
	for (c = 0; c < num_yarn_types; c++) {
		pattern->yarn_types[c] = yarn_types[c];
	}
    pattern->entries = (PatternEntry*)calloc((w)*(h),sizeof(PatternEntry));
    for(y=0;y<h;y++){
        for(x=0;x<w;x++){
            pattern->entries[x+y*w].warp_above = warp_above[x+y*w];
            pattern->entries[x+y*w].yarn_type  =  yarn_type[x+y*w];
        }
    }
    return pattern;
}

#ifndef WC_NO_FILES

WC_PREFIX
static char * find_next_newline(char * buffer)
{
    char *r = buffer;
    while(*r != 0 && *r != '\n'){
        r++;
    }
    return r;
}

WC_PREFIX
static char * read_color_from_weave_string(char *string, float * color)
{
    char *s = string;
    for(int i=0;i<3;i++){
        int a = atoi(s);
        s = find_next_newline(s+1);
        color[i] = (float)a/255.f;
    }
    return find_next_newline(find_next_newline(s)+1)+1;
}

WC_PREFIX
static char * read_dimensions_from_weave_string(char *string,
        float * thicknessw, float * thicknessh)
{
    char *s = string;
    
    *thicknessw = (float)str2d(s);
    s = find_next_newline(s+1);
    *thicknessh = (float)str2d(s);
    s = find_next_newline(s+1);
    
    return find_next_newline(find_next_newline(s)+1)+1;
}


WC_PREFIX
static void read_pattern_from_weave_string(char * s, uint32_t *pattern_width,
    uint32_t *pattern_height, float *pattern_realwidth,
    float *pattern_realheight, Pattern **pattern)
{
    //A Weave file has 2-three sets of color
    //2-two sets of thickness and spacing in cm
    //a pattern matrix

    float warp_color[3], weft_color[3], 
          warp_thickness, weft_thickness;
    s = read_color_from_weave_string(s,warp_color);
    s = read_color_from_weave_string(s,weft_color);
    s = read_dimensions_from_weave_string(s, &warp_thickness, &weft_thickness);

    char * t = find_next_newline(s);
    *pattern_width = t - s;
    int i = 0;
    int num_chars = 0;
    while(s[i] != 0) {
        if(s[i] == '0' || s[i] == '1'){
            num_chars++;
        }
        i++;
    }
    *pattern_height = num_chars/(*pattern_width);

    *pattern = (Pattern*)calloc(1,sizeof(Pattern));
    (*pattern)->entries = (PatternEntry*)calloc(num_chars,sizeof(PatternEntry));
    (*pattern)->yarn_types = (YarnType*)calloc(2,sizeof(YarnType));
    (*pattern)->num_yarn_types = 2;
    memcpy((*pattern)->yarn_types[0].color, warp_color,3*sizeof(float));
    memcpy((*pattern)->yarn_types[1].color, weft_color,3*sizeof(float));
    
    *pattern_realwidth = REALWORLD_UV_WIF_TO_MM
        * (*pattern_width * (warp_thickness)); 
    *pattern_realheight = REALWORLD_UV_WIF_TO_MM
        * (*pattern_height * (weft_thickness)); 
   
    i = 0;
    int ii =0;
    while(s[i] != 0) {
        if(s[i] == '0' || s[i] == '1'){
            if(s[i] == '1'){
                (*pattern)->entries[ii].warp_above = 1;
                (*pattern)->entries[ii].yarn_type  = 0;
            }else{
                (*pattern)->entries[ii].yarn_type  = 1;
            }
            ii++;
        }
        i++;
    }
}

WC_PREFIX
void wcWeavePatternFromFile(wcWeaveParameters *params, const char *filename)
{
    if(strlen(filename) > 4){
        if(strcmp(filename+strlen(filename)-4,".wif") == 0){
            wcWeavePatternFromWIF(params,filename);
        } else {
            wcWeavePatternFromWeaveFile(params,filename);
        }
    } else{
        params->pattern_height = params->pattern_width = 0;
        params->pattern = 0;
    }
}

WC_PREFIX
void wcWeavePatternFromFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename)
{
    if(wcslen(filename) > 4){
        if(wcscmp(filename+wcslen(filename)-4,L".wif") == 0){
            wcWeavePatternFromWIF_wchar(params,filename);
        } else {
            wcWeavePatternFromWeaveFile_wchar(params,filename);
        }
    }else{
        params->pattern_height = params->pattern_width = 0;
        params->pattern = 0;
    }
}

WC_PREFIX
void wcWeavePatternFromWIF(wcWeaveParameters *params, const char *filename)
{
    WeaveData *data = wif_read(filename);
    params->pattern = wif_get_pattern(data,
        &params->pattern_width, &params->pattern_height,
        &params->pattern_realwidth, &params->pattern_realheight);
    wif_free_weavedata(data);
    wcFinalizeWeaveParameters(params);
}

WC_PREFIX
void wcWeavePatternFromWIF_wchar(wcWeaveParameters *params,
        const wchar_t *filename)
{
    WeaveData *data = wif_read_wchar(filename);
    params->pattern = wif_get_pattern(data,
        &params->pattern_width, &params->pattern_height,
        &params->pattern_realwidth, &params->pattern_realheight);
    wif_free_weavedata(data);
    wcFinalizeWeaveParameters(params);
}

WC_PREFIX
static void weave_pattern_from_weave_file(wcWeaveParameters *params,
    FILE *f)
{
    fseek (f , 0 , SEEK_END);
    long lSize = ftell (f);
    rewind (f);
    char *buffer = (char*) calloc (lSize+1,sizeof(char));
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
    fread (buffer,1,lSize,f);
    read_pattern_from_weave_string(buffer, 
            &params->pattern_width, &params->pattern_height,
            &params->pattern_realwidth, &params->pattern_realheight,
            &params->pattern);
    wcFinalizeWeaveParameters(params);
}

WC_PREFIX
void wcWeavePatternFromWeaveFile(wcWeaveParameters *params,
    const char *filename)
{
    FILE *f = fopen(filename,"rt");
    if(f){
        weave_pattern_from_weave_file(params,f);
        fclose(f);
    }else{
        params->pattern_width = params->pattern_height = 0;
    }
}

WC_PREFIX
void wcWeavePatternFromWeaveFile_wchar(wcWeaveParameters *params,
    const wchar_t *filename)
{
#ifdef _WIN32
    FILE *f = _wfopen(filename, L"rt");
    if(f){
        weave_pattern_from_weave_file(params,f);
        fclose(f);
    }else{
        params->pattern_width = params->pattern_height = 0;
		params->pattern = 0;
    }
#endif
}
#endif

WC_PREFIX
void wcWeavePatternFromData(wcWeaveParameters *params, uint8_t *warp_above,
	uint32_t *yarn_type, YarnType *yarn_types, uint32_t num_yarn_types,
	uint32_t pattern_width, uint32_t pattern_height)
{
	params->pattern_width  = pattern_width;
	params->pattern_height = pattern_height;
	params->pattern = build_pattern_from_data(warp_above,
		yarn_type, yarn_types, num_yarn_types, pattern_width, pattern_height);
	wcFinalizeWeaveParameters(params);
}

WC_PREFIX
void wcFreeWeavePattern(wcWeaveParameters *params)
{
    if(params->pattern){
		if (params->pattern->yarn_types) {
			free(params->pattern->yarn_types);
		}
		if (params->pattern->entries) {
			free(params->pattern->entries);
		}
        free(params->pattern);
    }
}

WC_PREFIX
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

WC_PREFIX
static float yarnVariation(wcPatternData pattern_data, 
        const wcWeaveParameters *params)
{

    float variation = 1.f;

    uint32_t tindex_x = pattern_data.total_index_x;
    uint32_t tindex_y = pattern_data.total_index_y;

    //Switch X and Y for warp, so that we have the yarn going along y
    if(!pattern_data.warp_above){
        float tmp = tindex_x;
        tindex_x = tindex_y;
        tindex_y = tmp;
    }

    //We want to vary the intesity along the yarn.
    //For a parallel yarn we want a diffrent variation. 
    //Use a large xscale to make the values different.

    float amplitude = params->yarnvar_amplitude;
    float xscale = params->yarnvar_xscale;
    float yscale = params->yarnvar_yscale;
    int octaves = params->yarnvar_octaves;
    float persistance = params->yarnvar_persistance; //paramter

    float x_noise = (tindex_x/(float)params->pattern_width) * xscale;
    //NOTE(Vidar): Should it really be pattern_width here too? Ask Peter...
    float y_noise = (tindex_y + (pattern_data.y/2.f + 0.5))
        /(float)params->pattern_width * yscale;

    variation = octavePerlin(x_noise, y_noise, 0,
            octaves, persistance) * amplitude + 1.f;

    return wcClamp(variation, 0.f, 1.f);

    //NOTE(Peter): Right now there is perlin on both warp and weft.
    //Would be useful to specify only warp or weft. Or even better, 
    //if a yarn_type definition is made, 
    // define noise parmeters for a certain yarn. along with other 
    // yarn properties.
}

WC_PREFIX
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

WC_PREFIX
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

WC_PREFIX
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
    *entry = params->pattern->entries[tmpx + tmpy*params->pattern_width];
}

WC_PREFIX
int32_t wcRepeatIndex(const int32_t coord, const int32_t size) {
    int32_t tmpcoord = (int32_t)fmod(coord, (float)size);
    if (tmpcoord < 0.f) {
        tmpcoord = size + tmpcoord;
    }
    return tmpcoord;
}


WC_PREFIX
wcYarnSegment wcCalculateSegmentDim(float total_u, float total_v,
        wcWeaveParameters *params) {
    float u = fmod(total_u,1.f);
    float v = fmod(total_v,1.f);
    if (u < 0.f) {
        u = u - floor(u);
    }
    if (v < 0.f) {
        v = v - floor(v);
    }

    PatternEntry *pattern_entries = params->pattern->entries;
    uint32_t pattern_width = params->pattern_width;
    uint32_t pattern_height = params->pattern_height;
    //TODO fmod repeat uvs, to avoiding the u or v == 1 check.
    int32_t pattern_x = (u == 1.f) ? pattern_width-1 : (uint32_t)(u*(pattern_width));
    int32_t pattern_y = (v == 1.f) ? pattern_height-1 : (uint32_t)(v*(pattern_height));
    //The origin is the pattern entry from which size of segment is calculated. 
    //The origin entry changes if we miss a thin yarn.
    int32_t origin_x = pattern_x;
    int32_t origin_y = pattern_y;
    PatternEntry origin_entry = params->pattern->entries[pattern_x + 
        pattern_y*pattern_width];
    uint8_t warp_above = origin_entry.warp_above;
    
    int32_t current_x = origin_x;
    int32_t current_y = origin_y;
    int32_t *incremented_coord_along = warp_above ? &current_y : &current_x;
    int32_t *incremented_coord_across = warp_above ? &current_x : &current_y;
    float cell_x = (u == 1.f) ? 1 : u*(float)(pattern_width) - pattern_x;
    float cell_y = (v == 1.f) ? 1 : v*(float)(pattern_height) - pattern_y;
    float *cell_coord_along = warp_above ? &cell_y : &cell_x;
    float *cell_coord_across = warp_above ? &cell_x : &cell_y;
    uint32_t max_size_along = warp_above ? pattern_height: pattern_width;
    uint32_t max_size_across = warp_above ? pattern_width: pattern_height;
    uint32_t initial_coord_along = warp_above ? pattern_y: pattern_x;
    uint32_t initial_coord_across = warp_above ? pattern_x: pattern_y;
    printf("-----\n");
    printf("pattern_x: %d, pattern_y: %d\n", pattern_x, pattern_y);
    printf("u: %f, v: %f\n", u, v);
    printf("cell_x: %f, cell_y: %f\n", cell_x, cell_y);
    printf("pattern_w: %d, pattern_h: %d\n", pattern_width, pattern_height);
    printf("start warp_above: %d\n", warp_above);
    printf("start yarnsize: %f\n", params->pattern->yarn_types[origin_entry.yarn_type].yarnsize);
    printf("*cell_coord_across: %f\n", (*cell_coord_across));
    printf("2*cell_coord_across-1.f: %f\n", fabsf(2.f*(*cell_coord_across)-1.f));

    //Have we hit the yarn? (For later, are we between two directly parallel yarns?)
    uint8_t yarn_hit = 0;
    uint8_t between_parallel = 0;
    uint8_t extension = 0;
    uint32_t origin_offset = 0;
    if (fabsf(2*(*cell_coord_across)-1.f) <= params->pattern->yarn_types[origin_entry.yarn_type].yarnsize) {
        yarn_hit = 1;
    } else {
        printf("Miss... look for extension\n");
        //Did not hit yarn, look for extension...
        int8_t direction = (*cell_coord_across >= 0.5) ? 1 : -1;
        PatternEntry tmp_pe; 
        *incremented_coord_across = initial_coord_across;
        printf("(before loop)(*incremented_coord_across): %d\n", *incremented_coord_across);
        uint8_t found_extension_entry = 1;
        do{
            (*incremented_coord_across) += direction;
            lookupPatternEntry(&tmp_pe, params, current_x, current_y);
            printf("(loop )(*incremented_coord_across): %d\n", *incremented_coord_across);
            printf("(loop) current_x: %d, current_y: %d\n", current_x, current_y);
            printf("stuck in loop? max_size_across: %d, inc-initial: %d\n", max_size_across, abs((int)*incremented_coord_across) - initial_coord_across);
            if ((*incremented_coord_across) == initial_coord_across +
                    direction && tmp_pe.warp_above == warp_above) {
                between_parallel = 1;
            }
            if (abs((int)*incremented_coord_across - (int)initial_coord_across) == max_size_across) {
                found_extension_entry = 0;
                break;
            }
        } while (tmp_pe.warp_above == warp_above);
        printf("found_extension_entry: %d \n", found_extension_entry);
        
        //Is there are yarn that can be used as extension. Did we hit it?
        if (found_extension_entry && fabsf(2*(*cell_coord_along)-1.f) <=
                params->pattern->yarn_types[tmp_pe.yarn_type].yarnsize) {
            //Yes we hit an extention. Use this pattern entry as new origin!
            printf("HIT extension!\n");
            origin_entry = tmp_pe;
            origin_offset = abs((!warp_above) ? (current_y - (int)origin_y) : (current_x - (int)origin_x));
            printf("current_y - origin_y: %d - %d = %d\n",current_y, origin_y, current_y-(int)origin_y);
            printf("current_x - origin_x: %d - %d = %d\n",current_x, origin_x, current_x-(int)origin_x);
            printf("warp_above: %d\n",warp_above);
            warp_above = origin_entry.warp_above;
            origin_x = wcRepeatIndex(current_x, pattern_width);
            origin_y = wcRepeatIndex(current_y, pattern_height);

            extension = 1;
            yarn_hit = 1;
        }
    }
    printf("origin offset: %d\n",origin_offset);
    
    //look right and left from origin until we hit cell that is not current yarn weft/warp.
    PatternEntry border_yarn_left;
    PatternEntry border_yarn_right;
    uint32_t steps_right = 0;
    uint32_t steps_left  = 0;
    uint8_t parallel_right = 0;
    uint8_t parallel_left  = 0;
    current_x = origin_x;
    current_y = origin_y;
    do{ 
        (*incremented_coord_along)++;
        if(*incremented_coord_along == max_size_along){
            *incremented_coord_along = 0;
        }
        PatternEntry tmp_pe = pattern_entries[current_x + 
            current_y*pattern_width];
        if((tmp_pe.warp_above) != warp_above){
            border_yarn_right = tmp_pe;
            break;
        }
        (steps_right)++;
    } while(*incremented_coord_along != initial_coord_along);

    *incremented_coord_along = initial_coord_along;
    do{
        if(*incremented_coord_along == 0){
            *incremented_coord_along = max_size_along;
        }
        (*incremented_coord_along)--;
        PatternEntry tmp_pe = pattern_entries[current_x + 
            current_y*pattern_width];
        if((tmp_pe.warp_above) != warp_above){
            border_yarn_left = tmp_pe;
            break;
        }
        (steps_left)++;
    } while(*incremented_coord_along != initial_coord_along);
    
    float border_yarn_size_left = params->pattern->
        yarn_types[border_yarn_left.yarn_type].yarnsize;
    float border_yarn_size_right = params->pattern->
        yarn_types[border_yarn_right.yarn_type].yarnsize;

    float width = params->pattern->yarn_types[origin_entry.yarn_type].yarnsize;
    float length = steps_left + steps_right + 1.f +
        ((1.f-border_yarn_size_left) + (1.f-border_yarn_size_right))/2.f;
    printf("steps_left: %d, steps_right: %d\n", steps_left, steps_right);
    printf("tmp width: %f, length: %f \n", width, length);
    printf("between parallel: %d\n", between_parallel);

    //If current segment is between two parallel yarns. Do not count self.
    if(between_parallel) length -= 1;

    //Determine coordinates for top left corner of segment.
    float start_u, start_v;
    {
        float distance_left = steps_left + (1.f - border_yarn_size_left)/2.f
            + origin_offset;
        float distance_top = - (1.f-width)/2.f;
        printf("distance_left: %f, distance_top: %f \n", distance_left, distance_top);
        printf("cell_coord_along: %f, cell_coord_across: %f \n", *cell_coord_along, *cell_coord_across);
        if (!warp_above) {
            start_u = total_u + (-cell_x - distance_left)/pattern_width;
            start_v = total_v + (-cell_y - distance_top)/pattern_height;
        } else {
            start_u = total_u + (-cell_x - distance_top)/pattern_width;
            start_v = total_v + (-cell_y - distance_left)/pattern_height;
            printf("total_v - distance_top/pattern_height: %f - %f/%d = %f \n", total_v, distance_left, pattern_height, start_v);
        }
    }

    wcYarnSegment yarn;
    yarn.length = length;
    yarn.width = width;
    yarn.start_u = start_u;
    yarn.start_v = start_v;
    yarn.warp_above = origin_entry.warp_above;
    yarn.yarn_hit = yarn_hit;
    yarn.between_parallel = between_parallel;
    printf("yarn_hit: %d \n", yarn_hit);
    return yarn;
}

/*
 * Given intersection_data, wcGetPatternData determines the current yarn
 * segment and associated paramters needed for shading.
 * Determination of yarn segment is made more complicated by the fact that 
 * yarnsizes can vary. This results in a certain number of special cases.
 */

/* Outline of function...
 * 1) converts intersection_data to scaled repeating uv coordinates
 * 2) stores cell indicies for a theoretical inf. pattern matrix. (used later)
 * 3) sets pattern_x, pattern_y, for looking up in pattern_matrix
 * 4) Sets current_point and original_point (incl original_pattern_x, y)
 * 5) Declare flags for yarn_hit, orignal_parallel_warp/weft
 * 6) Check that we have hit within current yarn.
 *      if yes -> mark yarn_hit = 1 and proceed as normal
 *      if no  -> check if adjascent background wef/warp is hit (in this process populate original_pattern_x/y if needed)
 *          if yes -> yarn_hit = 1, change pattern_x/y to adjascent
 *          if no  -> yarn hit = 0
 * 7) if no yarn hit. Stop and return minimal data.
 * 8) Calculate size of yarnsegment (in number of cells.)
 * 9) If needed. add to this size the extentions that have been made.
 * 10) Calculate offsets based on theese extentions.
 *      a) special case if u/v on first half of first cell and pattern_x/y is last.
 *          -> then offset_x/y = pattern_width/height
 *      b) special case if u/v on last half of last cell and pattern_x/y is first.
 *          -> then offset_x/y = -pattern_width/height
 *      c) look left/right and add adjascent yarns (1-yarnsize)/2
 *      d) Negative offset if this yarn is to be shorter.
 * 11) Add 1 to width/length if we are on a warp/weft not if we are between to parallel yarns.
 *      a) special case if parallel_warp/weft 
 *          i) set pattern_x/y to original
 *          ii) set offset_x/y to -1 if on second half of cell
 *          iii) reset steps_left-tight_weft/warp to zero (might not be needed)
 * 12) Calculate length, width, x, y coordinates of yarn_segment.
 * 13) rescale to fit yarn_segment_local coordinate system.
 * 14) swap coordinates if weft. So x/w is acroos and y/l is along.
 * */

/*
WC_PREFIX
wcPatternData wcGetPatternDataNEW(wcIntersectionData intersection_data,
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

    float total_u = uv_x*u_scale;
    float total_v = uv_y*u_scale;

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
    float width;
    float length;
    uint8_t yarn_hit = wcCalculateSegmentDim(u_repeat, v_repeat, params,
            &width, &length);
    
    if (!yarn_hit) {
        //No hit, No need to do more calculations.
        wcPatternData ret_data;
        ret_data.yarn_hit = 0;
        ret_data.yarn_type = 0;
        return ret_data;
    }

    float l = length;
    float y = 0;//?

	float w = width;
    float x = 0; //?

    //Rescale x, y to [-1,1], w,v scaled by 2
    x = x*2.f - 1.f;
    y = y*2.f - 1.f;
    w = w*2.f;
    l = l*2.f;

    //Swap X and Y for warp, so that we always have the yarn going along y.
    if(!extention_entry.warp_above){
        float tmp1 = x;
        float tmp2 = w;
        x = -y;
        y = tmp1;
        w = l;
        l = tmp2;
    }
    
    //return the results
    wcPatternData ret_data;
	ret_data.yarn_hit = yarn_hit;
	//TODO ret_data.ext_between_parallel = (parallel_warp || parallel_weft) ? 1 : 0;
	//TODO ret_data.yarn_type = extention_entry.yarn_type;
    ret_data.length = l; 
    ret_data.width  = w; 
    //TODO ret_data.x = x; 
    //TODO ret_data.y = y; 
    //TODO ret_data.warp_above = extention_entry.warp_above; 
    calculate_segment_uv_and_normal(&ret_data, params, &intersection_data);
    ret_data.total_index_x = total_pattern_x; //total x index of wrapped pattern matrix
    ret_data.total_index_y = total_pattern_y; //total y index of wrapped pattern matrix
    return ret_data;
}*/


WC_PREFIX
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
    float u_repeat = fmod(uv_x*u_scale,1.f);
    float v_repeat = fmod(uv_y*v_scale,1.f);
    if (u_repeat < 0.f) {
        u_repeat = u_repeat - floor(u_repeat);
    }
    if (v_repeat < 0.f) {
        v_repeat = v_repeat - floor(v_repeat);
    }
    //non-repeating pattern index
    uint32_t total_pattern_x = uv_x*u_scale*params->pattern_width;
    uint32_t total_pattern_y = uv_y*v_scale*params->pattern_height;
    //pattern index
    uint32_t pattern_x = (uint32_t)(u_repeat*(float)(params->pattern_width));
    uint32_t pattern_y = (uint32_t)(v_repeat*(float)(params->pattern_height));
    //Coordinate within the current pattern cell
    float cell_x = ((u_repeat*(float)(params->pattern_width) - (float)pattern_x));
    float cell_y = ((v_repeat*(float)(params->pattern_height) - (float)pattern_y));
    
    PatternEntry current_point = params->pattern->entries[pattern_x +
        pattern_y*params->pattern_width];        

    //Verify that we are on the Yarn! And if not find adjascent extention.
    uint8_t yarn_hit = 0;
    uint8_t parallel_warp = 0;
    uint8_t parallel_weft = 0;
    PatternEntry extention_entry = current_point;
    uint8_t extension_pattern_x = pattern_x;
    uint8_t extension_pattern_y = pattern_y;
    {
        float yarnsize = params->pattern->
            yarn_types[current_point.yarn_type].yarnsize;
        float x = cell_x*2.f - 1.f; float y = cell_y*2.f - 1.f;
        if (current_point.warp_above){
            if (fabsf(x) <= yarnsize) {
                yarn_hit = 1;
            } else {
                //Yarn is thin. We have hit outside it!
                //Need to check if we hit an extention from an adjascent weft.
                //For later, need to know if adjsacent yarn is warp (2 parallel)
                int8_t dir = (x > 0) ? 1 : -1;
                PatternEntry lookup_entry = current_point;
                uint8_t weft_flag = 1;
                uint8_t i = 0;
                while (lookup_entry.warp_above) {
                    i++;
                    lookupPatternEntry(&lookup_entry, params,
                            (pattern_x + i*dir), pattern_y);
                    //Set parallel flag if thread directly adjescent also is a warp
                    if (i == 1 && lookup_entry.warp_above) {
                        parallel_warp = 1;
                    }

                    if (i > params->pattern_width) {
                        //no adjascent wefts! Stop to prevent infinite loop!
                        weft_flag = 0;
                        break;
                    }
                }
                //Check if adjascent weft is large enough aswell!
                if (weft_flag && fabs(y) <= params->pattern->
                        yarn_types[lookup_entry.yarn_type].yarnsize) {
                    extention_entry = lookup_entry;
                    extension_pattern_x = wcRepeatIndex((pattern_x + i*dir),
                            params->pattern_width);
                    yarn_hit = 1;
                }
            }
        } else { //weft above
            if (fabsf(y) <= yarnsize) {
                yarn_hit = 1;
            } else {
                //Yarn is thin. We have hit outside it!
                //Need to check if we hit an extention from an adjascent weft.
                //For later, need to know if adjsacent yarn is warp (2 parallel)
                int8_t dir = (y > 0) ? 1 : -1;
                PatternEntry lookup_entry = current_point;
                uint8_t warp_flag = 1;
                uint8_t i = 0;
                while (!lookup_entry.warp_above) {
                    i++;
                    lookupPatternEntry(&lookup_entry, params, pattern_x,
                            (pattern_y + i*dir));
                    //Set parallel flag if thread directly adjescent also is a weft
                    if (i == 1 && !lookup_entry.warp_above) {
                        parallel_weft = 1;
                    }

                    if (i > params->pattern_width) {
                        //no adjascent wefts! Stop to prevent infinite loop!
                        warp_flag = 0;
                        break;
                    }
                }
                //Check if adjascent warp is large enough aswell!
                if (warp_flag && fabs(x) <= params->pattern->
                        yarn_types[lookup_entry.yarn_type].yarnsize) {
                    extention_entry = lookup_entry;
                    extension_pattern_y = wcRepeatIndex((pattern_y + i*dir),
                            params->pattern_height);
                    yarn_hit = 1;
                }
            }
        
        }
    }

    if (!yarn_hit) {
        //No hit, No need to do more calculations.
        //return the results
        wcPatternData ret_data;
        ret_data.yarn_hit = 0;
        ret_data.yarn_type = 0;
        return ret_data;
    }

    //Calculate the size of the segment at the cell level.
    uint32_t steps_left_warp = 0, steps_right_warp = 0;
    uint32_t steps_left_weft = 0, steps_right_weft = 0;
    if (extention_entry.warp_above) {
        calculateLengthOfSegment(extention_entry.warp_above, extension_pattern_x,
            extension_pattern_y, &steps_left_warp, &steps_right_warp,
            params->pattern_width, params->pattern_height,
            params->pattern->entries);
    } else{
        calculateLengthOfSegment(extention_entry.warp_above, extension_pattern_x,
            extension_pattern_y, &steps_left_weft, &steps_right_weft,
            params->pattern_width, params->pattern_height,
            params->pattern->entries);
    }

    //Se if yarnsegment needs to be extended or shrunk...
    //TODO(Peter): Maybe move this into a new calculateLengthOfSegment!
    float extension_x_left = 0;
    float extension_x_right = 0;
    float offset_x = 0;
    float extension_y_left = 0;
    float extension_y_right = 0;
    float offset_y = 0;
    if (extention_entry.warp_above) {
        if ((v_repeat)*(float)(params->pattern_height) <= 0.5 && extension_pattern_y == (params->pattern_height-1)) {
            //Special case! If we are on first half of first cell and
            //extension_pattern_x for extention is last cell in pattern. 
            //Then offset the y coordinate with pattern height so that we 
            //continue from extention start. Get correct coordinate behaviour.
            offset_y = (float)(params->pattern_height);
        }
        if ((1.f - v_repeat)*(float)(params->pattern_height) <= 0.5 && extension_pattern_y == 0) {
            //Same when we are on last half of last cell and 
            //extension_pattern_y is on first cell. 
            offset_y = -1.f*(float)(params->pattern_height);
        }

        //Look left at end of segment to see if current needs extending.
        PatternEntry lookup_entry;
        lookupPatternEntry(&lookup_entry, params, extension_pattern_x,
                (extension_pattern_y - steps_left_warp - 1));        
        float tmp_yarnsize = params->pattern->
            yarn_types[lookup_entry.yarn_type].yarnsize;
        extension_y_left = (1.f-tmp_yarnsize)/2.f;
        //right
        lookupPatternEntry(&lookup_entry, params, extension_pattern_x,
                (extension_pattern_y + steps_right_warp + 1));        
        tmp_yarnsize = params->pattern->
            yarn_types[lookup_entry.yarn_type].yarnsize;
        extension_y_right = (1.f-tmp_yarnsize)/2.f;
        if(parallel_weft) {
            //if between two parallel yarns then use parallel yarn to get 
            //extention.
            tmp_yarnsize = params->pattern->
                yarn_types[current_point.yarn_type].yarnsize;
            extension_y_right = (1.f-tmp_yarnsize)/2.f;
        }
		
        //Shrink current warp, if need be
        extension_x_left = -1.f*(1.f-params->pattern->yarn_types[extention_entry.yarn_type].yarnsize)/2.f;
		extension_x_right = -1.f*(1.f-params->pattern->yarn_types[extention_entry.yarn_type].yarnsize)/2.f;
    } else {
        if ((u_repeat)*(float)(params->pattern_width) <= 0.5 && extension_pattern_x == (params->pattern_width-1)) {
            //Special case! If we are on first half of first cell and
            //extension_pattern_x for extention is last cell in pattern. 
            //Then offset the y coordinate with pattern height so that we 
            //continue from extention start. Get correct coordinate behaviour.
            offset_x = (float)(params->pattern_width);
        }
        if ((1.f - u_repeat)*(float)(params->pattern_width) <= 0.5 && extension_pattern_x == 0) {
            //Same when we are on last half of last cell and 
            //extension_pattern_y is on first cell. 
            offset_x = -1.f*(float)(params->pattern_width);
        }

        //Look left at end of segment to see if current needs extending.
        PatternEntry lookup_entry;
        lookupPatternEntry(&lookup_entry, params, 
                (extension_pattern_x - steps_left_weft -1), extension_pattern_y);        
        float tmp_yarnsize = params->pattern->
            yarn_types[lookup_entry.yarn_type].yarnsize;
        extension_x_left = (1.f-tmp_yarnsize)/2.f;
        //right
        lookupPatternEntry(&lookup_entry, params, 
                (extension_pattern_x + steps_right_weft + 1), extension_pattern_y);        
        tmp_yarnsize = params->pattern->
            yarn_types[lookup_entry.yarn_type].yarnsize;
        extension_x_right = (1.f-tmp_yarnsize)/2.f;
        if(parallel_warp) {
            //if between two parallel yarns then use parallel yarn to get 
            //extention.
            tmp_yarnsize = params->pattern->
                yarn_types[current_point.yarn_type].yarnsize;
            extension_x_right = (1.f-tmp_yarnsize)/2.f;
        }
		
        //Shrink current warp, if need be
        extension_y_left = -1.f*(1.f-params->pattern->yarn_types[extention_entry.yarn_type].yarnsize)/2.f;
		extension_y_right = -1.f*(1.f-params->pattern->yarn_types[extention_entry.yarn_type].yarnsize)/2.f;
    }
    //Do not add current cell to width/length if we are on an extention
    //between two parallel yarns.
    float add_current_to_length = 1;
    float add_current_to_width = 1;
    if (parallel_weft) {
        add_current_to_length = 0;

        //MAKE CLEANER!
        //Explain this!
        offset_y += (cell_y > 0.5) ? -1 : 0;
        //If parallel then lookup pattern 
        extension_pattern_y = pattern_y;
    }
    if (parallel_warp) {
        add_current_to_width = 0;

        offset_x += (cell_x > 0.5) ? -1 : 0;

        extension_pattern_x = pattern_x;
        
        //not needed?!
        steps_left_weft=0;
        steps_right_weft=0;
    }
    
    //Yarn-segment-local coordinates.
    float l = (steps_left_warp + steps_right_warp + add_current_to_length + extension_y_left + extension_y_right);
    float y = (((v_repeat)*(float)(params->pattern_height) + offset_y - (float)extension_pattern_y + extension_y_left)
            + steps_left_warp)/l;

	float w = (steps_left_weft + steps_right_weft + add_current_to_width + extension_x_left + extension_x_right);
    float x = (((u_repeat)*(float)(params->pattern_width) + offset_x - (float)extension_pattern_x + extension_x_left)
            + steps_left_weft)/w;
    
    /* Debug */
    /*
    printf("parallel_weft: %d, parallel_warp: %d\n", parallel_weft, parallel_warp);
    printf("add_current_to_length: %f, add_current_to_width: %f\n", add_current_to_length, add_current_to_width);
    printf("steps_left_warp: %d, steps_right_warp: %d\n", steps_left_warp, steps_right_warp);
    printf("steps_left_weft: %d, steps_right_weft: %d\n", steps_left_weft, steps_right_weft);
    printf("extension_y_left: %f, extension_y_right: %f\n", extension_y_left, extension_y_right);
    printf("extension_x_left: %f, extension_x_right: %f\n", extension_x_left, extension_x_right);
    printf("offset_x: %f, offset_y: %f\n", offset_x, offset_y);
    printf("v_repeat: %f, u_repeat: %f\n", v_repeat, u_repeat);
    printf("u_repeat*(float)(params->pattern_width): %f, u_repeat: %f\n", (u_repeat)*(float)(params->pattern_width), u_repeat);
    printf("pattern_x: %d, pattern_y: %d\n", pattern_x, pattern_y);
    printf("extension_pattern_x: %d, extension_pattern_y: %d\n", extension_pattern_x, extension_pattern_y);
    printf("tmp x: %f, y: %f\n", x, y);
    printf("tmp w: %f, l: %f\n", w, l);
    */
    

    //Rescale x, y to [-1,1], w,v scaled by 2
    x = x*2.f - 1.f;
    y = y*2.f - 1.f;
    w = w*2.f;
    l = l*2.f;

    //Switch X and Y for warp, so that we always have the yarn
    // cylinder going along the y axis
    if(!extention_entry.warp_above){
        float tmp1 = x;
        float tmp2 = w;
        x = -y;
        y = tmp1;
        w = l;
        l = tmp2;
    }

    //return the results
    wcPatternData ret_data;
	ret_data.yarn_hit = yarn_hit;
	ret_data.ext_between_parallel = (parallel_warp || parallel_weft) ? 1 : 0;
	ret_data.yarn_type = extention_entry.yarn_type;
    ret_data.length = l; 
    ret_data.width  = w; 
    ret_data.x = x; 
    ret_data.y = y; 
    ret_data.warp_above = extention_entry.warp_above; 
    calculate_segment_uv_and_normal(&ret_data, params, &intersection_data);
    ret_data.total_index_x = total_pattern_x; //total x index of wrapped pattern matrix
    ret_data.total_index_y = total_pattern_y; //total y index of wrapped pattern matrix
    return ret_data;

}

WC_PREFIX
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
        umax = yarn_type_get_umax(params->pattern,data.yarn_type,
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

        float delta_x = yarn_type_get_delta_x(params->pattern, data.yarn_type,
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

            float alpha = yarn_type_get_alpha(params->pattern, data.yarn_type,
				intersection_data.context);
            float beta = yarn_type_get_beta(params->pattern, data.yarn_type,
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

WC_PREFIX
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

    float psi = yarn_type_get_psi(params->pattern,data.yarn_type,
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

        float delta_x = yarn_type_get_delta_x(params->pattern,data.yarn_type,
			intersection_data.context);
        float umax;
        if (data.ext_between_parallel){
            //if segment is extension between to parallel yarns -> bend = 0
            umax = 0.001;
        } else {
            umax = yarn_type_get_umax(params->pattern,data.yarn_type,
                    intersection_data.context);
        }

        //Clamp specular_x
        specular_x = specular_x < 1.f - delta_x ? specular_x :
            1.f - delta_x;
        specular_x = specular_x > -1.f + delta_x ? specular_x :
            -1.f + delta_x;

        if (fabsf(specular_x - x) < delta_x) {

            float alpha = yarn_type_get_alpha(params->pattern,data.yarn_type,
				intersection_data.context);
            float beta  = yarn_type_get_beta( params->pattern,data.yarn_type,
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

WC_PREFIX
wcColor wcEvalDiffuse(wcIntersectionData intersection_data,
        wcPatternData data, const wcWeaveParameters *params)
{
    float value = intersection_data.wi_z;

	YarnType *yarn_type = params->pattern->yarn_types + data.yarn_type;
    if(!yarn_type->color_enabled || !data.yarn_hit){
        yarn_type = &params->pattern->yarn_types[0];
    }

    wcColor color = {
        yarn_type->color[0] * value,
        yarn_type->color[1] * value,
        yarn_type->color[2] * value
    };
	if(yarn_type->color_texmap){
		wc_eval_texmap_color(yarn_type->color_texmap,intersection_data.context,
			&color.r);
		color.r*=value; color.g*=value; color.b*=value;
	}

    return color;
}

WC_PREFIX
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
    float psi = yarn_type_get_psi(params->pattern, data.yarn_type,
		intersection_data.context);
    if (psi <= 0.001f) {
        //Filament yarn
        reflection = wcEvalFilamentSpecular(intersection_data, data, params); 
    } else {
        //Staple yarn
        reflection = wcEvalStapleSpecular(intersection_data, data, params); 
    }
	float specular_noise=yarn_type_get_specular_noise(params->pattern,
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
    float specular_strength = yarn_type_get_specular_strength(params->pattern,
        data.yarn_type,
		intersection_data.context);
    ret.r = ret.r*(1.f-specular_strength) + specular_strength*spec;
    ret.g = ret.g*(1.f-specular_strength) + specular_strength*spec;
    ret.b = ret.b*(1.f-specular_strength) + specular_strength*spec;
    return ret;
    }
