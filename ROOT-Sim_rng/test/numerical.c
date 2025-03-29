/**
 * @file test/numerical.c
 *
 * @brief Test: rollbackable RNG
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <ROOT-Sim/random.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static struct rng_t ctx;

/**
 * @brief Kolmogorov-Smirnov src
 * @param N the number of samples to use
 * @param nBins the number of bins to use
 * @param sample the function to use to generate the samples in [0, 1]
 * @return 0 if the src is passed, 1 otherwise
 */
int kolmogorov(uint32_t n_samples, double (*sample)(struct rng_t *ctx))
{
	struct ks_bin {
		double min;
		double max;
		uint_fast32_t count;
	};

	struct ks_bin *bins = malloc(sizeof(*bins) * (n_samples + 1));
	if(bins == NULL) {
		perror("Unable to allocate memory in kolmogorov()");
		return 1;
	}
	memset(bins, 0, sizeof(*bins) * (n_samples + 1));

	int ret = 1;
	for(uint32_t i = 0; i < n_samples; i++) {
		double rf = sample(&ctx);
		uint32_t k = ceil(rf * n_samples);
		if(k > n_samples)
			goto fail;
		struct ks_bin *b = &bins[k];
		b->min = !b->count || rf < b->min ? rf : b->min;
		b->max = !b->count || rf > b->max ? rf : b->max;
		++b->count;
	}

	double threshold = 1.51743 * sqrt(n_samples);
	uint32_t j = 0;
	for(uint32_t i = 0; i < n_samples + 1; i++) {
		struct ks_bin *b = &bins[i];
		if(b->count == 0)
			continue;

		double z = n_samples * b->min - j;
		j += b->count;
		double k = j - b->max * n_samples;
		z = k > z ? k : z;

		if(z > threshold)
			goto fail;
	}
	ret = 0;

fail:
	free(bins);
	return ret;
}

static int aux_ks_test(_unused void *_)
{
	test_assert(kolmogorov(100000000, Random) == 0);
	test_assert(kolmogorov(10000000, Random) == 0);
	test_assert(kolmogorov(1000000, Random) == 0);
	test_assert(kolmogorov(100000, Random) == 0);
	test_assert(kolmogorov(10000, Random) == 0);
	test_assert(kolmogorov(1000, Random) == 0);
	test_assert(kolmogorov(100, Random) == 0);

	return 0;
}

static int random_range_non_uniform_test(_unused void *_)
{
	int passed = 0;
	int x, min, max, r, i;

	for(i = 0; i < 1000000; i++) {
		x = INT_MAX * Random(&ctx);
		max = INT_MAX * Random(&ctx);
		min = (int)(max * Random(&ctx));
		r = RandomRangeNonUniform(&ctx, x, min, max);

		if(r < min || r > max)
			passed = 1;
	}

	return passed;
}

static int random_range_test(_unused void *_)
{
	int passed = 0;
	int min, max, r, i;

	for(i = 0; i < 1000000; i++) {
		max = INT_MAX * Random(&ctx);
		min = (int)(max * Random(&ctx));
		r = RandomRange(&ctx, min, max);

		if(r < min || r > max)
			passed = 1;
	}

	return passed;
}


int main(void)
{
	initialize_stream(0, &ctx);

	test("Kolmogorov-Smirnov test on Random(&ctx)", aux_ks_test, NULL);
	test("Functional test on RandomRange()", random_range_test, NULL);
	test("Functional test on RandomRangeNonUniform()", random_range_non_uniform_test, NULL);
}
