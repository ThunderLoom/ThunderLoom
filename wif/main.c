#include <stdio.h>
#include <stdint.h>
#include "wif.h"

int main(int argc, char **argv)
{
    const char *filename = "data/41753.wif";
    WeaveData *data = wif_read(filename);
    {
        uint32_t x,y;
        printf("\nTieup:\n");
        for(y = 0; y < data->num_shafts; y++){
            for(x = 0; x < data->num_treadles; x++){
                printf("%c",data->tieup[x+y*data->num_treadles] ? 'X' : '.');
            }
            printf("\n");
        }
        printf("\nThreading:\n");
        for(y = 0; y < data->num_shafts; y++){
            for(x = 0; x < data->warp.num_threads; x++){
                printf("%c",data->threading[x] == y ? 'X' : '.');
            }
            printf("\n");
        }
        printf("\nTreadling:\n");
        for(y = 0; y < data->weft.num_threads; y++){
            for(x = 0; x < data->num_treadles; x++){
                printf("%c",data->treadling[y] == x ? 'X' : '.');
            }
            printf("\n");
        }
    }
    {
        uint32_t x,y,w,h;
        uint8_t *pattern = wif_get_pattern(data,&w,&h);

        printf("\nPattern:\n");
        for(y = 0; y < h; y++){
            for(x = 0; x < w; x++){
                printf("%c",pattern[x+y*w] ? ' ' : 'X');
            }
            printf("\n");
        }
    }
    wif_free_weavedata(data);
    return 0;
}
