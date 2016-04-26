#include <string.h>
#include <stdlib.h>
#include "wif.h"
#include "ini.h" //TODO(Vidar): Write this ourselves

static int string_to_float(const char *str, float *val)
{
    uint32_t accum  = 0;
    uint32_t scale = 10;
    char dot_encountered = 0;
    while(*str){
        if(*str == '.'){
            dot_encountered = 1;
        }else{
            int32_t a = *str - '0';
            if(a >= 0 && a <= 9){
                accum = 10*accum + (uint32_t)a;
                if(dot_encountered){
                    scale *=10;
                }
            }else{
                //invalid character
                return 0;
            }
        }
        str++;
    }
    *val = (float)accum / (float)scale;
    return 1;
}

//TODO(Vidar): Bounds check! IMPORTANT!
static int32_t handler(void* user_data, const char* section, const char* name,
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
            wdata->num_threads = (uint32_t)atoi(value);
        }
        if(strcmp("Spacing",name)==0){
            if(!string_to_float(value,&wdata->spacing)){
                printf("could not read %s in [%s]!\n",name,section);
                return 0;
            }
        }
        if(strcmp("Thickness",name)==0){
            if(!string_to_float(value,&wdata->thickness)){
                printf("could not read %s in [%s]!\n",name,section);
                return 0;
            }
        }
    }
    if(strcmp("WEAVING",section)==0){
        if(strcmp("Shafts",name)==0){
            data->num_shafts = (uint32_t)atoi(value);
        }
        if(strcmp("Treadles",name)==0){
            data->num_treadles = (uint32_t)atoi(value);
        }
    }
    if(strcmp("TIEUP",section)==0){
        const char *p;
        uint32_t x;
        uint32_t num_tieup_entries = data->num_treadles * data->num_shafts;
        if(num_tieup_entries <= 0){
            printf("Tieup section appeared before specification of "
                    "Shafts and Treadles!\n");
            return 0;
        }
        if(data->tieup == 0){
            data->tieup = (uint8_t*)calloc(num_tieup_entries,sizeof(uint8_t));
        }
        x = data->num_treadles - (uint32_t)atoi(name);
        for(p = value; p != NULL;
                p = (const char*)strchr(p, ','), p = (p == NULL)? NULL: p+1) {
            uint32_t y = data->num_shafts - (uint32_t)atoi(p);
            data->tieup[x + y*data->num_treadles] = 1;
        }
    }
    if(strcmp("THREADING",section)==0){
        uint32_t w = data->warp.num_threads ;
        if(w <= 0){
            printf("ERROR! Threading section appeared before specification of "
                    "warp threads!\n");
            return 0;
        }
        if(data->threading == 0){
            data->threading = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        data->threading[((uint32_t)atoi(name)-1)] = data->num_shafts
            - (uint32_t)atoi(value);
    }
    if(strcmp("TREADLING",section)==0){
        uint32_t w = data->weft.num_threads ;
        if(w <= 0){
            printf("ERROR! Treadling section appeared before specification of "
                    "weft threads!\n");
            return 0;
        }
        if(data->treadling == 0){
            data->treadling = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        data->treadling[((uint32_t)atoi(name)-1)] = data->num_treadles
            - (uint32_t)atoi(value);
    }
    if(strcmp("COLOR PALETTE",section)==0){
        if(strcmp("Entries",name)==0){
            data->num_colors = (uint32_t)atoi(value);
        }
    }
    if(strcmp("COLOR TABLE",section)==0){
        if(data->num_colors==0){
            printf("ERROR! COLOR TABLE appeared before specification of "
                    "COLOR PALETTE\n");
            return 0;
        }
        if(data->colors == 0){
            data->colors = (float*)calloc(data->num_colors,sizeof(float)*3);
        }
        uint32_t i = (uint32_t)atoi(name)-1;
        //TODO(Vidar):Make sure all entries exist, handle different formats
        const char *p = value;
        data->colors[i*3+0] = (float)(atoi(p))/255.0f;
        p = strchr(p, ',')+1;
        data->colors[i*3+1] = (float)(atoi(p))/255.0f;
        p = strchr(p, ',')+1;
        data->colors[i*3+2] = (float)(atoi(p))/255.0f;
    }
    if(strcmp("WARP COLORS",section)==0){
        uint32_t w = data->warp.num_threads ;
        if(w <= 0){
            printf("ERROR! WARP COLORS section appeared before specification of "
                    "warp threads!\n");
            return 0;
        }
        if(data->warp.colors == 0){
            data->warp.colors = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        data->warp.colors[((uint32_t)atoi(name)-1)] = (uint32_t)atoi(value)-1;
    }
    if(strcmp("WEFT COLORS",section)==0){
        //TODO(Vidar): Join with the case above...
        uint32_t w = data->weft.num_threads ;
        if(w <= 0){
            printf("ERROR! WEFT COLORS section appeared before specification of "
                    "weft threads!\n");
            return 0;
        }
        if(data->weft.colors == 0){
            data->weft.colors = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        data->weft.colors[((uint32_t)atoi(name)-1)] = (uint32_t)atoi(value)-1;
    }
    return 1;
}

WeaveData *wif_read(const char *filename)
{
    WeaveData *data;
    data = (WeaveData*)calloc(1,sizeof(WeaveData));
    if (ini_parse(filename, handler, data) < 0) {
        printf("Could not read \"%s\"\n",filename);
    }
    return data;
}


WeaveData *wif_read_wchar(const wchar_t *filename)
{
    WeaveData *data;
    data = (WeaveData*)calloc(1,sizeof(WeaveData));
    #ifdef WIN32
    FILE* file;
    int error = -1;

    file = _wfopen(filename, L"rt");
    if(file){
        error = ini_parse_file(file, handler, data);
        fclose(file);
    }
    if (error < 0) {
        wprintf(L"Could not read \"%s\"\n",filename);
    }
    #endif
    return data;
}


void wif_free_weavedata(WeaveData *data)
{
#define FREE_IF_NOT_NULL(a) if(a!=0) free(a)
    FREE_IF_NOT_NULL(data->tieup);
    FREE_IF_NOT_NULL(data->treadling);
    FREE_IF_NOT_NULL(data->threading);
    FREE_IF_NOT_NULL(data->colors);
    FREE_IF_NOT_NULL(data->warp.colors);
    FREE_IF_NOT_NULL(data->weft.colors);
    FREE_IF_NOT_NULL(data);
}


PatternEntry *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h, float *rw, float *rh)
{
    uint32_t x,y;
    PatternEntry *pattern = 0;

    //Pattern width/height in num of elements
    //TODO(Peter) should these not be reversed? :/
    *w = data->warp.num_threads;
    *h = data->weft.num_threads;

    //Real width/height in meters
    //TODO(Peter): Assuming unit in wif is centimeters for thickness and spacing. Make it more general.
    *rw = 10.0*(*rw * (data->warp.thickness) + (*rw - 1) * (data->warp.spacing)); 
    *rh = 10.0*(*rh * (data->weft.thickness) + (*rh - 1) * (data->weft.spacing)); 

    if(*w > 0 && *h >0){
        pattern = (PatternEntry*)malloc((*w)*(*h)*sizeof(PatternEntry));
        for(y=0;y<*h;y++){
            for(x=0;x<*w;x++){
                uint32_t v = data->threading[x];
                uint32_t u = data->treadling[y];
                uint8_t  warp_above = data->tieup[u+v*data->num_treadles];
                float *col = data->colors + (warp_above ? data->warp.colors[x]
                    : data->weft.colors[y])*3;
                pattern[x+y*(*w)].warp_above = warp_above;
                pattern[x+y*(*w)].color[0] = col[0];
                pattern[x+y*(*w)].color[1] = col[1];
                pattern[x+y*(*w)].color[2] = col[2];
            }
        }
    }
    return pattern;
}


void wif_free_pattern(PatternEntry *pattern)
{
    free(pattern);
}
