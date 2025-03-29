#pragma once
#include <math.h>
#include "RBlockSim.h"
#include "../external/bitmap.h"

/**
 * @brief Draws a number from a Normal distribution with `mean` mean and `std_dev` standard deviation
 *
 * @param[in,out] rng_t pointer to RNG state
 * @param[in] mean of the Normal distribution
 * @param[in] std_dev standard deviation of the Normal distribution
 *
 * @return Value drawn from requested Normal distribution
 */
double NormalExpanded(struct rng_t *ctx, double mean, double std_dev);

/**
 * @brief Debug wrapper for bitmap_check. Checks a bit in a bitmap
 *
 * @param bitmap a pointer to the bitmap.
 * @param bit_index the index of the bit to read
 * @return 0 if the bit is not set, 1 otherwise
 *
 * Avoid side effects in the arguments, they may be evaluated more than once.
 */
int bitmap_check_aux(block_bitmap *bitmap, size_t bit_index);
