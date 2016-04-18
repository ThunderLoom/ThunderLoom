#pragma once
//Updates the elements of val to the n-th 6-dimensional point
//in the Halton sequence
static const int halton_6_base[] = {2, 3, 5, 7, 11, 13};
static const float halton_6_base_inv[] =
    {1.f/2.f, 1.f/3.f, 1.f/5.f, 1.f/7.f, 1.f/11.f, 1.f/13};
static void halton_6(int n, float val[]){
    int j;
    for(j=0;j<6;j++){
        float f = 1.f;
        int i = n;
        val[j] = 0.f;
        while(i>0){
            f = f * halton_6_base_inv[j];
            val[j] = val[j] + f * (i % halton_6_base[j]);
            i = i / halton_6_base[j];
        }
    }
}
