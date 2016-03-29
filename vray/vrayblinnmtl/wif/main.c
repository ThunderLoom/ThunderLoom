#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "wif.h"

int main(UNUSED int argc, UNUSED char **argv)
{
    const char *filename = "data/2229.wif";
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
        PaletteEntry *pattern = wif_get_pattern(data,&w,&h);

        printf("\nPattern:\n");
        for(y = 0; y < h; y++){
            for(x = 0; x < w; x++){
                PaletteEntry entry = pattern[x+y*w];
                float col = entry.color[0];
                printf("%c", col > 0.5f ? 'X' : ' ');
                //printf("%c", entry.warp_above ? ' ' : 'X');
            }
            printf("\n");
        }
        wif_free_pattern(pattern);
    }
    wif_free_weavedata(data);
    return 0;
}
