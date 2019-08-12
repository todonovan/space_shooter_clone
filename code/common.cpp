#include "common.h"

void InitRand(uint32_t seed)
{
    if (seed == 0)
    {
        srand(time(NULL));
    }
    else
    {
        srand(seed);
    }
}

double GenerateRandomDouble(double Min, double Max)
{
    double range = (Max - Min);
    double div = RAND_MAX / range;
    return Min + (rand() / div);
}

float GenerateRandomFloat(float Min, float Max)
{
    float range = (Max - Min);
    float div = RAND_MAX / range;
    return Min + (rand() / div);
}

uint32_t GenerateRandomUnsignedIntFromZeroTo(uint32_t Max)
{
    if ((Max - 1) == RAND_MAX)
    {
        return rand();
    }
    else
    {
        // Chop off all of the values that would cause skew...
        uint32_t end = RAND_MAX / Max; // truncate skew
        
        end *= Max;

        // ... and ignore results from rand() that fall above that limit.
        uint32_t r;
        while ((r = rand()) >= end);

        return r % Max;
    }
}
