#include <stdlib.h>
#include <stdio.h>
// For M_PI etc.
#include <math.h>
#include "noise.cpp"


// This file is only used for testing purposes. Generates random numbers
// and stores them in a text file. These can then be visualized by
// another program i.e. matlab.
int main(int argc, char *argv[]) {
    remove("out.txt");
    FILE *fp;
    fp = fopen("out.txt", "a");
    
    int size = atoi(argv[1]);
    int octaves = atoi(argv[2]);
    float scale = (float)atof(argv[3]);
    float persistance = (float)atof(argv[4]);
    printf("args, size: %d, octs: %d, scale: %g pers: %g'", size, octaves, scale, persistance);

    int i;
    for (i = 0; i < size; i++) {
        //int j;
        //for (j = 0; j < size; j++) {
            float   u = ((float)i)/((float)size)*scale;
          //          v = ((float)j)/((float)size)*scale;
            fprintf(fp, "%f\n", octavePerlin(u,0,0,octaves,persistance));
       // }
    }
    fclose(fp);
    return 0;
}

