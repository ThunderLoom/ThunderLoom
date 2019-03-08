#pragma once

struct tlRaytracerParameters;

struct tlRaytracerParameters *tl_raytracer_load_parameters(struct tlWeaveParameters *params)
;

struct tlWeaveParameters;

void tl_raytracer_render_to_memory(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, uint8_t *pixels)
;

void tl_raytracer_render_to_opengl_pbo(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, unsigned int pbo_gl)
;
