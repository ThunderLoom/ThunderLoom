#include "build/out.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


int main(int argc, char **argv){
    int w = 20;
    int h = 20;
    float max = 0;
    for(int i=0;i<w*h;i++){
        float val = ((float*)normalization_table)[i];
        max = val > max ? val : max;
    }
    unsigned char *pixels = calloc(w*h,3*sizeof(char));
    for(int i=0;i<w*h;i++){
        float val = ((float*)normalization_table)[i];
        pixels[i*3+0] = (unsigned char)(val/max*255);
        pixels[i*3+1] = (unsigned char)(val/max*255);
        pixels[i*3+2] = (unsigned char)(val/max*255);
    }
    stbi_write_png("out.png", 20, 20, 3, pixels, w*h*3);
}
