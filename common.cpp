#include "common.h"

float GenerateRandomFloat(float min, float max)
{
    srand(time(NULL));
    float range = (max - min);
    float div = RAND_MAX / range;
    return min + (rand() / div);
}