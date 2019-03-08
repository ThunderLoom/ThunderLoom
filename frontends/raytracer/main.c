#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "raytracer.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "thunderloom.h"

int main(int argc, char **argv)
{
	int res = 1024;
	uint8_t *pixels = calloc(res*res, 4 * sizeof(uint8_t));
	printf("Hello\n");

	const char *ptn_file = "C:/Temp/test.ptn";
	const char *error = 0;

	tlWeaveParameters *params = tl_weave_pattern_from_file(ptn_file, &error);
	if (error) {
		printf("Error loading %s:\n %s\n", ptn_file, error);
		return;
	}

	params->uscale = params->vscale = 4.f;
	float sun_x = 0.2f;
	float sun_y = 0.f;
	tl_raytracer_render(sun_x, sun_y, res, params, pixels);
	stbi_write_png("img.png", res, res, 4, pixels, res * 4);
	return 0;
}
