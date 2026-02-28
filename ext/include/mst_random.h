/* Copyright (c) 2026 All Rights Reserved
 * Author: Michael "Stomy" Stopa
 * Summary: 32-bit Permuted Congruential Random Number Generator.
 *
 * Based on Minimal C Implementation by M.E. O'Neill / pcg-random.org.
 * Licensed under Apache License 2.0 (NO WARRANTY, see website) */

#include <stdint.h>

typedef struct PcgRandom_
{
    uint64_t state;
    uint64_t increment;
} PcgRandom;

MST_RANDOM_API void PcgRandom_init(PcgRandom* rng, uint64_t seed);
MST_RANDOM_API uint32_t PcgRandom_randomu(PcgRandom* rng);
MST_RANDOM_API float PcgRandom_randomf(PcgRandom* rng);
MST_RANDOM_API uint32_t PcgRandom_roll(PcgRandom* rng, uint32_t numDice, uint32_t dieFaces);

#if MST_RANDOM_IMPL

MST_RANDOM_API void
PcgRandom_init(PcgRandom* rng, uint64_t seed)
{
    rng->state = 0;
    rng->increment = (seed << 1u | 1u);
    PcgRandom_randomu(rng);
    rng->state += 0xC7E51F504ull;
    PcgRandom_randomu(rng);
}

MST_RANDOM_API uint32_t
PcgRandom_randomu(PcgRandom* rng)
{
    uint32_t shift, rotate;
    uint64_t old = rng->state;

    rng->state = old * 6364136223846793005ull + rng->increment;
    shift = ((old >> 18u) ^ old) >> 27u;
    rotate = old >> 59u;
    return (shift >> rotate) | (shift << ((-rotate) & 31));
}

MST_RANDOM_API float
PcgRandom_randomf(PcgRandom* rng)
{
    uint32_t org = PcgRandom_randomu(rng) % 524288u;
    return (float) org / 524288.f;
}

/* Usage: PcgRandom_roll(rngPtr, 3, 6) for result of 3D6 roll */
MST_RANDOM_API uint32_t
PcgRandom_roll(PcgRandom* rng, uint32_t numDice, uint32_t dieFaces)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < numDice; i++) {
        result += PcgRandom_randomu(rng) % dieFaces + 1;
    }
    return result;
}

#endif // MST_RANDOM_IMPL

