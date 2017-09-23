#include <string.h>
#include <stdlib.h>
#include "wif.h"
#include "ini.h" //TODO(Vidar): Write this ourselves

//TODO(Vidar):Report errors in a proper way, not by printf

//NOTE(Vidar):The sections and keys of the WIF file
#define DATA_WARP_SECTION                1<<0
#define DATA_WEFT_SECTION                1<<1
    #define WARP_OR_WEFT_THREADS_KEY     1<<0
    #define WARP_OR_WEFT_SPACING_KEY     1<<1
    #define WARP_OR_WEFT_THICKNESS_KEY   1<<2

#define DATA_WEAVING_SECTION             1<<2
    #define WEAVING_SHAFTS_KEY           1<<0
    #define WEAVING_TREADLES_KEY         1<<1

#define DATA_TIEUP_SECTION               1<<3
#define DATA_THREADING_SECTION           1<<4
#define DATA_TREADLING_SECTION           1<<5
//NOTE(Vidar): In these section, read_keys is the number of values read

#define DATA_COLOR_PALETTE_SECTION       1<<6
    #define COLOR_PALETTE_ENTRIES_KEY    1<<0

#define DATA_COLOR_TABLE_SECTION         1<<7
#define DATA_WARP_COLORS_SECTION         1<<8
#define DATA_WEFT_COLORS_SECTION         1<<9
//NOTE(Vidar): In these section, read_keys is the number of values read

const uint32_t DATA_ALL_SECTIONS = DATA_WARP_SECTION | DATA_WEFT_SECTION | 
    DATA_WEAVING_SECTION | DATA_TIEUP_SECTION | DATA_THREADING_SECTION | 
    DATA_TREADLING_SECTION | DATA_COLOR_PALETTE_SECTION | 
    DATA_COLOR_TABLE_SECTION | DATA_WARP_COLORS_SECTION | 
    DATA_WEFT_COLORS_SECTION;

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

static const char *get_section_name(uint32_t section)
{
    switch(section){
        case DATA_WARP_SECTION:
            return "WARP";
        case DATA_WEFT_SECTION:
            return "WEFT";
        case DATA_WEAVING_SECTION:
            return "WEAVING";
        case DATA_TIEUP_SECTION:
            return "TIEUP";
        case DATA_THREADING_SECTION:
            return "THREADING";
        case DATA_TREADLING_SECTION:
            return "TREADLING";
        case DATA_COLOR_PALETTE_SECTION:
            return "COLOR PALETTE";
        case DATA_COLOR_TABLE_SECTION:
            return "COLOR_TABLE";
        case DATA_WARP_COLORS_SECTION:
            return "WARP_COLORS";
        case DATA_WEFT_COLORS_SECTION:
            return "WEFT_COLORS";
        default:
            return "";
    }
}

#define CHECK_KEY(key,section,name) if(!(data->read_keys & key)){ \
    printf("ERROR! Missing key \"" name "\" in section \"" section "\"\n");\
    return 1;}
//NOTE(Vidar): Returns 1 if there was an error
static uint8_t set_section(WeaveData *data, uint32_t section)
{
    if(section != data->current_section){
        if(section & data->read_sections){
            printf("ERROR! section \"%s\" appeared twice!\n",
                    get_section_name(section));
            return 1;
        }
        switch(data->current_section){
            case DATA_WARP_SECTION:
                CHECK_KEY(WARP_OR_WEFT_THREADS_KEY  ,"WARP","Threads");
                CHECK_KEY(WARP_OR_WEFT_SPACING_KEY,  "WARP","Spacing");
                CHECK_KEY(WARP_OR_WEFT_THICKNESS_KEY,"WARP","Thickness");
                break;
            case DATA_WEFT_SECTION:
                CHECK_KEY(WARP_OR_WEFT_THREADS_KEY  ,"WEFT","Threads");
                CHECK_KEY(WARP_OR_WEFT_SPACING_KEY,  "WEFT","Spacing");
                CHECK_KEY(WARP_OR_WEFT_THICKNESS_KEY,"WEFT","Thickness");
                break;
            case DATA_WEAVING_SECTION:
                CHECK_KEY(WEAVING_SHAFTS_KEY,   "WEAVING", "Shafts");
                CHECK_KEY(WEAVING_TREADLES_KEY, "WEAVING", "Treadles");
                break;
            case DATA_COLOR_PALETTE_SECTION:
                CHECK_KEY(COLOR_PALETTE_ENTRIES_KEY, "COLOR PALETTE", "Entries");
                break;
        }
        data->read_sections |= data->current_section;
        data->current_section = section;
    }
    return 0;
}

static int32_t handler(void* user_data, const char* section, const char* name,
                   const char* value)
{
    WeaveData *data = (WeaveData*)user_data;
    WarpOrWeftData *wdata = 0;
    if(strcmp("WARP",section)==0){
        if(set_section(data, DATA_WARP_SECTION)) return 0;
        wdata = &data->warp;
    }
    if(strcmp("WEFT",section)==0){
        if(set_section(data, DATA_WEFT_SECTION)) return 0;
        wdata = &data->weft;
    }
    if(wdata){
        if(strcmp("Threads",name)==0){
            data->read_keys |= WARP_OR_WEFT_THREADS_KEY;
            uint32_t v = (uint32_t)atoi(value);
            if(v == 0){
                printf("ERROR! Threads cannot be 0\n");
                return 0;
            }
            wdata->num_threads = v;
        }
        if(strcmp("Spacing",name)==0){
            data->read_keys |= WARP_OR_WEFT_SPACING_KEY;
            if(!string_to_float(value,&wdata->spacing)){
                printf("could not read %s in [%s]!\n",name,section);
                return 0;
            }
        }
        if(strcmp("Thickness",name)==0){
            data->read_keys |= WARP_OR_WEFT_THICKNESS_KEY;
            if(!string_to_float(value,&wdata->thickness)){
                printf("could not read %s in [%s]!\n",name,section);
                return 0;
            }
        }
    }
    if(strcmp("WEAVING",section)==0){
        if(set_section(data, DATA_WEAVING_SECTION)) return 0;
        if(strcmp("Shafts",name)==0){
            data->read_keys |= WEAVING_SHAFTS_KEY;
            uint32_t v = (uint32_t)atoi(value);
            if(v == 0){
                printf("ERROR! Shafts cannot be 0\n");
                return 0;
            }
            data->num_shafts = v;
        }
        if(strcmp("Treadles",name)==0){
            data->read_keys |= WEAVING_TREADLES_KEY;
            uint32_t v = (uint32_t)atoi(value);
            if(v == 0){
                printf("ERROR! Treadles cannot be 0\n");
                return 0;
            }
            data->num_treadles = v;
        }
    }
    if(strcmp("TIEUP",section)==0){
        if(set_section(data, DATA_TIEUP_SECTION)) return 0;
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
        uint32_t index = (uint32_t)atoi(name);
        if(index > data->num_treadles || index == 0){
            printf("ERROR! Tieup entry %s is invalid\n", name);
            return 0;
        }
        x = data->num_treadles - index;
        for(p = value; p != NULL && *p != '\0';){
            uint32_t entry = (uint32_t)atoi(p);
            if(entry > data->num_shafts || entry == 0){
                printf("ERROR! Tieup entry %s contains the invalid value %d\n",
                        name, entry);
                return 0;
            }
            uint32_t y = data->num_shafts - entry;
            data->tieup[x + y*data->num_treadles] = 1;
            p = (const char*)strchr(p, ',');
            p = (p == NULL)? NULL: p+1;
        }
    }
    if(strcmp("THREADING",section)==0){
        if(set_section(data, DATA_THREADING_SECTION)) return 0;
        uint32_t w = data->warp.num_threads ;
        if(w <= 0){
            printf("ERROR! Threading section appeared before specification of "
                    "warp threads!\n");
            return 0;
        }
        if(data->threading == 0){
            data->threading = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        //TODO(Vidar): Generalize this?
        uint32_t index = (uint32_t)atoi(name);
        if(index > w || index == 0){
            printf("ERROR! Threading entry %s is out of bounds\n",name);
            return 0;
        }
        uint32_t entry = (uint32_t)atoi(value);
        if(entry > data->num_shafts || entry == 0){
            printf("ERROR! Threading value %s at entry %s is out of bounds\n",
                    name, value);
            return 0;
        }
        data->threading[index-1] = data->num_shafts - entry;
    }
    if(strcmp("TREADLING",section)==0){
        if(set_section(data, DATA_TREADLING_SECTION)) return 0;
        uint32_t w = data->weft.num_threads ;
        if(w <= 0){
            printf("ERROR! Treadling section appeared before specification of "
                    "weft threads!\n");
            return 0;
        }
        if(data->treadling == 0){
            data->treadling = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        uint32_t index = (uint32_t)atoi(name);
        if(index > w || index == 0){
            printf("ERROR! Treadling entry %s is out of bounds\n",name);
            return 0;
        }
        uint32_t entry = (uint32_t)atoi(value);
        if(entry > data->num_treadles || entry == 0){
            printf("ERROR! Treadling value %s at entry %s is out of bounds\n",
                    name, value);
            return 0;
        }
        data->treadling[(index-1)] = data->num_treadles - entry;
    }
    if(strcmp("COLOR PALETTE",section)==0){
        if(set_section(data, DATA_COLOR_PALETTE_SECTION)) return 0;
        if(strcmp("Entries",name)==0){
            data->read_keys |= COLOR_PALETTE_ENTRIES_KEY;
            data->num_colors = (uint32_t)atoi(value);
        }
    }
    if(strcmp("COLOR TABLE",section)==0){
        if(set_section(data, DATA_COLOR_TABLE_SECTION)) return 0;
        if(data->num_colors==0){
            printf("ERROR! COLOR TABLE appeared before specification of "
                    "COLOR PALETTE\n");
            return 0;
        }
        if(data->colors == 0){
            data->colors = (float*)calloc(data->num_colors,sizeof(float)*3);
        }
        uint32_t i = (uint32_t)atoi(name)-1;
        //TODO(Vidar):Handle different formats
        if(i<data->num_colors){
            const char *p = value;
            data->colors[i*3+0] = (float)(atoi(p))/255.0f;
            p = strchr(p, ',')+1;
            if(p == (void*)1){
                printf("Invalid color at color table entry %s\n",name);
                return 0;
            }
            data->colors[i*3+1] = (float)(atoi(p))/255.0f;
            p = strchr(p, ',')+1;
            if(p == (void*)1){
                printf("Invalid color at color table entry %s\n",name);
                return 0;
            }
            data->colors[i*3+2] = (float)(atoi(p))/255.0f;
        }
    }
    if(strcmp("WARP COLORS",section)==0){
        if(set_section(data, DATA_WARP_COLORS_SECTION)) return 0;
        uint32_t w = data->warp.num_threads ;
        if(w <= 0){
            printf("ERROR! WARP COLORS section appeared before specification of "
                    "warp threads!\n");
            return 0;
        }
        if(data->warp.colors == 0){
            data->warp.colors = (uint32_t*)calloc(w,sizeof(uint32_t));
        }
        uint32_t index = (uint32_t)atoi(name);
        if(index > w || index == 0){
            printf("ERROR! Warp color entry %s is out of bounds\n",name);
            return 0;
        }
        uint32_t entry = (uint32_t)atoi(value);
        if(entry > data->num_colors || entry == 0){
            printf("ERROR! Warp color value %s at entry %s is out of bounds\n",
                    name, value);
            return 0;
        }
        data->warp.colors[(index-1)] = entry-1;
    }
    if(strcmp("WEFT COLORS",section)==0){
        if(set_section(data, DATA_WEFT_COLORS_SECTION)) return 0;
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
        uint32_t index = (uint32_t)atoi(name);
        if(index > w || index == 0){
            printf("ERROR! Weft color entry %s is out of bounds\n",name);
            return 0;
        }
        uint32_t entry = (uint32_t)atoi(value);
        if(entry > data->num_colors || entry == 0){
            printf("ERROR! Weft color value %s at entry %s is out of bounds\n",
                    name, value);
            return 0;
        }
        data->weft.colors[(index-1)] = entry-1;
    }
    return 1;
}

struct read_data{
        char *str;
        long len;
};

static char* read_from_string(char* str, int num, void* stream)
{
    struct read_data *data = (struct read_data*)stream;
    if(data->len >0){
            char *ret = str;
            num--;
            num = num > data->len ? data->len : num;
            int end = 0;
            for(int i=0;i<num;i++){
                if(*data->str != '\n'){
                        *str = *data->str;
                        str++;
                        data->str++;
                        data->len--;
                        end++;
                }
            }
            str[0] = 0;
            if(data->str[0] == '\n'){
                data->len--;
                data->str++;
            }
            return ret;
    }
    return NULL;
}

WeaveData *wif_read(char *in_data, long len, const char **error)
{
    WeaveData *data;
    data = (WeaveData*)calloc(1,sizeof(WeaveData));
    struct read_data d = {in_data,len};
    int e = ini_parse_stream((ini_reader)read_from_string,&d,handler,data);
    if(e != 0){
        printf("ERROR! %d\n",e);
        *error = "Error reading file";
        wif_free_weavedata(data);
        return 0;
    }
    data->read_sections |= data->current_section;
    if(data->read_sections != DATA_ALL_SECTIONS){
        //NOTE(Vidar): One of the sections was missing from the file.
        // Check which one and report it!
#define WIF_CHECK_SECTION(var,section,name) if(!(var & section)) {\
    *error = ("Missing section " name); goto ret;}
        WIF_CHECK_SECTION(data->read_sections, DATA_WARP_SECTION, "WARP")
        WIF_CHECK_SECTION(data->read_sections, DATA_WEFT_SECTION, "WEFT")
        WIF_CHECK_SECTION(data->read_sections, DATA_WEAVING_SECTION, "WEAVING")
        WIF_CHECK_SECTION(data->read_sections, DATA_TIEUP_SECTION, "TIEUP")
        WIF_CHECK_SECTION(data->read_sections, DATA_THREADING_SECTION, "THREADING")
        WIF_CHECK_SECTION(data->read_sections, DATA_TREADLING_SECTION, "TREADLING")
        WIF_CHECK_SECTION(data->read_sections,
            DATA_COLOR_PALETTE_SECTION, "COLOR PALETTE")
        WIF_CHECK_SECTION(data->read_sections,
            DATA_COLOR_TABLE_SECTION, "COLOR TABLE")
        WIF_CHECK_SECTION(data->read_sections,
            DATA_WARP_COLORS_SECTION, "WARP COLORS")
        WIF_CHECK_SECTION(data->read_sections,
            DATA_WEFT_COLORS_SECTION, "WEFT COLORS")
#undef WIF_CHECK_SECTION
        ret:
        wif_free_weavedata(data);
        return 0;
    }
    return data;
}


void wif_free_weavedata(WeaveData *data)
{
    if(data){
#define FREE_IF_NOT_NULL(a) if(a!=0) free(a)
        FREE_IF_NOT_NULL(data->tieup);
        FREE_IF_NOT_NULL(data->treadling);
        FREE_IF_NOT_NULL(data->threading);
        FREE_IF_NOT_NULL(data->colors);
        FREE_IF_NOT_NULL(data->warp.colors);
        FREE_IF_NOT_NULL(data->weft.colors);
        FREE_IF_NOT_NULL(data);
    }
}


//NOTE(Vidar): This function takes the data which was read from the WIF file
// and converts it to the data used by the shader
void wif_get_pattern(tlWeaveParameters *param, WeaveData *data, uint32_t *w,
    uint32_t *h, float *rw, float *rh)
{
    uint32_t x,y;
    if(data == 0){
        //NOTE(Vidar): The file was invalid...
        tlYarnType *yarn_types =
            (tlYarnType*)calloc(1,sizeof(tlYarnType));
		yarn_types[0]=tl_default_yarn_type;
        param->yarn_types = yarn_types;
        param->num_yarn_types = 1;
        *w = 0;
        *h = 0;
        param->pattern = 0;
        return;
    }

	//Pattern width/height in num of elements
	*w = data->warp.num_threads;
	*h = data->weft.num_threads;

	//Real width/height in meters
	//TODO(Peter): Assuming unit in wif is centimeters for thickness and
	// spacing. Make it more general.
	*rw = REALWORLD_UV_WIF_TO_MM*(*w * (data->warp.thickness)
			+ (*w - 1) * (data->warp.spacing)); 
	*rh = REALWORLD_UV_WIF_TO_MM*(*h * (data->weft.thickness)
			+ (*h - 1) * (data->weft.spacing)); 

    if(*w > 0 && *h >0){
        uint32_t c;
        PatternEntry *entries =
            (PatternEntry*)calloc((*w)*(*h),sizeof(PatternEntry));
        tlYarnType *yarn_types =
            (tlYarnType*)calloc(data->num_colors+1,sizeof(tlYarnType));

		yarn_types[0]=tl_default_yarn_type;
        for(c=1;c<data->num_colors+1;c++){
            yarn_types[c].color.r = data->colors[(c-1)*3+0];
            yarn_types[c].color.g = data->colors[(c-1)*3+1];
            yarn_types[c].color.b = data->colors[(c-1)*3+2];
			yarn_types[c].color_enabled=1;
		#define TL_FLOAT_PARAM(name)\
            yarn_types[c].name = tl_default_yarn_type.name;\
            yarn_types[c].name##_enabled = 0;
		#define TL_COLOR_PARAM(name)
            TL_YARN_PARAMETERS
		#undef TL_FLOAT_PARAM
		#undef TL_COLOR_PARAM
        }
        for(y=0;y<*h;y++){
            for(x=0;x<*w;x++){
                uint32_t v = data->threading[x];
                uint32_t u = data->treadling[y];
                uint8_t warp_above = data->tieup[u+v*data->num_treadles];
                uint32_t yarn_type = (warp_above ? data->warp.colors[x]
                    : data->weft.colors[y])+1;
                uint32_t index = x+y*(*w);
                entries[index].warp_above = warp_above;
                entries[index].yarn_type = yarn_type;
            }
        }
        param->pattern = entries;
		param->num_yarn_types = data->num_colors+1;
        param->yarn_types = yarn_types;
    }
}

void wif_free_pattern(PatternEntry *pattern)
{
    free(pattern);
}
