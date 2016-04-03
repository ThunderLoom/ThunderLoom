#include <stdio.h>
#include <stdlib.h>
// For M_PI etc.
#define _USE_MATH_DEFINES
#include <math.h>
#include "noise.h"

int main(int argc, char *argv[]) {

    FILE *fp;

    remove("out.txt");
    fp = fopen("out.txt", "a");

    int size = atoi(argv[1]);
    int octaves = atoi(argv[2]);
    float scale = (float)atof(argv[3]);
    float persistance = (float)atof(argv[4]);

    printf("args, size: %d, octs: %d, scale: %g pers: %g'", size, octaves, scale, persistance);

    int i;
    for (i = 0; i < size; i++) {
        int j;
        for (j = 0; j < size; j++) {
            float   u = ((float)i)/((float)size)*scale,
                    v = ((float)j)/((float)size)*scale;
            fprintf(fp, "%f\n", octavePerlin(u,v,0,octaves,persistance));
        }
    }

    fclose(fp);

    /* OLD2
    int size = atoi(argv[1]);
    float freq = (float)atof(argv[2]);
    float persistance = (float)atof(argv[3]);

    printf("args, size: %d, freq: %g, pers: %g'", size, freq, persistance);

    int i;
    for (i = 0; i < size; i++) {
        int j;
        for (j = 0; j < size; j++) {
            float   u = ((float)i)/((float)size) * freq,
                    v = ((float)j)/((float)size) * freq;
            fprintf(fp, "%f\n", noise(u,v,0));
        }
    }

    fclose(fp);
*/
    // OLD

    //float x = (float)atof(argv[1]);
    //float y = (float)atof(argv[2]);
    //float z = (float)atof(argv[3]);

    //printf("args %s, %s, %s", argv[1], argv[2], argv[3]);

    //call noise and return value
    //double val = noise(x,y,z);

    //printf("Result: noise(%g,%g,%g): %f", x, y, z, val);
    //printf("%f", val);
    return 0;
}

