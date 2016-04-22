#include "../../src/woven_cloth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    wcWeaveParameters params;
    wcWeavePatternFromFile_wchar(&params,L"test.weave");
    printf("Pattern:\n");
    for(uint32_t y = 0; y < params.pattern_height; y++){
        printf("[ ");
        for(uint32_t x = 0; x < params.pattern_width; x++){
            printf("%d",
                params.pattern_entry[x+y*params.pattern_width].warp_above);
        }
        printf(" ]\n");
    }
    return 0;
}
