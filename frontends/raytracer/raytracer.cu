#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>

#include <Windows.h>
#include <gl\GL.h>

#include <cuda_gl_interop.h>
#include <device_launch_parameters.h>

#define TL_FUNC_PREFIX __device__
#define TL_NO_TEXTURE_CALLBACKS
#define TL_THUNDERLOOM_IMPLEMENTATION
extern "C"{
#include "thunderloom.h"
}

__global__
void trace_rays(float3 sun_dir, uint8_t *pixels, tlWeaveParameters *params)
{
	int w = blockDim.x*gridDim.x;
	int h = blockDim.y*gridDim.y;
	int x = blockIdx.x*blockDim.x + threadIdx.x;
	int y = blockIdx.y*blockDim.y + threadIdx.y;
	int i = x + y * w;
	float fx = 2.f*(float)x/(float)w - 1.f;
	float fy = 2.f*(float)y/(float)h - 1.f;

	tlIntersectionData intersection_data;
	intersection_data.uv_x = fx;
	intersection_data.uv_y = fy;
	intersection_data.wi_x = sun_dir.x;
	intersection_data.wi_y = sun_dir.y;
	intersection_data.wi_z = sun_dir.z;

	intersection_data.wo_x = 0.f;
	intersection_data.wo_y = 0.f;
	intersection_data.wo_z = 1.f;
	intersection_data.context = 0;

	tlColor col = tl_shade(intersection_data, params);
	float mult = 5.f;
	col.r *= mult;
	col.g *= mult;
	col.b *= mult;
	
	pixels[i * 4 + 0] = (uint8_t)(saturate(col.r)*255.f);
	pixels[i * 4 + 1] = (uint8_t)(saturate(col.g)*255.f);
	pixels[i * 4 + 2] = (uint8_t)(saturate(col.b)*255.f);
	pixels[i * 4 + 3] = 255;
}

#define CHECK_CUDA_CALL(call) if((err= call) != cudaSuccess){ printf("Error %d %s!\n   %s\n", err,\
	cudaGetErrorName(err), cudaGetErrorString(err)); abort();}

struct tlRaytracerParameters
{
	tlWeaveParameters *d_weave_params;
	tlYarnType *d_yarn_types;
	PatternEntry *d_pattern;
};

extern "C"
struct tlRaytracerParameters *tl_raytracer_load_parameters(tlWeaveParameters *params)
{
	enum cudaError err;

	int num_yarn_types = params->num_yarn_types;
	tlYarnType *d_yarn_types = 0;
	CHECK_CUDA_CALL(cudaMalloc(&d_yarn_types, num_yarn_types * sizeof(tlYarnType)));
	CHECK_CUDA_CALL(cudaMemcpy(d_yarn_types, params->yarn_types, num_yarn_types * sizeof(tlYarnType), cudaMemcpyHostToDevice));

	int res_pattern = params->pattern_width*params->pattern_height;
	PatternEntry *d_pattern;
	CHECK_CUDA_CALL(cudaMalloc(&d_pattern, res_pattern * sizeof(PatternEntry)));
	CHECK_CUDA_CALL(cudaMemcpy(d_pattern, params->pattern, res_pattern * sizeof(PatternEntry), cudaMemcpyHostToDevice));

	tlWeaveParameters tmp_params = *params;
	tmp_params.yarn_types = d_yarn_types;
	tmp_params.pattern = d_pattern;
	tlWeaveParameters *d_params;
	CHECK_CUDA_CALL(cudaMalloc(&d_params, sizeof(tlWeaveParameters)));
	CHECK_CUDA_CALL(cudaMemcpy(d_params, &tmp_params, sizeof(tlWeaveParameters), cudaMemcpyHostToDevice));
	return 0;
}


extern "C"
void tl_raytracer_render_to_memory(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, uint8_t *pixels)
{
	enum cudaError err;

	int num_yarn_types = params->num_yarn_types;
	tlYarnType *d_yarn_types = 0;
	CHECK_CUDA_CALL(cudaMalloc(&d_yarn_types, num_yarn_types * sizeof(tlYarnType)));
	CHECK_CUDA_CALL(cudaMemcpy(d_yarn_types, params->yarn_types, num_yarn_types * sizeof(tlYarnType), cudaMemcpyHostToDevice));

	int res_pattern = params->pattern_width*params->pattern_height;
	PatternEntry *d_pattern;
	CHECK_CUDA_CALL(cudaMalloc(&d_pattern, res_pattern * sizeof(PatternEntry)));
	CHECK_CUDA_CALL(cudaMemcpy(d_pattern, params->pattern, res_pattern * sizeof(PatternEntry), cudaMemcpyHostToDevice));

	tlWeaveParameters tmp_params = *params;
	tmp_params.yarn_types = d_yarn_types;
	tmp_params.pattern = d_pattern;
	tlWeaveParameters *d_params;
	CHECK_CUDA_CALL(cudaMalloc(&d_params, sizeof(tlWeaveParameters)));
	CHECK_CUDA_CALL(cudaMemcpy(d_params, &tmp_params, sizeof(tlWeaveParameters), cudaMemcpyHostToDevice));

	dim3 dim_block(res, 1, 1);
	dim3 dim_grid(1, res, 1);

	uint8_t *d_pixels;
	CHECK_CUDA_CALL(cudaMalloc(&d_pixels, res*res * 4 * sizeof(uint8_t)));

	float3 sun_dir;
	sun_dir.x = sun_x;
	sun_dir.y = sun_y;
	sun_dir.z = 1.f - sqrtf(sun_x*sun_x + sun_y*sun_y);
	trace_rays <<<dim_grid, dim_block >>> (sun_dir, d_pixels, d_params);

	CHECK_CUDA_CALL(cudaDeviceSynchronize());

	CHECK_CUDA_CALL(cudaMemcpy(pixels, d_pixels, res*res * 4 * sizeof(uint8_t), cudaMemcpyDeviceToHost));

	CHECK_CUDA_CALL(cudaFree(d_pixels));
}


extern "C"
void tl_raytracer_render_to_opengl_pbo(float sun_x, float sun_y, int res,
	tlWeaveParameters *params, unsigned int pbo_gl)
{
	enum cudaError err;

	uint8_t *d_pixels = 0;
	size_t size = 0;
	cudaGraphicsResource_t pbo_cuda;
	CHECK_CUDA_CALL(cudaGraphicsGLRegisterBuffer(&pbo_cuda, pbo_gl, cudaGraphicsRegisterFlagsWriteDiscard));
	CHECK_CUDA_CALL(cudaGraphicsMapResources(1, &pbo_cuda, 0));
	CHECK_CUDA_CALL(cudaGraphicsResourceGetMappedPointer((void**)&d_pixels, &size, pbo_cuda));

	int num_yarn_types = params->num_yarn_types;
	tlYarnType *d_yarn_types = 0;
	CHECK_CUDA_CALL(cudaMalloc(&d_yarn_types, num_yarn_types * sizeof(tlYarnType)));
	CHECK_CUDA_CALL(cudaMemcpy(d_yarn_types, params->yarn_types, num_yarn_types * sizeof(tlYarnType), cudaMemcpyHostToDevice));

	int res_pattern = params->pattern_width*params->pattern_height;
	PatternEntry *d_pattern;
	CHECK_CUDA_CALL(cudaMalloc(&d_pattern, res_pattern * sizeof(PatternEntry)));
	CHECK_CUDA_CALL(cudaMemcpy(d_pattern, params->pattern, res_pattern * sizeof(PatternEntry), cudaMemcpyHostToDevice));

	tlWeaveParameters tmp_params = *params;
	tmp_params.yarn_types = d_yarn_types;
	tmp_params.pattern = d_pattern;
	tlWeaveParameters *d_params;
	CHECK_CUDA_CALL(cudaMalloc(&d_params, sizeof(tlWeaveParameters)));
	CHECK_CUDA_CALL(cudaMemcpy(d_params, &tmp_params, sizeof(tlWeaveParameters), cudaMemcpyHostToDevice));

	dim3 dim_block(res, 1, 1);
	dim3 dim_grid(1, res, 1);

	float3 sun_dir;
	sun_dir.x = sun_x;
	sun_dir.y = sun_y;
	sun_dir.z = 1.f - sqrtf(sun_x*sun_x + sun_y*sun_y);
	trace_rays <<<dim_grid, dim_block >>> (sun_dir, d_pixels, d_params);

	CHECK_CUDA_CALL(cudaGraphicsUnmapResources(1, &pbo_cuda, 0));
	CHECK_CUDA_CALL(cudaGraphicsUnregisterResource(pbo_cuda));

	CHECK_CUDA_CALL(cudaFree(d_yarn_types));
	CHECK_CUDA_CALL(cudaFree(d_pattern));
	CHECK_CUDA_CALL(cudaFree(d_params));
}

