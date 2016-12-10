#define WC_PATTERN_EDITOR_IMPLEMENTATION
#include "pattern_editor.h"
#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int CALLBACK WinMain(
  _In_ HINSTANCE hInstance,
  _In_ HINSTANCE hPrevInstance,
  _In_ LPSTR     lpCmdLine,
  _In_ int       nCmdShow
)
#else
int main(int argc, char **argv)
#endif
{
	const char *error = 0;
	uint8_t warp_above[] = { 0,1,1,0 };
	uint8_t yarn_type[] = { 1,2,2,1 };
	uint32_t pattern_width = 2;
	uint32_t pattern_height = 2;
	wcColor yarn_colors[2];
	yarn_colors[0].r = 1.f; yarn_colors[0].g = 0.f; yarn_colors[0].b = 0.f;
	yarn_colors[1].r = 0.f; yarn_colors[1].g = 0.f; yarn_colors[1].b = 1.f;
	uint32_t num_yarn_types = 2;
	wcWeaveParameters* params = wcWeavePatternFromData(
		warp_above,yarn_type,num_yarn_types,yarn_colors,
		pattern_width, pattern_height);
	if(params){
		wc_pattern_editor(params);
	}else{
        //TODO(Vidar):Report error in a message box
		printf("ERROR! %s\n",error);
	}

	return 0;
}

float wc_eval_texmap_mono(void * a, void * b)
{
    return 0.f;
}

wcColor wc_eval_texmap_color(void * a, void * b)
{
	wcColor ret = {0.f,0.f,0.f};
	return ret;
}

float wc_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context)
{
    return 1.f;
}
