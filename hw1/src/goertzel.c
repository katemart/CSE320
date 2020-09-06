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
    gp -> s0 = x + (gp -> B * gp -> s1) - gp -> s2;
    gp -> s2 = gp -> s1;
    gp -> s1 = gp -> s0;
}

double goertzel_strength(GOERTZEL_STATE *gp, double x) {
    // TO BE IMPLEMENTED
    /*
    We have that A = 2pi(k/N) and C = e^-jA and D = e^-j(2pik/N)(N-1)
    So then, C = cosA - jsinA
    y = s0 - s1 * C ----> y = s0 - s1(cosA - jsinA)
    y = y * D ----------> y = s0 - s1(cosA - jsinA) * D
    y = [s0 - s1(cosA - jsinA)] * [cos(A*(N-1)) - jsin(A*(N-1))]
    Then let vars a, b, c and d represent the real and imaginary parts (respectively) of y
    multiplication of complex nums: (ac - bd) + j (ad + bc)
    y = part1 + part2 so then y^2 = part1^2 + part2^2
    return (2*y^2)/N^2
    */
    gp -> s0 = x + (gp -> B * gp -> s1) - gp -> s2;
    double a = gp -> s0 - (gp -> s1 * cos(gp -> A));        //s0 - s1(cosA)
    double b = gp -> s1 * sin(gp -> A);                     //s1 * sinA
    double c = cos(gp -> A * (gp -> N - 1));                //cosA * N-1
    double d = sin(gp -> A * (gp -> N - 1));                //sinA * N-1
    double p = (a * c) - (b * d);                           //a
    double q = (a * d) + (b * c);                           //b
    double y = (p * p) + (q * q);                           //y^2 = a^2 + b^2
    return (2 * y)/(gp -> N * gp -> N);                     //2*y^2/N^2
}
