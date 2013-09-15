#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <climits>

#include "random.hpp"

uint32_t Random::x = 123456789;
uint32_t Random::y = 362436069;
uint32_t Random::z = 521288629;
uint32_t Random::w = 88675123;

void Random::SeedGood() {
    auto f = open("/dev/urandom", O_RDONLY);
    auto bytes_read = read(f, &x, sizeof x);
    assert(bytes_read == sizeof x);
    bytes_read = read(f, &y, sizeof y);
    assert(bytes_read == sizeof y);
    bytes_read = read(f, &z, sizeof z);
    assert(bytes_read == sizeof z);
    bytes_read = read(f, &w, sizeof w);
    assert(bytes_read == sizeof w);
    close(f);
}

uint32_t Random::U32() {
    uint32_t t;
    
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

float Random::Float() {
    auto t = U32();
    return static_cast<float>(t)/UINT_MAX;
}

uint32_t Random::U32MinMax(uint32_t min, uint32_t max) {
    auto t = U32() % (max - min + 1);
    return t + min;    
}
