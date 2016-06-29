#include <stdio.h>
#include "../../src/woven_cloth.cpp"

int main(int argc, char **argv){
    for(int i=1;i<argc;i++){
        printf("Reading file %s\n",argv[i]);
        wcWeaveParameters params;
        WeaveData *data = wif_read(argv[i]);
        params.pattern_entry = wif_get_pattern(data,
            &params.pattern_width, &params.pattern_height,
            &params.pattern_realwidth, &params.pattern_realheight);
        wif_free_weavedata(data);
        printf("%d, %d\n",params.pattern_height, params.pattern_width);
        /*if(params.pattern_height == 0 || params.pattern_width == 0){
            return 1;
        }*/
    }
    return 0;
}
