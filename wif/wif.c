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
            uint32_t a = *str - '0';
            if(a >= 0 && a <= 9){
                accum = 10*accum + a;
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

static uint32_t handler(void* user_data, const char* section, const char* name,
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
            data->num_shafts = atoi(value);
        }
        if(strcmp("Treadles",name)==0){
            data->num_treadles = atoi(value);
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
            data->tieup = (uint8_t*)calloc(num_tieup_entries*sizeof(uint8_t));
        }
        x = data->num_treadles - atoi(name);
        for(p = value; p != NULL;
                p = (const char*)strchr(p, ','), p = (p == NULL)? NULL: p+1) {
            int y = data->num_shafts - atoi(p);
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
            data->threading = (uint32_t*)calloc(w*sizeof(uint32_t));
        }
        data->threading[(atoi(name)-1)] = data->num_shafts - atoi(value);
    }
    if(strcmp("TREADLING",section)==0){
        uint32_t w = data->weft.num_threads ;
        if(w <= 0){
            printf("ERROR! Treadling section appeared before specification of "
                    "weft threads!\n");
            return 0;
        }
        if(data->treadling == 0){
            data->treadling = (uint32_t*)calloc(w*sizeof(uint32_t));
        }
        data->treadling[(atoi(name)-1)] = data->num_treadles - atoi(value);
    }
    return 1;
}

WeaveData *wif_read(const char *filename)
{
    WeaveData *data;
    FILE *fp;
    data = (WeaveData*)calloc(sizeof(WeaveData));
    fp = fopen(filename,"rt");
    if (ini_parse(filename, handler, data) < 0) {
        printf("Could not read \"%s\"\n",filename);
    }
    return data;
}

void wif_free_weavedata(WeaveData *data)
{
#define FREE_IF_NOT_NULL(a) if(a!=0) free(a)
    FREE_IF_NOT_NULL(data->tieup);
    FREE_IF_NOT_NULL(data->treadling);
    FREE_IF_NOT_NULL(data->threading);
    FREE_IF_NOT_NULL(data);
}


uint8_t *wif_get_pattern(WeaveData *data, uint32_t *w, uint32_t *h)
{
    uint8_t x,y;
    uint8_t *pattern;
    *w = data->warp.num_threads;
    *h = data->weft.num_threads;
    pattern = (uint8_t*)malloc((*w)*(*h)*sizeof(uint8_t));
    for(y=0;y<*h;y++){
        for(x=0;x<*w;x++){
            uint32_t v = data->threading[x];
            uint32_t u = data->treadling[y];
            pattern[x+y*(*w)] = data->tieup[u+v*data->num_treadles];
        }
    }
    return pattern;
}

