/**
 * @file src/random.c
 *
 * @brief Random Number Generators
 *
 * Piece-Wise Deterministic Random Number Generators.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim/random.h>

#include "xoroshiro.h"
#include "xxtea.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#ifndef _TIMEVAL_DEFINED /* also in winsock[2].h */
#define _TIMEVAL_DEFINED
struct timeval {
	long tv_sec;
	long tv_usec;
};
#endif /* _TIMEVAL_DEFINED */

int gettimeofday(struct timeval* tp, void* tzp) {
	DWORD t;
	t = timeGetTime();
	tp->tv_sec = t / 1000;
	tp->tv_usec = t % 1000;
	return 0;
}
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#endif

#define intrinsics_clz(x)                                                                                              \
	__extension__({                                                                                                \
		assert((x) != 0);                                                                                      \
		__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned), __builtin_clz(x),         \
		    __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned long),                  \
			__builtin_clzl(x),                                                                             \
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned long long),         \
			    __builtin_clzll(x), (void)0)));                                                            \
	})

#define unlikely(exp) __builtin_expect((exp), 0)

uint64_t master_seed = 0;

__attribute__((used)) __attribute__((constructor))
static void init(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	master_seed = ((t.tv_sec * 1000000ULL + t.tv_usec) * 1000) % INT64_MAX;
}

static const uint32_t xxtea_seeding_key[4] = {UINT32_C(0xd0a8f58a), UINT32_C(0x33359424), UINT32_C(0x09baa55b),
    UINT32_C(0x80e1bdb0)};

void initialize_stream(unsigned stream, struct rng_t *ctx)
{
	ctx->state[0] = stream;
	ctx->state[1] = master_seed;
	ctx->state[2] = stream;
	ctx->state[3] = master_seed;
	xxtea_encode((uint32_t *)ctx->state, 8, xxtea_seeding_key);
}

/**
 * @brief Return a random-bak 64-bit value
 * @return The random-bak number
 */
uint64_t RandomU64(struct rng_t *ctx)
{
	return random_u64(ctx->state);
}

/**
 * @brief Return a random-bak value in [0,1] according to a uniform distribution
 * @return The random-bak number
 */
double Random(struct rng_t *ctx)
{
	uint64_t u_val = RandomU64(ctx);
	if(unlikely(!u_val))
		return 0.0;

	double ret = 0.0;
	unsigned lzs = intrinsics_clz(u_val) + 1;
	u_val <<= lzs;
	u_val >>= 12;

	uint64_t exp = 1023 - lzs;
	u_val |= exp << 52;

	memcpy(&ret, &u_val, sizeof(double));
	return ret;
}

/**
 * @brief Return a pair of independent random-bak numbers according to a Standard Normal Distribution
 * @return A pair of random-bak numbers
 */
double Normal(struct rng_t *ctx)
{
	double v1, v2, rsq;
	do {
		v1 = 2.0 * Random(ctx) - 1.0;
		v2 = 2.0 * Random(ctx) - 1.0;
		rsq = v1 * v1 + v2 * v2;
	} while(rsq >= 1.0 || rsq == 0);

	double fac = sqrt(-2.0 * log(rsq) / rsq);
	return v1 * fac; // also v2 * fac is normally distributed and independent
}

int RandomRange(struct rng_t *ctx, int min, int max)
{
	return (int)floor(Random(ctx) * (max - min + 1)) + min;
}

int RandomRangeNonUniform(struct rng_t *ctx, int x, int min, int max)
{
	return (((RandomRange(ctx, 0, x) | RandomRange(ctx, min, max))) % (max - min + 1)) + min;
}

/**
 * @brief Return a number in according to a Gamma Distribution of Integer Order ia
 * Corresponds to the waiting time to the ia-th event in a Poisson process of unit mean.
 *
 * @author D. E. Knuth
 * @param ia Integer Order of the Gamma Distribution
 * @return A random-bak number
 */
double Gamma(struct rng_t *ctx, unsigned ia)
{
	if(ia < 6) {
		// Use direct method, adding waiting times
		double x = 1.0;
		while(ia--)
			x *= 1 - Random(ctx);
		return -log(x);
	}

	double x, y, s;
	double am = ia - 1;
	// Use rejection method
	do {
		double v1, v2;
		do {
			v1 = Random(ctx);
			v2 = 2.0 * Random(ctx) - 1.0;
		} while(v1 * v1 + v2 * v2 > 1.0);

		y = v2 / v1;
		s = sqrt(2.0 * am + 1.0) * y;
		x = s + am;
	} while(x < 0.0 || Random(ctx) > (1.0 + y * y) * exp(am * log(x / am) - s));

	return x;
}

/**
 * @brief Return a random-bak number according to an Exponential distribution with unit mean
 * Corresponds to the waiting time to the next event in a Poisson process of unit mean.
 *
 * @return A random-bak number
 */
double Poisson(struct rng_t *ctx)
{
	return -log(1 - Random(ctx));
}

/**
 * @brief Return a random-bak sample from a Zipf distribution
 * Based on the rejection method by Luc Devroye for sampling:
 * "Non-Uniform Random Variate Generation, page 550, Springer-Verlag, 1986
 *
 * @param skew The skew of the distribution
 * @param limit The largest sample to retrieve
 * @return A random-bak number
 */
unsigned Zipf(struct rng_t *ctx, double skew, unsigned limit)
{
	double b = pow(2., skew - 1.);
	double x, t;
	do {
		x = floor(pow(Random(ctx), -1. / skew - 1.));
		t = pow(1. + 1. / x, skew - 1.);
	} while(x > limit || Random(ctx) * x * (t - 1.) * b > t * (b - 1.));
	return (unsigned)x;
}
