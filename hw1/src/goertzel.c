#include <stdint.h>
#include <math.h>

#include "debug.h"
#include "goertzel.h"

void goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k) {
    // TO BE IMPLEMENTED
    double a = 2 * M_PI * (k/N);
    double b = 2 * cos(a);
    gp -> N = N;
    gp -> k = k;
    gp -> A = a;
    gp -> B = b;
    gp -> s0 = 0;
    gp -> s1 = 0;
    gp -> s2 = 0;
}

void goertzel_step(GOERTZEL_STATE *gp, double x) {
    // TO BE IMPLEMENTED
    for(int i = 0; i < gp -> N; i++) {
    	gp -> s0 = x + (gp -> B * gp -> s1) - gp -> s2;
    	gp -> s2 = gp -> s1;
    	gp -> s1 = gp -> s0;
    }
}

double goertzel_strength(GOERTZEL_STATE *gp, double x) {
    // TO BE IMPLEMENTED
    double y;
    //real and imaginary vars to be used for C and D
    double real = cos(gp -> A);
    double imaginary = sin(gp -> A);
	gp -> s0 = x + (gp -> B * gp -> s1) - gp -> s2;
	//y = gp -> s0 -
    return 0;
}
