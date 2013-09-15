#ifndef __RANDOM__
#define __RANDOM__

#include <stdint.h>

class Random {
private:
    static uint32_t x;
    static uint32_t y;
    static uint32_t z;
    static uint32_t w;

public:

    static void SeedGood();
    static uint32_t U32();
    static float Float();
    static uint32_t U32MinMax(uint32_t min, uint32_t max);
};

#endif  // __RANDOM__
