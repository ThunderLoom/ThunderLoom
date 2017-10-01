#define TL_THUNDERLOOM_IMPLEMENTATION
#define TL_NO_TEXTURE_CALLBACKS
#include "thunderloom.h"
#define TL_PATTERN_EDITOR_IMPLEMENTATION
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
	tlColor yarn_colors[2];
	yarn_colors[0].r = 0.6f; yarn_colors[0].g = 0.6f; yarn_colors[0].b = 0.6f;
	yarn_colors[1].r = 0.2f; yarn_colors[1].g = 0.2f; yarn_colors[1].b = 0.2f;
	uint32_t num_yarn_types = 2;
	tlWeaveParameters* params = tl_weave_pattern_from_data(
		warp_above,yarn_type,num_yarn_types,yarn_colors,
		pattern_width, pattern_height);
	if(params){
		tl_pattern_editor(params);
	}else{
        //TODO(Vidar):Report error in a message box
		printf("ERROR! %s\n",error);
	}

	return 0;
}
