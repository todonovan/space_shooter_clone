#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <windows.h>

// A simple typedef providing a bool guaranteed to be 32 bits
// By contract, zero is false and nonzero is true.
// By convention, use the value one (1) for true where possible.
// DO NOT USE FOR TRINARY, ETC. LOGIC. 
typedef uint32_t bool32_t;

// A set of simple float-equals macros
#define FLT_EQ_EPS(a, b, eps) ((fabs((a) - (b))) < (eps))
#define FLT_EQ(a, b) FLT_EQ_EPS(a, b, 0.001f)

struct color_triple
{
    uint8_t Red;
    uint8_t Blue;
    uint8_t Green;
};


float GenerateRandomFloat(float min, float max);


#endif