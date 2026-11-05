#ifndef RNG_XOSHIRO_H
#define RNG_XOSHIRO_H

/*
 * Thin wrapper around the unmodified reference implementations by
 * David Blackman and Sebastiano Vigna (public domain / CC0):
 *   third_party/xoshiro256plusplus.c  (https://prng.di.unimi.it/xoshiro256plusplus.c)
 *   third_party/splitmix.c            (https://prng.di.unimi.it/splitmix64.c)
 * Both define next(), so each is wrapped in a rename. splitmix64 is used
 * only to expand the integer seed into the 256-bit state, as the authors
 * recommend. Do not compile the files in third_party/ separately.
 */

#include <stdint.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#define next splitmix64_next
#include "third_party/splitmix64.c"
#undef next

#define next xoshiro256pp_next
#include "third_party/xoshiro256plusplus.c"
#undef next

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* Seed the generator. splitmix64 expands the seed into the 256-bit state. */
static inline void xoshiro_init(unsigned long long seed)
{
    x = (uint64_t)seed;
    for (int i = 0; i < 4; i++) s[i] = splitmix64_next();
}

/* Uniformly random uint64_t over [0, 2^64 - 1]. */
static inline uint64_t xoshiro_next(void)
{
    return (uint64_t)xoshiro256pp_next();
}

/* Uniformly random double over [0, 1) with 53 bits of precision. */
static inline double xoshiro_uniform(void)
{
    return (xoshiro_next() >> 11) * 0x1.0p-53;
}

#endif /* RNG_XOSHIRO_H */