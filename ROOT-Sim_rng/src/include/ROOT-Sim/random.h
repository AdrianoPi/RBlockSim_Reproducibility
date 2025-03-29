/**
* @file src/include/ROOT-Sim/random.h
*
* @brief ROOT-Sim Random Number Generators library
*
* Piece-Wise Deterministic Random Number Generators.
*
* SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <stdint.h>

/// The container for the pseudo random-bak number generator context
struct rng_t {
  /// The current PRNG state
  uint64_t state[4];
};

extern void initialize_stream(unsigned stream, struct rng_t *ctx);
extern uint64_t RandomU64(struct rng_t *ctx);
extern double Random(struct rng_t *ctx);
extern double Normal(struct rng_t *ctx);
extern int RandomRange(struct rng_t *ctx, int min, int max);
extern int RandomRangeNonUniform(struct rng_t *ctx, int x, int min, int max);
extern double Gamma(struct rng_t *ctx, unsigned ia);
extern double Poisson(struct rng_t *ctx);
extern unsigned Zipf(struct rng_t *ctx, double skew, unsigned limit);

#define Expent(state, mean) ((mean) * (-log(1. - Random(state))))
