#include "out.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <math.h>


int main(int argc, char **argv){
	int num = 20;
    int w = 20;
    int h = 20;
	unsigned char *pixels = calloc(w*h, 3 * sizeof(char));
	for (int j = 0; j < num; j++) {
		float max = 0;
		for (int i = 0; i < w*h; i++) {
			float val = normalization_table[i + j*w*h];
			max = val > max ? val : max;
		}
		printf("max: %f\n", max);
		for (int i = 0; i < w*h; i++) {
			float val = normalization_table[i + j*w*h];
			unsigned char pixel = (unsigned char)(val / max * 255.f);
			pixels[i * 3 + 0] = pixel;
			pixels[i * 3 + 1] = pixel;
			pixels[i * 3 + 2] = pixel;
		}
		char buffer[128];
		sprintf(buffer, "out_%02d.png", j);
		stbi_write_png(buffer, w, h, 3, pixels, w * 3);
	}
}
