#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/woven_cloth.h"

#define test(fn) \
        printf("\x1b[33m" # fn "\x1b[0m "); \
        test_##fn(); \
        puts("\x1b[1;32m ok \x1b[0m");

static wcWeaveParameters params_fullsize;
static wcWeaveParameters params_halfsize;
static wcWeaveParameters params_2parallel_fullsize;
static wcWeaveParameters params_2parallel_halfsize;
static wcWeaveParameters params_2parallel_full_and_halfsize;
wcIntersectionData intersection_data;
wcYarnSegment yarn;
static float u;
static float v; 
static float width; 
static float length;

static void test_calculates_segment_size_fullsize() {
    wcWeaveParameters *params = &params_fullsize;
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            //printf("x: %d, y: %d\n", x, y);
            u = x/2.f;
            v = y/2.f;
            yarn = wcGetYarnSegment(u, v, params, &intersection_data);
            //printf("w: %f, l: %f\n", yarn.width, yarn.length);
            assert(yarn.width == 1);
            assert(yarn.length == 1);
        }
    }
    u = 1/4.f; v = 1/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.start_u == 0); assert(yarn.start_v == 0);
    u = 1/4.f; v = 3/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.start_u == 0); assert(yarn.start_v == 0.5);
    
    params = &params_2parallel_fullsize; //se 2parallel.png for reference
    for (int x = 0; x <= 2; x++) {
        for (int y = 0; y < 2; y++) {
            //printf("x: %d, y: %d\n", x, y);
            u = x/2.f;
            v = y/2.f;
            yarn = wcGetYarnSegment(u, v,  params, &intersection_data);
            //printf("w: %f, l: %f\n", yarn.width, yarn.length);
            if (y == 1 && (x == 0 || x == 2)) {
                assert(yarn.length == 2);
                assert(yarn.width == 1);
            } else {
                assert(yarn.length == 1);
                assert(yarn.width == 1);
            }
        }
    }
    
    //TODO more test with different lengths. 
    //Would be nice to have Load from string working for this.
}

static void test_calculates_segment_size_halfsize() {
    //pattern with yarnsize set to 0.5 for all yarns.
    
    wcWeaveParameters *params = &params_halfsize;
    for (int x = 0; x < 1; x++) {
        for (int y = 0; y < 1; y++) {
            u = x/4.f + 1.f/4.f;
            v = y/4.f + 1.f/4.f;
            yarn = wcGetYarnSegment(u, v, params, &intersection_data);
            assert(yarn.length == 1.5);
            assert(yarn.width == 0.5);
        }
    }
    u = 1/4.f; v = 1/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.warp_above == 0);
    assert(yarn.start_u == 0 - 0.25/2.f); assert(yarn.start_v == 0.25/2.f);
    u = 1/4.f; v = 3/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.warp_above == 1);
    assert(yarn.start_u == 0.25/2.f); assert(yarn.start_v == 0.5 - 0.25/2.f);

    params = &params_2parallel_halfsize; //se 2parallel.png for reference
    for (int x = 0; x <= 1; x++) {
        for (int y = 0; y < 1; y++) {
            //printf("x: %d, y: %d \n",x,y);
            u = x/2.f + 1.f/4.f;
            v = y/2.f + 1.f/4.f;
            yarn = wcGetYarnSegment(u, v, params, &intersection_data);
            if (y == 1 && (x == 0 || x == 2)) {
                //printf("w: %f, l: %f \n", yarn.width, yarn.length);

                assert(yarn.length == 0.25 + 1 + 1 + 0.25);
                assert(yarn.width == 1 - 0.5);
            } else {
                assert(yarn.length == 1 + 0.25 + 0.25);
                assert(yarn.width == 1 - 0.5);
            }
        }
    }

    //TODO more test with different lengths. 
    //Would be nice to have Load from string working for this.
}

static void test_calculates_segment_size_when_missing_main_yarn_and_hitting_extension() {
    wcPatternData pattern_data;
    wcIntersectionData intersection_data;
    uint8_t value;
    wcWeaveParameters *params = &params_2parallel_full_and_halfsize;

    intersection_data.uv_x = u = 3.f/6.f;
    intersection_data.uv_y = v = 2.f/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    assert(yarn.warp_above == 1);
    assert(yarn.length == 1.5); assert(yarn.width == 1.0);
    assert(yarn.start_u > 0.32 && yarn.start_u < 0.34); assert(yarn.start_v == 0.25 + 0.25/2.f);

    intersection_data.uv_x = u = 3.f/6.f;
    intersection_data.uv_y = v = 0.01;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    assert(yarn.warp_above == 1);
    assert(yarn.length == 1.5); assert(yarn.width == 1.0);
    assert(yarn.start_u > 0.32 && yarn.start_u < 0.34); assert(yarn.start_v == -(1.f - (0.25 + 0.25/2.f)));
}


static void test_calculates_segment_size_full_and_halfsize() {
    //pattern with yarnsize set to 1.0 for yarn1 and 0.5 for yarn 2
    
    wcWeaveParameters *params = &params_2parallel_full_and_halfsize;
    u = 0/2.f;
    v = 0/2.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.length == 1 + 0.25 + 0.25);
    assert(yarn.width == 1);
    
    u = 3/6.f;
    v = 1/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.length == 1);
    assert(yarn.width == 0.5);
    
    u = 2/2.f;
    v = 0/2.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.length == 1 + 0.25 + 0.25);
    assert(yarn.width == 1);

    u = 1/4.f;
    v = 3/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.length == 1 + 1);
    assert(yarn.width == 0.5);
}

static void test_calculateSegmentDim_returns_true_when_hitting_yarn() {
    wcWeaveParameters *params = &params_halfsize;

    u = 1.f/4.f;
    v = 1/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    
    u = 3.f/4.f;
    v = 1/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    
    u = 1/4.f;
    v = 3/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    
    u = 3/4.f;
    v = 3/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
}

static void test_calculateSegmentDim_returns_true_when_hitting_extension() {
    wcWeaveParameters *params = &params_halfsize;

    u = 1.f/4.f; v = 3.f/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1); assert(yarn.warp_above == 1);

    u = 1.f/4.f - 0.5/2.f - 0.01; v = 3.f/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    assert(yarn.between_parallel == 0);
    assert(yarn.warp_above == 0); //extension is weft
}

static void test_calculateSegmentDim_returns_false_when_missing_yarn_and_extension() {
    wcWeaveParameters *params = &params_halfsize;

    u = 1.f/4.f - 0.5/2.f - 0.01; v = 3.f/4.f - 0.5/2.f - 0.01;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 0);
    assert(yarn.between_parallel == 0);
}
/*TODO
 * static void test_calculateSegmentDim_returns_false_when_hitting_between_all_paralllel_yarns() {
    wcWeaveParameters *params = &params_halfsize;
    uint8_t value;

    intersection_data.uv_x = u = 1.f/4.f - 0.5/2.f - 0.01;
    intersection_data.uv_y = v = 3.f/4.f - 0.5/2.f - 0.01;
    value = wcGetYarnSegment(u, v, params, &width, &length);
    assert(value == 0);
}*/

static void test_calculates_segment_between_parallel_warps_halfsize() {
    //pattern with yarnsize set to 1.0 for yarn1 and 0.5 for yarn 2
    wcWeaveParameters *params = &params_2parallel_halfsize;

    u = 0;
    v = 1.f/4.f;
    yarn = wcGetYarnSegment(u, v, params, &intersection_data);
    assert(yarn.yarn_hit == 1);
    assert(yarn.between_parallel == 1);
    assert(yarn.width == 0.5); assert(yarn.length == 0.25 + 0.25);
}

static void setup() {
    intersection_data.wi_z = 1.f;
    intersection_data.context = NULL;

    wcWeaveParameters *params = &params_fullsize;
    params->realworld_uv = 0;
    params->uscale = params->vscale = 1.f;
    wcWeavePatternFromFile(params,"54235plain.wif");
    
    params = &params_halfsize;
    params->realworld_uv = 0;
    params->uscale = params->vscale = 1.f;
    wcWeavePatternFromFile(params,"54235plain.wif");
    params->yarn_types[1].yarnsize = 0.5;
    params->yarn_types[1].yarnsize_enabled = 1;
    params->yarn_types[2].yarnsize = 0.5;
    params->yarn_types[2].yarnsize_enabled = 1;
    
    params = &params_2parallel_fullsize;
    params->realworld_uv = 0;
    params->uscale = params->vscale = 1.f;
    wcWeavePatternFromFile(params,"2parallel.wif");
    
    params = &params_2parallel_halfsize;
    params->realworld_uv = 0;
    params->uscale = params->vscale = 1.f;
    wcWeavePatternFromFile(params,"2parallel.wif");
    params->yarn_types[1].yarnsize = 0.5;
    params->yarn_types[1].yarnsize_enabled = 1;
    params->yarn_types[2].yarnsize = 0.5;
    params->yarn_types[2].yarnsize_enabled = 1;
    //wcFinalizeWeaveParameters(params);
    
    params = &params_2parallel_full_and_halfsize;
    params->realworld_uv = 0;
    params->uscale = params->vscale = 1.f;
    wcWeavePatternFromFile(params,"2parallel.wif");
    params->yarn_types[1].yarnsize = 1.0;
    params->yarn_types[1].yarnsize_enabled = 1;
    params->yarn_types[2].yarnsize = 0.5;
    params->yarn_types[2].yarnsize_enabled = 1;
}

int main(int argc, char **argv)
{
    setup();
    printf("-----------------------------\n");
    test(calculates_segment_size_fullsize);
    test(calculates_segment_size_halfsize);
    test(calculates_segment_size_when_missing_main_yarn_and_hitting_extension);
    test(calculates_segment_size_full_and_halfsize);
    test(calculateSegmentDim_returns_true_when_hitting_yarn);
    test(calculateSegmentDim_returns_true_when_hitting_extension);
    test(calculateSegmentDim_returns_false_when_missing_yarn_and_extension);
    test(calculates_segment_between_parallel_warps_halfsize);
}

//Define dummy wceval for texmaps
float wc_eval_texmap_mono(void *texmap, void *data)
{
	return 1.f;
}
float wc_eval_texmap_mono_lookup(void *texmap, float u, float v, void *data)
{
	return 1.f;
}
wcColor wc_eval_texmap_color(void *texmap, void *data)
{
    wcColor col = {1.f, 1.f, 1.f};
    return col;
}
