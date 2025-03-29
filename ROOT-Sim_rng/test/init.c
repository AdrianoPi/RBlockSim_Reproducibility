/**
 * @file test/init.c
 *
 * @brief Test: initialization of the RNG library
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdint.h>
#include <test.h>

extern uint64_t master_seed;

int test_rng_is_initialized(_unused void *_)
{
	test_assert(master_seed != 0);
	return 0;
}

int main(void)
{
	test("RNG is initialized", test_rng_is_initialized, NULL);
	return 0;
}
