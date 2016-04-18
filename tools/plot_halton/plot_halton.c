#include "../../src/halton.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *f = fopen("out.txt","wt");
    if(f){
        float val[6] = {};
        int start = 20;
        int num = 1000;
        for(int n=start;n<=start+num;n++){
            halton_6(n,val);
            fprintf(f,"%d, %f, %f, %f, %f, %f, %f\n",n,val[0],val[1],val[2],
                val[3],val[4],val[5]);
        }
        fclose(f);
    }
    return 0;
}
