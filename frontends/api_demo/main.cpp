#define TL_NO_TEXTURE_CALLBACKS
#define TL_THUNDERLOOM_IMPLEMENTATION
#include "thunderloom.h"
#include "stdlib.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char **argv)
{
    //TODO(Vidar):Use a ptn file instead...
    const char *errors =0;
    tlWeaveParameters *params =tl_weave_pattern_from_file("test.wif",&errors);
    if(params){
        params->uscale = 1.f;
        params->vscale = 1.f;
        params->intensity_fineness = 0.f;
        params->realworld_uv = 0;
        tl_prepare(params);

        int w =500, h=500;
        unsigned char *pixels = (unsigned char*)calloc(w*h,3);
        float inv_w = 1.f/(float)w;
        float inv_h = 1.f/(float)h;

        tlIntersectionData intersection;
        intersection.wi_x = 0.f;
        intersection.wi_y = 0.f;
        intersection.wi_z = 1.f;

        intersection.wo_x = 0.f;
        intersection.wo_y = 0.f;
        intersection.wo_z = -1.f;

        intersection.context = NULL;

        printf("w: %d h: %d\n", params->pattern_width, params->pattern_height);
        printf("yarn types: %d\n", params->num_yarn_types);

        int y; int x;
        for(y=0;y<h;y++){
            intersection.uv_y = (float)y*inv_h;
            for(x=0;x<w;x++){
                intersection.uv_x = (float)x*inv_w;
                tlPatternData pattern_data = tl_get_pattern_data(intersection, 
                        params);
                tlColor col = tl_shade(intersection, params);
                float pattern_type = (float) pattern_data.yarn_type /
                    (float)(params->num_yarn_types - 1);
                pixels[3*(x + y*h) + 0] = (unsigned char)(255.f * col.r);
                pixels[3*(x + y*w) + 1] = (unsigned char)(255.f * col.g);
                pixels[3*(x + y*w) + 2] = (unsigned char)(255.f * col.b);
            }
        }
        stbi_write_png("out.png", w, h, 3, pixels, 0);
        tl_free_weave_parameters(params);
    }else{
        printf("ERROR: %s\n",errors);
    }
    return 0;
}
