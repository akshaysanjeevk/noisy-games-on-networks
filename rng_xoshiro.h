/**
 * @file rng_xoshiro.h
 * @brief Wrapper for third party RNG tools
 * 
 * The following is a wrapper for the (public domain / CC0) xoshiro256++
 * implementations by David Blackman and Sebastiano Vigna (vigna@acm.org):
 *   third_party/xoshiro256plusplus.c (https://prng.di.unimi.it/xoshiro256plusplus.c)
 *   third_party/splitmix64.c         (https://prng.di.unimi.it/splitmix64.c)
 *   third_party/f2x.c                (https://prng.di.unimi.it/f2x.c)
 * The wrapper directly calls the first two files which in turn require the
 * third. Please refer to README.md for more details. Do not compile the files
 * in third_party/ separately.
 * @author Shakti N. Menon
 * @date 06-11-2026
 * @license Distributed under the MIT License (see LICENSE for details).
 */

#ifndef RNG_XOSHIRO_H
#define RNG_XOSHIRO_H

#include <stdint.h>

#if defined(__GNUC__)
// save the current state of compiler warning states in a stack
#pragma GCC diagnostic push
// suppress compiler warning related to signed/unsigned integers
#pragma GCC diagnostic ignored "-Wsign-compare"
// suppress warning for K&R-style function declarations
// (e.g. "uint64_t next()" instead of "uint64_t next(void)")
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

// Note that there are two different functions named next() in
// xoshiro256plusplus.c and splitmix64.c, so each is wrapped in a
// separate name.
#define next splitmix64_next
#include "third_party/splitmix64.c"
#undef next

#define next xoshiro256pp_next
#include "third_party/xoshiro256plusplus.c"
#undef next

// restores the most recently saved compiler warning states
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// we require a 256-bit state specified by 4 unsigned 64-bit integers,
// so we assign the single seed to the state variable "x" of splitmix64
// and generate 4 values to fill the state array
static inline void xoshiro_init(unsigned long long seed)
{
    x = (uint64_t)seed;
    for (int i = 0; i < 4; i++) s[i] = splitmix64_next();
}

// generate a uniformly random 64-bit unsigned integer from [0, 2^64 - 1].
static inline uint64_t xoshiro_next(void)
{
    return (uint64_t)xoshiro256pp_next();
}

// generate a double-precision value from the uniform distribution [0, 1).
static inline double xoshiro_uniform(void)
{
    // generate a 64-bit random integer and shift right by 11 bit to obtain
    // 53 bits (the fractional precision of a standard IEEE 754 double),
    // and then scale by (1/2^53) to map it to [0, 1)
    return (xoshiro_next() >> 11) * 0x1.0p-53;
}

#endif /* RNG_XOSHIRO_H */
/*=============================================================================*/