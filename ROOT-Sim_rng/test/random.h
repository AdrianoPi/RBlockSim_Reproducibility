/**
 * @file test/random.h
 *
 * @brief Test: rollbackable RNG
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Poisson(void);
#define Expent(mean) ((mean) * Poisson())
extern double Normal(void);
extern int RandomRange(int min, int max);
extern int RandomRangeNonUniform(int x, int min, int max);
extern double Gamma(unsigned ia);
extern unsigned Zipf(double skew, unsigned limit);
