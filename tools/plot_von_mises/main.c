#include "../../src/woven_cloth.cpp"

int main(int argc, char **argv)
{
    float b = 2.f;
    float a = 0.f;
    const int num_steps = 40;
    float a_step = 2.f*M_PI/(float)num_steps;
    for(int i=0;i<num_steps;i++){
        float val = vonMises(cosf(a),b);
        //printf("%1.8f,%1.8f\n",a,val);
        printf("%1.8f,%1.8f\n",val*cosf(a),val*sinf(a));
        a += a_step;
    }
    return 0;
}
