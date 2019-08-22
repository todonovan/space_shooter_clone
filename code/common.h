#pragma once

// Forward-declarations
#include "common.fwd.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <windows.h>

// A set of simple float-equals macros
#define FLT_EQ_EPS(a, b, eps) ((fabs((a) - (b))) < (eps))
#define FLT_EQ(a, b) FLT_EQ_EPS(a, b, 0.001f)

struct color_triple
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
};

void InitRand(uint32_t Seed);
float GenerateRandomFloat(float Min, float Max);
double GenerateRandomDouble(double Min, double Max);
uint32_t GenerateRandomUnsignedIntFromZeroTo(uint32_t Max);