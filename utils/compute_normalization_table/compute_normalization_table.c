#define TL_THUNDERLOOM_IMPLEMENTATION
#define TL_NO_TEXTURE_CALLBACKS
#include "thunderloom.h"
#include <stdio.h>

static const int halton_4_base[] = {2, 3, 5, 7};
static const float halton_4_base_inv[] =
        {1.f/2.f, 1.f/3.f, 1.f/5.f, 1.f/7.f};

//Sets the elements of val to the n-th 4-dimensional point
//in the Halton sequence
static void halton_4(int n, float val[]){
    int j;
    for(j=0;j<4;j++){
        float f = 1.f;
        int i = n;
        val[j] = 0.f;
        while(i>0){
            f = f * halton_4_base_inv[j];
            val[j] = val[j] + f * (i % halton_4_base[j]);
            i = i / halton_4_base[j];
        }
    }
}


float compute_normalization_factor(float umax, float psi, float alpha, float beta)
{
    //Calculate normalization factor for the specular reflection
    int nLocationSamples = 100;
    int nDirectionSamples = 100;

    float highest_result = 0.f;

    tlYarnType yarn_type = tl_default_yarn_type;
    yarn_type.specular_color.r = 1.f;
    yarn_type.specular_amount  = 1.f;
    yarn_type.specular_noise   = 0.f;
    yarn_type.umax  = umax;
    yarn_type.psi   = psi;
    yarn_type.alpha = alpha;
    yarn_type.beta  = beta;

    tlWeaveParameters params = {0};
    params.num_yarn_types = 1;
    params.yarn_types = &yarn_type;
    params.specular_normalization = 1.f;

    // Normalize by the largest reflection across all uv coords and
    // incident directions
    for (int i = 0; i < nLocationSamples; i++) {
        float result = 0.0f;
        float halton_point[4];
        halton_4(i + 50, halton_point);
        tlPatternData pattern_data = {0};
        // Pick a random location on a segment rectangle...
        pattern_data.x = -1.f + 2.f*halton_point[0];
        pattern_data.y = -1.f + 2.f*halton_point[1];
        pattern_data.length = 1.f;
        pattern_data.width = 1.f;
        pattern_data.warp_above = 0;
        pattern_data.yarn_type = 0;
        pattern_data.yarn_hit = 1;
        tlIntersectionData intersection_data = { 0 };
        calculate_segment_uv_and_normal(&pattern_data, &params,
                &intersection_data);

        sample_uniform_hemisphere(halton_point[2], halton_point[3],
                &intersection_data.wi_x, &intersection_data.wi_y,
                &intersection_data.wi_z);

        for (int j = 0; j < nDirectionSamples; j++) {
            float halton_direction[4];
            halton_4(j + 50 + nLocationSamples, halton_direction);
            // Since we use cosine sampling here, we can ignore the cos term
            // in the integral
            sample_cosine_hemisphere(halton_direction[0], halton_direction[1],
                    &intersection_data.wo_x, &intersection_data.wo_y,
                    &intersection_data.wo_z);
            result += tl_eval_specular(intersection_data, pattern_data, &params).r;
        }
        if (result > highest_result) {
            highest_result = result;
        }
    }
    return (float)nDirectionSamples / highest_result;
}

int main(int argc, char **argv){
	float epsilon = 0.0001f;
	int umax_steps = 20;
	int psi_steps = 20;
	int alpha_steps = 20;
    size_t n = umax_steps*psi_steps*alpha_steps;
    float *data = calloc(n, sizeof(float));
    printf("Data size: %g MB\n", (float)n*sizeof(float)/1024.f/1024.f);
	for(int i_umax=0;i_umax<umax_steps;i_umax++){
		float umax = ((float)i_umax / (float)(umax_steps - 1))*(1.f-epsilon) + epsilon;
		for (int i_psi = 0; i_psi < psi_steps; i_psi++) {
			float psi = (float)i_psi / (float)(psi_steps - 1);
			for (int i_alpha = 0; i_alpha < alpha_steps; i_alpha++) {
				float alpha = (float)i_alpha / (float)(alpha_steps - 1);
				float factor = compute_normalization_factor(umax, psi, alpha, 4.f);
                data[i_alpha + alpha_steps*(i_psi + psi_steps*(i_umax))] = factor;
			}
		}
        printf("Progress: %3.0f%%\n", 100.f*(float)(i_umax+1)/(float)umax_steps);
	}
    //TODO(Vidar):Write data for plotting
    FILE *fp = fopen("out.h", "wt");
    if(fp){
        fprintf(fp,"unsigned char normalization_table[] = {\n");
        for(int i=0;i<n*sizeof(float);i++){
            fprintf(fp,"%d,",((unsigned char*)data)[i]);
            if(i%128==127){ fputc('\n',fp);}
        }
        fprintf(fp,"};\n");
        fclose(fp);
    }else{
        printf("ERROR! Could not open output file\n");
    }
    return 0;
}
