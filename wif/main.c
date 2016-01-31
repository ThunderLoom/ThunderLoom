#include <stdio.h>
#include "ini.h"

typedef struct{
    int num_threads;
    float spacing, thickness;
}WarpOrWeftData;

typedef struct
{
    WarpOrWeftData warp, weft;
}WeaveData;

static int handler(void* user_data, const char* section, const char* name,
                   const char* value)
{
    WeaveData *data = (WeaveData*)user_data;
    WarpOrWeftData *wdata = 0;
    if(strcmp("WARP",section)==0){
        wdata = &data->warp;
    }
    if(strcmp("WEFT",section)==0){
        wdata = &data->weft;
    }
    if(wdata){
        if(strcmp("Threads",name)==0){
            wdata->num_threads = atoi(value);
        }
        if(strcmp("Spacing",name)==0){
            printf("%s\n",value);
        }
        if(strcmp("Thickness",name)==0){
            printf("%s\n",value);
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    WeaveData data;
    const char *filename = "data/2229.wif";
    FILE *fp = fopen(filename,"rt");
    if (ini_parse(filename, handler, &data) < 0) {
        printf("Could not read \"%s\"\n",filename);
        return 1;
    }
    printf("Warp threads: %d\nWeft threads: %d\n",data.warp.num_threads,
            data.weft.num_threads);
    printf("Warp thickness: %f\nWeft spacing: %f\n",data.warp.thickness,
            data.weft.spacing);
    return 0;
}
